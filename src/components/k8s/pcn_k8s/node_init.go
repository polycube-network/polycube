/*
 * Copyright 2018 The Polycube Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package main

import (
	"crypto/rand"
	"fmt"
	"net"
	"os"
	"os/exec"

	"github.com/containernetworking/plugins/pkg/ip"

	k8sfilter "github.com/polycube-network/polycube/src/components/k8s/utils/k8sfilter"
	k8switch "github.com/polycube-network/polycube/src/components/k8s/utils/k8switch"

	//v1 "k8s.io/api/core/v1"

	"github.com/polycube-network/polycube/src/components/k8s/utils"

	"github.com/vishvananda/netlink"

	log "github.com/sirupsen/logrus"
)

// This loadBalancer handles the node port services traffic
func createLBRPNode(node *k8sNode) error {
	log.Debugf("Creating %s", k8switchName)

	// port connected to br0
	toStack := k8switch.Ports{Name: "toStack"}

	// port connected to physical nic (through k8sfilter)
	toNodePort := k8switch.Ports{Name: "toNodePort", Type_: "NODEPORT"}

	ports := []k8switch.Ports{toStack, toNodePort}

	serviceClusterIPRange := os.Getenv("POLYCUBE_SERVICE_CLUSTERIP_RANGE")
	if serviceClusterIPRange == "" {
		log.Warningf("POLYCUBE_SERVICE_CLUSTERIP_RANGE not found, defaulting to %s",
		serviceClusterIPRangeDefault)
			serviceClusterIPRange = serviceClusterIPRangeDefault
	}

	lb := k8switch.K8switch{
		Name:                k8switchName,
		Ports:               ports,
		ClusterIpSubnet:     serviceClusterIPRange,
		ClientSubnet:        node.GetPodCIDR().String(),
		VirtualClientSubnet: node.GetVPodCIDR().String()}

	if _, err := k8switchAPI.CreateK8switchByID(k8switchName, lb); err != nil {
		return err
	}

	log.Debugf("%s created", k8switchName)
	return nil
}

func createK8sFilter() error {

	nodePortServiceRange := os.Getenv("POLYCUBE_SERVICE_NODEPORT_RANGE")
	if nodePortServiceRange == "" {
		log.Warningf("POLYCUBE_SERVICE_NODEPORT_RANGE not found, defaulting to %s",
			serviceNodePortRangeDefault)
		nodePortServiceRange = serviceNodePortRangeDefault
	}

	external := k8sfilter.Ports{Name: "external", Type_: "EXTERNAL"}
	internal := k8sfilter.Ports{Name: "internal", Type_: "INTERNAL"}

	ports := []k8sfilter.Ports{external, internal}

	k8sf := k8sfilter.K8sfilter{Name: "k8sf",
								Ports: ports,
								NodeportRange: nodePortServiceRange}
	if _, err := k8sfilterAPI.CreateK8sfilterByID("k8sf", k8sf); err != nil {
		return err
	}

	log.Debugf("k8sf created")
	return nil
}

func generateRandomMac() net.HardwareAddr {
	buf := make([]byte, 6)
	_, err := rand.Read(buf)
	if err != nil {
		log.Errorf("error: %s", err)
		return nil
	}

	buf[0] |= 0x02 // Set the local bit
	buf[0] &= 0xFE // Unicast
	return buf
}

func connectModules(nodeportIface string) error {
	/*** connect k8sfilter to k8switch ***/
	if _, err := k8switchAPI.UpdateK8switchPortsPeerByID(k8switchName, "toNodePort",
		"k8sf:internal"); err != nil {
		return err
	}
	if _, err := k8sfilterAPI.UpdateK8sfilterPortsPeerByID("k8sf",
		"internal", k8switchName+":toNodePort"); err != nil {
		return err
	}
	/*** connect k8sfilter to public interface ***/
	if _, err := k8sfilterAPI.UpdateK8sfilterPortsPeerByID("k8sf",
		"external", nodeportIface); err != nil {
		return err
	}

	return nil
}

// returns first usable address in a network
func calcGatewayIP(ipn *net.IPNet) net.IP {
	nid := ipn.IP.Mask(ipn.Mask)
	return ip.NextIP(nid)
}

// returns second usable address in a network
func calcLoadBalancerIP(ipn *net.IPNet) net.IP {
	nid := ipn.IP.Mask(ipn.Mask)
	return ip.NextIP(ip.NextIP(nid))
}

