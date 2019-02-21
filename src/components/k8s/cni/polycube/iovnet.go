/*
 * Copyright 2014 CNI authors
 * Copyright 2017 The Polycube Authors
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
	"encoding/json"
	"errors"
	"fmt"
	"math/big"
	"net"
	"os"
	"runtime"
	"strings"

	"github.com/containernetworking/cni/pkg/skel"
	"github.com/containernetworking/cni/pkg/types"
	"github.com/containernetworking/cni/pkg/types/current"
	"github.com/containernetworking/cni/pkg/version"
	"github.com/containernetworking/plugins/pkg/ip"
	"github.com/containernetworking/plugins/pkg/ipam"
	"github.com/containernetworking/plugins/pkg/ns"
	"github.com/vishvananda/netlink"
	"golang.org/x/sys/unix"

	k8sfirewall "github.com/SunSince90/polycube/src/components/k8s/utils/k8sfirewall"
	k8switch "github.com/polycube-network/polycube/src/components/k8s/utils/k8switch"

	log "github.com/sirupsen/logrus"
)

type NetConf struct {
	types.NetConf
	MTU          int    `json:"mtu"`
	VClusterCIDR string `json:"vclustercdir"`
}

type gwInfo struct {
	gws               []net.IPNet
	family            int
	defaultRouteFound bool
}

const (
	basePath             = "http://127.0.0.1:9000/polycube/v1"
	polycubeK8sInterface = "pcn_k8s"
)

var (
	k8switchAPI *k8switch.K8switchApiService
	fwAPI       *k8sfirewall.FirewallApiService
)

func init() {
	log.SetLevel(log.DebugLevel)
	// this ensures that main runs only on main thread (thread group leader).
	// since namespace ops (unshare, setns) are done for a single thread, we
	// must ensure that the goroutine does not jump from OS thread to thread
	runtime.LockOSThread()

	//  init load balancer API
	cfgK8switch := k8switch.Configuration{BasePath: basePath}
	srK8switch := k8switch.NewAPIClient(&cfgK8switch)
	k8switchAPI = srK8switch.K8switchApi

	//	for firewall creation
	cfgK8firewall := k8sfirewall.Configuration{BasePath: basePath}
	srK8firewall := k8sfirewall.NewAPIClient(&cfgK8firewall)
	fwAPI = srK8firewall.FirewallApi
}

func loadNetConf(bytes []byte) (*NetConf, string, error) {
	n := &NetConf{}
	if err := json.Unmarshal(bytes, n); err != nil {
		return nil, "", fmt.Errorf("failed to load netconf: %v", err)
	}

	return n, n.CNIVersion, nil
}

func setupVeth(netns ns.NetNS, ifName string, mtu int, containerID string) (*current.Interface, *current.Interface, error) {
	contIface := &current.Interface{}
	hostIface := &current.Interface{}

	err := netns.Do(func(hostNS ns.NetNS) error {
		// create the veth pair in the container and move host end into host netns
		hostVeth, containerVeth, err := ip.SetupVeth(ifName, mtu, hostNS)
		if err != nil {
			return err
		}
		contIface.Name = containerVeth.Name
		contIface.Mac = containerVeth.HardwareAddr.String()
		contIface.Sandbox = netns.Path()
		hostIface.Name = hostVeth.Name
		return nil
	})
	if err != nil {
		return nil, nil, err
	}

	// need to lookup hostVeth again as its index has changed during ns move
	hostVeth, err := netlink.LinkByName(hostIface.Name)
	if err != nil {
		return nil, nil, fmt.Errorf("failed to lookup %q: %v", hostIface.Name, err)
	}
	hostIface.Mac = hostVeth.Attrs().HardwareAddr.String()

	return hostIface, contIface, nil
}

func dupIP(ip net.IP) net.IP {
	dup := make(net.IP, len(ip))
	copy(dup, ip)
	return dup
}

func calculateVirtualIP(ip net.IP, vClusterCIDR string) (net.IP, error) {
	var vPodsClusterCDIR *net.IPNet
	_, vPodsClusterCDIR, err := net.ParseCIDR(vClusterCIDR)
	if err != nil {
		log.Errorf("%s cannot be parsed", vClusterCIDR)
		return nil, err
	}

	vpodsClusterInt := big.NewInt(0).SetBytes(vPodsClusterCDIR.IP.To4())
	clusterMask := big.NewInt(0).SetBytes(vPodsClusterCDIR.Mask)
	ipInt := big.NewInt(0).SetBytes(ip.To4())

	//vpodsClusterInt | (ipInt & ~clusterMask)
	vpodsClusterInt.Or(vpodsClusterInt, big.NewInt(0).And(ipInt, big.NewInt(0).Not(clusterMask)))
	return net.IP(vpodsClusterInt.Bytes()), nil
}

func cmdAdd(args *skel.CmdArgs) error {
	var success bool = false
	log.Debugf("polycube cmdAdd for %s", args.ContainerID)
	n, cniVersion, err := loadNetConf(args.StdinData)
	if err != nil {
		return err
	}

	netns, err := ns.GetNS(args.Netns)
	if err != nil {
		return fmt.Errorf("failed to open netns %q: %v", args.Netns, err)
	}
	defer netns.Close()

	// run the IPAM plugin and get back the config to apply
	r, err := ipam.ExecAdd(n.IPAM.Type, args.StdinData)
	if err != nil {
		return err
	}

	// release IP in case of failure
	defer func(ok *bool) {
		if !*ok {
			os.Setenv("CNI_COMMAND", "DEL")
			ipam.ExecDel(n.IPAM.Type, args.StdinData)
			os.Setenv("CNI_COMMAND", "ADD")
		}
	}(&success)

	// Convert whatever the IPAM result was into the current Result type
	result, err := current.NewResultFromResult(r)
	if err != nil {
		return err
	}

	if len(result.IPs) == 0 {
		return errors.New("No Ips returned from IPAM")
	}

	var ip net.IP
	for _, ip_i := range result.IPs {
		if ip_i.Version == "4" {
			log.Debugf("Got %s ipv4", ip_i.Address.IP)
			ip = ip_i.Address.IP
		}

		if ip_i.Interface == nil {
			log.Debugf("Skipping")
			continue
		}
	}

	virtualIP, _ := calculateVirtualIP(ip, n.VClusterCIDR)

	hostInterface, containerInterface, err := setupVeth(netns, args.IfName, n.MTU, args.ContainerID)
	if err != nil {
		return err
	}
	result.Interfaces = []*current.Interface{hostInterface, containerInterface}

	// create port on k8switch
	portName := args.ContainerID[0:10] // take just first digits of the id
	k8switchPort := k8switch.Ports{Name: portName}
	if _, err := k8switchAPI.CreateK8switchPortsByID("k8switch0", portName, k8switchPort); err != nil {
		return fmt.Errorf("Error creating port in k8switch: %s", err)
	}
	if err = createFirewallInBetween(hostInterface.Name, "k8switch0:"+portName, ip.String()); err != nil {
		log.Errorln("Could not create the firewall!")
	}

	var mac net.HardwareAddr

	gwLink, err := netlink.LinkByName(polycubeK8sInterface)
	if err != nil {
		fmt.Errorf("failed to lookup %s: %v", polycubeK8sInterface, err)
	}

	// Configure the container hardware address and IP address(es)
	if err := netns.Do(func(_ ns.NetNS) error {
		// configure interface "by hand" because ipam.ConfigureIface does not do
		// what we want to
		link, err := netlink.LinkByName(args.IfName)
		if err != nil {
			return fmt.Errorf("failed to lookup %q: %v", args.IfName, err)
		}

		mac = link.Attrs().HardwareAddr

		if err := netlink.LinkSetUp(link); err != nil {
			return fmt.Errorf("failed to set %q UP: %v", args.IfName, err)
		}

		// add ip with a /32 mask
		ipnet := net.IPNet{IP: ip, Mask: net.IPv4Mask(0xFF, 0xFF, 0xFF, 0xFF)}
		addr := &netlink.Addr{IPNet: &ipnet, Label: ""}
		if err = netlink.AddrAdd(link, addr); err != nil {
			return fmt.Errorf("Error configuring iface")
		}
		gw := dupIP(ip)
		gw = gw.To4()
		gw[3] = 0x01
		gwNet := net.IPNet{IP: gw, Mask: net.IPv4Mask(0xFF, 0xFF, 0xFF, 0xFF)}
		// add link local route to reach gw
		gwRoute := netlink.Route{
			Dst:       &gwNet,
			Scope:     unix.RT_SCOPE_LINK,
			LinkIndex: link.Attrs().Index}
		if err := netlink.RouteAdd(&gwRoute); err != nil {
			log.Error("error adding routing table to srciprewritten")
			return err
		}

		defaultRoute := netlink.Route{
			Dst: nil,
			Gw:  gw,
		}
		if err := netlink.RouteAdd(&defaultRoute); err != nil {
			log.Error("error adding routing table to srciprewritten")
			return err
		}

		// add arp entry to reach gateway
		arpentry := netlink.Neigh{
			LinkIndex:    link.Attrs().Index,
			State:        netlink.NUD_PERMANENT,
			IP:           gw,
			HardwareAddr: gwLink.Attrs().HardwareAddr,
		}

		if err := netlink.NeighAdd(&arpentry); err != nil {
			log.Error("Error adding static arp entry")
			return err
		}
		return nil
	}); err != nil {
		return err
	}

	// populate fwd table in k8switch
	fwdEntry := k8switch.FwdTable{
		Address: ip.String(),
		Mac:     mac.String(),
		Port:    portName}
	if _, err := k8switchAPI.CreateK8switchFwdTableByID("k8switch0",
		fwdEntry.Address, fwdEntry); err != nil {
		return fmt.Errorf("Error creating fwdEntry entry %s", err)
	}

	// add arp entry to reach pod
	podarp := netlink.Neigh{
		LinkIndex:    gwLink.Attrs().Index,
		State:        netlink.NUD_PERMANENT,
		IP:           ip,
		HardwareAddr: mac,
	}
	if err := netlink.NeighAdd(&podarp); err != nil {
		log.Error("Error adding static arp entry for pod")
		return err
	}

	// add arp entry to reach virtual IP (for source rewritten)
	virtualarp := netlink.Neigh{
		LinkIndex:    gwLink.Attrs().Index,
		State:        netlink.NUD_PERMANENT,
		IP:           virtualIP,
		HardwareAddr: mac,
	}
	if err := netlink.NeighAdd(&virtualarp); err != nil {
		log.Error("Error adding static arp entry for virtualarp")
		return err
	}

	if err = createFirewallInBetween(hostInterface.Name, portName, ip.String()); err != nil {
		//if err = createFirewallInBetween(containerInterface.Name, portName, ip.String()); err != nil {
		log.Error("Could not create the firewall!")
	}

	success = true
	return types.PrintResult(result, cniVersion)
}

func createFirewallInBetween(containerPort, switchPort, ip string) error {
	//	At this point we don't have the UUID of the pod.
	//	So, the only way we have to create the firewall and to be able to reference it later, is by using its IP.
	//	So its name will be fw-ip

	//name := "temp-fw-" + ip
	name := "fw-" + ip

	//	First create the firewall
	if response, err := fwAPI.CreateFirewallByID(nil, name, k8sfirewall.Firewall{
		Name: name,
	}); err != nil {
		log.Errorln("An error occurred while trying to create firewall with name:", name, err, response)
		return err
	}

	//	Now create the ports
	//	Connect the firewall with the pod
	if response, err := fwAPI.CreateFirewallPortsByID(nil, name, "ingress-p", k8sfirewall.Ports{
		Name:   "ingress-p",
		Peer:   containerPort,
		Status: "up",
	}); err != nil {
		log.Errorln("An error occurred while trying to create ports for firewall:", name, containerPort, err, response)
		return err
	}

	//	Connect the firewall with the switch
	if response, err := fwAPI.CreateFirewallPortsByID(nil, name, "egress-p", k8sfirewall.Ports{
		Name:   "egress-p",
		Peer:   switchPort,
		Status: "up",
	}); err != nil {
		log.Errorln("An error occurred while trying to create ports for firewall:", name, switchPort, err, response)
		return err
	}

	ports := strings.Split(switchPort, ":")
	//	try to set it to up
	response, err := k8switchAPI.UpdateK8switchPortsByID("k8switch0", ports[1], k8switch.Ports{Status: "UP"})
	if err != nil {
		log.Errorln("could not set port to up")
	}

	return nil
}

func cmdDel(args *skel.CmdArgs) error {
	log.Debugf("polycube cmdDel for %s", args.ContainerID)
	n, _, err := loadNetConf(args.StdinData)
	if err != nil {
		return err
	}

	if args.Netns == "" {
		return nil
	}

	netns, err := ns.GetNS(args.Netns)
	if err != nil {
		log.Errorf("failed to open netns %q: %v", args.Netns, err)
	}
	defer netns.Close()

	var ip net.IP

	portName := args.ContainerID[0:10]

	if err := netns.Do(func(_ ns.NetNS) error {
		iface, err := netlink.LinkByName(args.IfName)
		if err != nil {
			return err
		}

		addr, err := netlink.AddrList(iface, netlink.FAMILY_V4)
		if err != nil {
			return err
		}

		if len(addr) > 0 {
			ip = addr[0].IP
		}
		return nil
	}); err != nil {
		log.Errorf("Remove entries  %s", err)
	}

	if err := ipam.ExecDel(n.IPAM.Type, args.StdinData); err != nil {
		log.Errorf("ExecDel %s", err)
	}

	if len(ip) == 0 {
		return nil
	}

	// remove fwd table in load balancer
	if _, err := k8switchAPI.DeleteK8switchFwdTableByID("k8switch0", ip.String()); err != nil {
		log.Errorf("Error removing fwdEntry entry %s", err)
	}

	gwLink, err := netlink.LinkByName(polycubeK8sInterface)
	if err != nil {
		fmt.Errorf("failed to lookup %s %v", polycubeK8sInterface, err)
	}

	// remove static arp entry for pod
	podarp := netlink.Neigh{
		LinkIndex: gwLink.Attrs().Index,
		State:     netlink.NUD_PERMANENT,
		IP:        ip,
	}
	if err := netlink.NeighDel(&podarp); err != nil {
		log.Error("Error deleting static arp entry for pod")
	}

	// remove static entry for virtual source IP
	virtualIP, _ := calculateVirtualIP(ip, n.VClusterCIDR)

	virtualarp := netlink.Neigh{
		LinkIndex: gwLink.Attrs().Index,
		State:     netlink.NUD_PERMANENT,
		IP:        virtualIP,
	}
	if err := netlink.NeighDel(&virtualarp); err != nil {
		log.Error("Error deleting static arp entry for virtualarp")
	}

	_, err = k8switchAPI.DeleteK8switchPortsByID("k8switch0", portName)
	return err
}

func main() {
	skel.PluginMain(cmdAdd, cmdDel, version.All)
}