func setupVeth(node *k8sNode) (netlink.Link, netlink.Link, error) {
	veth := &netlink.Veth{
		LinkAttrs: netlink.LinkAttrs{
			Name:  polycubeK8sInterface,
			Flags: net.FlagUp,
		},
		PeerName: polycubeLBInterface,
	}
	if err := netlink.LinkAdd(veth); err != nil {
		return nil, nil, err
	}

	veth1, err := netlink.LinkByName(polycubeK8sInterface)
	if err != nil {
		netlink.LinkDel(veth)
		return nil, nil, err
	}

	ip0 := &net.IPNet{IP: calcGatewayIP(node.GetPodCIDR()),
		Mask: node.GetPodCIDR().Mask}
	if err := netlink.AddrAdd(veth1, &netlink.Addr{IPNet: ip0}); err != nil {
		log.Errorf("error adding address %s", polycubeK8sInterface)
		return nil, nil, err
	}

	ip1 := &net.IPNet{IP: calcGatewayIP(node.GetVPodCIDR()),
		Mask: node.GetVPodCIDR().Mask}
	if err := netlink.AddrAdd(veth1, &netlink.Addr{IPNet: ip1}); err != nil {
		log.Errorf("error adding address %s", polycubeK8sInterface)
		return nil, nil, err
	}

	veth2, err := netlink.LinkByName(polycubeLBInterface)
	if err != nil {
		netlink.LinkDel(veth)
		return nil, nil, err
	}

	if err := netlink.LinkSetUp(veth2); err != nil {
		return nil, nil, err
	}

	return veth1, veth2, nil
}

func (node *k8sNode) Init() error {
	nodeIP = node.PrivateIP.String()
	log.Debugf("NodeIP: %s", nodeIP)

	// create the network topology
	hostNetworkConf, err := utils.GetHostNetworkConf(nodeIP)
	if err != nil {
		return err
	}

	node.PrivateMask = hostNetworkConf.PrivateMask

	if err := createLBRPNode(node); err != nil {
		log.Debugf("createLBRP: %s", err)
		return err
	}

	if err := createK8sFilter(); err != nil {
		log.Debugf("createK8sFilter: %s", err)
		return err
	}

	if err := connectModules(hostNetworkConf.NodeportIface); err != nil {
		log.Debugf("connectModules: %s", err)
		return err
	}

	// create veth pair and network namespace
	out, err := exec.Command("/init.sh", hostNetworkConf.DefaultIface).CombinedOutput()
	if err != nil {
		log.Errorf("error intializing node: %s", out)
		return err
	}

	link1, _, err := setupVeth(node)
	if err != nil {
		log.Errorf("error intializing node: %s", err.Error())
		return err
	}

	// connect k8s to stack
	if _, err := k8switchAPI.UpdateK8switchPortsPeerByID(k8switchName, "toStack",
		polycubeLBInterface); err != nil {
		return err
	}

	// add default gateway for k8switch
	gwIP := calcGatewayIP(node.GetPodCIDR())
	fwdEntryGW := k8switch.FwdTable{
		Address: gwIP.String(),
		Mac:     link1.Attrs().HardwareAddr.String(),
		Port:    "toStack",
	}
	if _, err := k8switchAPI.CreateK8switchFwdTableByID(k8switchName,
		fwdEntryGW.Address, fwdEntryGW); err != nil {
		return fmt.Errorf("Error creating fwdEntry entry %s", err)
	}

	// add default gateway for k8switch
	fwdEntry := k8switch.FwdTable{
		Address: "0.0.0.0",
		Mac:     link1.Attrs().HardwareAddr.String(),
		Port:    "toStack",
	}
	if _, err := k8switchAPI.CreateK8switchFwdTableByID(k8switchName,
		fwdEntry.Address, fwdEntry); err != nil {
		return fmt.Errorf("Error creating fwdEntry entry %s", err)
	}

	// add static fake entry for reaching the LB
	lbIP := calcLoadBalancerIP(node.GetVPodCIDR())
	arpentry := netlink.Neigh{
		LinkIndex:    link1.Attrs().Index,
		State:        netlink.NUD_PERMANENT,
		IP:           lbIP,
		HardwareAddr: generateRandomMac(),
	}
	if err := netlink.NeighAdd(&arpentry); err != nil {
		log.Error("Error adding static arp entry")
		return err
	}

	// FIXME: This policy is causing problems for the NodePort services,
	// there is not a solution yet, so disable it.
	// route to reach that node without trasnversing a nat
	//if out, err := exec.Command("ip", "rule", "add", "from",
	//	"192.168.0.0/16", "table", "1000").CombinedOutput(); err != nil {
	//	log.Debugf("error creating table 1000: %s", out)
	//	return err
	//}

	return nil
}

func (node *k8sNode) Uninit() error {

	// polycube services will be deleted automatically

	vxlanlink, err := netlink.LinkByName(vxlanInterface)
	if err == nil {
		netlink.LinkDel(vxlanlink)
	}

	link, err := netlink.LinkByName(polycubeK8sInterface)
	if err == nil {
		netlink.LinkDel(link)
	}

	return nil
}
