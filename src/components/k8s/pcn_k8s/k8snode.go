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
	"errors"
	"fmt"
	"math/big"
	"net"
	"os"
	"os/exec"

	"k8s.io/api/core/v1"
	"k8s.io/apimachinery/pkg/watch"

	"github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/node"

	"github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/vpc"
	"github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/vpc/aws"

	"github.com/vishvananda/netlink"

	log "github.com/sirupsen/logrus"
)

// represents a node in the cluster
type k8sNode struct {
	Name          string
	PrivateIP     net.IP
	PrivateMask   net.IPMask
	PublicIP      net.IP
	RouterIP      *net.IPNet
	PodCIDR       *net.IPNet
	VPodCIDR      *net.IPNet
	DirectRouting bool
	VpcMode       string
	Vpc           vpc.Vpc // inteface to control the vpc

	vxlan bool // is vxlan used to reach this node
	ignoreEnsure bool
}

// node.Node interface
func (n *k8sNode) GetName() string {
	return n.Name
}

// delete any route related to the node
func (n *k8sNode) Clean() error {
	// is this myself?
	if n.GetName() == nodeName {
		if n.Vpc != nil {
			n.Vpc.Clean()
		}
		return nil
	}

	if n.vxlan {
		// remove node from vxlan node address
		if err := exec.Command("bridge", "fdb", "del", "00:00:00:00:00:00", "dev",
			vxlanInterface, "dst", n.PublicIP.String()).Run(); err != nil {
			log.Debugf("Error removing fdb entry for vxlan: %s", err.Error())
		}
	}

	podNet := n.GetPodCIDR()
	vPodNet := n.GetVPodCIDR()
	routes, _ := netlink.RouteList(nil, netlink.FAMILY_V4)

	for _, r := range routes {
		if ipNetEqual(r.Dst, podNet) || ipNetEqual(r.Dst, vPodNet) { // TODO: Compare full route
			log.Debugf("Removing existing route to %s", r.Dst.String())
			netlink.RouteDel(&r)
		}
	}

	return nil
}

func (n *k8sNode) Print() {
	var str string

	str += fmt.Sprintf("Node %s\n", n.GetName())
	str += fmt.Sprintf("-->Public IP: %s\n", n.GetPublicIP().String())
	str += fmt.Sprintf("-->Private IP: %s\n", n.GetPrivateIP().String())
	str += fmt.Sprintf("-->Private mask: %s\n", n.GetPrivateMask().String())
	str += fmt.Sprintf("-->Direct routing: %v\n", n.GetDirectRouting())
	str += fmt.Sprintf("-->Vpc mode: %s\n", n.GetVpcMode())
	str += fmt.Sprintf("-->Pod cidr: %s\n", n.GetPodCIDR().String())
	str += fmt.Sprintf("-->VPod cidr: %s\n", n.GetVPodCIDR().String())
	str += fmt.Sprintf("-->Router IP: %s\n", n.GetRouterIP().String())

	log.Debug(str)
}

func (n *k8sNode) GetPrivateIP() net.IP {
	return n.PrivateIP
}

func (n *k8sNode) SetPrivateIP(ip net.IP) error {
	if n.PrivateIP.Equal(ip) {
		return nil
	}
	return errors.New("SetPrivateIP is not implemented")
}

func (n *k8sNode) GetPrivateMask() net.IPMask {
	return n.PrivateMask
}

func (n *k8sNode) SetPrivateMask(mask net.IPMask) error {
	maskIP := net.IP(mask)
	if maskIP.Equal(net.IP(n.PrivateMask)) {
		return nil
	}

	log.Debugf("node: %s, setting private mask to %s", n.Name, maskIP.String())
	n.PrivateMask = mask
	n.ensure()
	return nil
}

func (n *k8sNode) GetPublicIP() net.IP {
	return n.PublicIP
}

func (n *k8sNode) SetPublicIP(ip net.IP) error {
	if n.PublicIP.Equal(ip) {
		return nil
	}

	log.Debugf("Updating node %s ip from %s to %s",
		n.Name, n.PublicIP.String(), ip.String())
	// add node to list of peers in vxlan vtep
	if err := exec.Command("bridge", "fdb", "append", "00:00:00:00:00:00", "dev",
		vxlanInterface, "dst", ip.String()).Run(); err != nil {
		log.Debugf(err.Error())
		return err
	}

	// remove previous node address
	if err := exec.Command("bridge", "fdb", "del", "00:00:00:00:00:00", "dev",
		vxlanInterface, "dst", n.PublicIP.String()).Run(); err != nil {
		log.Debugf(err.Error())
		return err
	}

	n.PublicIP = ip

	return nil
}

func (n *k8sNode) GetDirectRouting() bool {
	return n.DirectRouting
}

func (n *k8sNode) SetDirectRouting(direct bool) error {
	if n.DirectRouting == direct {
		return nil
	}
	log.Debugf("node: %s, setting direct routing to %t", n.Name, direct)
	n.DirectRouting = direct
	n.ensure()
	return nil
}

func (n *k8sNode) GetVpcMode() string {
	return n.VpcMode
}

func (n *k8sNode) SetVpcMode(mode string) error {
	if n.VpcMode == mode {
		return nil
	}

	log.Debugf("node: %s, setting vpc mode to %s", n.Name, mode)

	if mode == "" {
		n.Vpc.Clean()
		n.Vpc = nil // here?
	} else if mode == "aws" {
		n.Vpc = &awsvpc.AwsVpc{}
	} else {
		return errors.New("vpc mode not valid")
	}

	n.VpcMode = mode
	// if this is the node, create a routing entry to reach this node's pods
	if n.Name == nodeName && n.Vpc != nil {
		n.Vpc.Init([]*net.IPNet{n.GetPodCIDR(), n.GetVPodCIDR()}, kvM,
			"/nodes/"+nodeName+"/vpc/")
	}
	n.ensure()

	return nil
}

func (n *k8sNode) SetVpc(key string, value string) error {
	if n.Vpc == nil {
		log.Errorf("---> SetVpc")
		return errors.New("SetVpc")
	}
	n.Vpc.UpdateKV(key, value)
	n.ensure() // TODO: call here or from vpc?
	return nil
}

func (n *k8sNode) GetPodCIDR() *net.IPNet {
	return n.PodCIDR
}

func (n *k8sNode) GetVPodCIDR() *net.IPNet {
	return n.VPodCIDR
}

func (n *k8sNode) GetRouterIP() *net.IPNet {
	return n.RouterIP
}

// updates the fields from the ***k8s api server***
func (node *k8sNode) Update(new node.Node) error {
	node.SetPrivateIP(new.GetPrivateIP())
	return nil
}

// internal methods

// creates initial config based on vxlan
func (node *k8sNode) ensure() error {
	if node.ignoreEnsure {
		return nil
	}
	// I changed myself, update connection to all nodes if needed
	if node.GetName() == nodeName {
		for _, n := range nodesStorage.GetAllNodes() {
			if n.GetName() != nodeName {
				n.(*k8sNode).ensure()
			}
		}
		return nil
	}

	var gw net.IP
	useVxlan := true
	m, ok := nodesStorage.GetNode(nodeName)
	if !ok {
		return errors.New("'This' node does not exist")
	}

	mine := m.(*k8sNode)

	// check if direct routing is active in remote and local node and then
	// check if two nodes are on the same subnet
	if node.GetDirectRouting() && mine.GetDirectRouting() {
		localNet := mine.GetPrivateIP().Mask(mine.GetPrivateMask())
		remoteNet := node.GetPrivateIP().Mask(node.GetPrivateMask())
		if localNet.Equal(remoteNet) {
			log.Debugf("Enabling direct route to %s. (%s, %s)", node.Name, node.PrivateIP, node.PrivateMask)
			gw = node.PrivateIP
			useVxlan = false
		}
	} else {
		if mine.GetVpcMode() != "" && mine.GetVpcMode() == node.GetVpcMode() { // is this check needed?
			if mine.Vpc.IsCompatible(node.Vpc) {
				gw, _ = node.Vpc.GetGateway()
				log.Debugf("Using vpc to communicate to %s", node.Name)
				useVxlan = false
			}
		}
	}

	if useVxlan {
		ensureVxlan()
		gw = node.GetRouterIP().IP

		log.Debugf("Router ip for %s is %s", node.GetName(), node.GetRouterIP().IP.String())

		// add node to peers list in vxlan vtep
		if out, err := exec.Command("bridge", "fdb", "append", "00:00:00:00:00:00", "dev",
			vxlanInterface, "dst", node.PublicIP.String()).CombinedOutput(); err != nil {
			log.Debugf("error adding to vxlan fdb: %s", out)
			return err
		}
	}

	// route to reach pods on that node
	podsRoute := netlink.Route{Dst: node.GetPodCIDR(), Gw: gw}
	if err := ensureRoute(&podsRoute, useVxlan); err != nil {
		log.Errorf("error adding routing table to pods: %s", err.Error())
		return err
	}

	// route to reach rewritten source ip from that node
	vPodsRoute := netlink.Route{Dst: node.GetVPodCIDR(), Gw: gw}
	if err := ensureRoute(&vPodsRoute, useVxlan); err != nil {
		log.Errorf("error adding routing table to virtualPodCIDR: %s", err.Error())
		return err
	}

	node.vxlan = useVxlan

	return nil
}

// ipNetEqual returns true iff both IPNet are equal
func ipNetEqual(ipn1 *net.IPNet, ipn2 *net.IPNet) bool {
	if ipn1 == ipn2 {
		return true
	}
	if ipn1 == nil || ipn2 == nil {
		return false
	}
	m1, _ := ipn1.Mask.Size()
	m2, _ := ipn2.Mask.Size()
	return m1 == m2 && ipn1.IP.Equal(ipn2.IP)
}

// tries to install route on the system, if it already exists it
// does nothing
func ensureRoute(route *netlink.Route, useVxlan bool) error {
	routes, _ := netlink.RouteList(nil, netlink.FAMILY_V4)
	for _, r := range routes {
		if ipNetEqual(r.Dst, route.Dst) { // TODO: Compare full route
			if r.Gw.Equal(route.Gw) { // is the route already correct?
				return nil
			}
			log.Debugf("Removing existing route to %s", route.Dst.String())
			netlink.RouteDel(&r)
			if !useVxlan {
				cleanVxlan()
			}
			break
		}
	}

	return netlink.RouteAdd(route)
}

func ensureVxlan() error {
	link, err := netlink.LinkByName(vxlanInterface)
	if err == nil && link != nil {
		// TODO: check that the interface is good to be used
		return nil
	}

	log.Debugf("creating %s interface", vxlanInterface)

	vxlan := &netlink.Vxlan{
		LinkAttrs: netlink.LinkAttrs{
			Name: vxlanInterface,
		},
		VxlanId: 100,
		Learning: true,
		Port:     4789,
	}

	if err := netlink.LinkAdd(vxlan); err != nil {
		return fmt.Errorf("error creating vxlan %s", err)
	}

	vxlanlink, err := netlink.LinkByIndex(vxlan.Index)
	if err != nil {
		return err
	}

	if err := netlink.LinkSetUp(vxlanlink); err != nil {
		return err
	}

	node, _ := nodesStorage.GetNode(nodeName)

	vxlan_addr := &netlink.Addr{IPNet: node.GetRouterIP()}
	if err := netlink.AddrAdd(vxlanlink, vxlan_addr); err != nil {
		log.Errorf("error adding address %s", "vxlanlink")
		return err
	}

	log.Debug("vxlan set")
	return nil
}

func cleanVxlan() {
	vxlan, err := netlink.LinkByName(vxlanInterface)
	if err != nil {
		return
	}

	routes, _ := netlink.RouteList(nil, netlink.FAMILY_V4)
	for _, r := range routes {
		if vxlan.Attrs().Index == r.LinkIndex && r.Scope == netlink.SCOPE_UNIVERSE {
			return
		}
	}

	log.Debugf("removing %s interface", vxlanInterface)
	netlink.LinkDel(vxlan)
}

func addNode(node *k8sNode) error {
	node.ignoreEnsure = true
	kvM.GetNode(node)
	node.ignoreEnsure = false

	log.Debugf("Adding node %s", node.Name)
	node.Print()

	if err := node.ensure(); err != nil {
		// TODO: log
		return err
	}

	nodesStorage.AddNode(node)

	return nil
}

func deleteNode(name string) error {
	node, ok := nodesStorage.GetNode(name)
	if !ok {
		return fmt.Errorf("node %s does not exist", name)
	}

	log.Debugf("Removing node")
	node.Print()
	err := node.Clean()
	nodesStorage.DeleteNode(name)

	return err
}

func deleteNodes() {
	nodes := nodesStorage.GetAllNodes()
	for _, n := range nodes {
		n.Clean()
		nodesStorage.DeleteNode(n.GetName())
	}
}

// this is a fast method to get an unique index for each module in the cluster
func getNodeIndex(cidr net.IP) uint {
	return uint(cidr[2])
}

// Calculate the address to be used for the VTEP for this node.
// It uses the node index as third octet on the vteps range
func calculateRouterIP(index uint) *net.IPNet {
	vtepsRange := os.Getenv("POLYCUBE_VTEPS_RANGE")
	if vtepsRange == "" {
		log.Warningf("POLYCUBE_VTEPS_RANGE not found, defaulting to %s",
			vtepsRangeDefault)
		vtepsRange = vtepsRangeDefault
	}

	var err error
	var vtepsCIDR *net.IPNet
	_, vtepsCIDR, err = net.ParseCIDR(vtepsRange)
	if err != nil {
		//log.Warningf("Error parsing POLYCUBE_VTEPS_RANGE=%s, defaulting to %s",
		//	vtepsRange, vtepsRangeDefault)

		_, vtepsCIDR, _ = net.ParseCIDR(vtepsRangeDefault)
	}

	vtepsCIDR.IP = vtepsCIDR.IP.To4()
	vtepsCIDR.IP[2] = byte(index)
	vtepsCIDR.IP[3] = 1

	return vtepsCIDR
}

func calculateVPodsCDIR(podsCIDR *net.IPNet) (*net.IPNet, error) {
	vPodsRange := os.Getenv("POLYCUBE_VPODS_RANGE")
	if vPodsRange == "" {
		//log.Warningf("POLYCUBE_VPODS_RANGE not found, defaulting to %s",
		//	vPodsRangeDefault)
		vPodsRange = vPodsRangeDefault
	}

	var err error
	var vPodsClusterCDIR *net.IPNet
	_, vPodsClusterCDIR, err = net.ParseCIDR(vPodsRange)
	if err != nil {
		log.Warningf("Error parsing POLYCUBE_VPODS_RANGE=%s, defaulting to %s",
			vPodsRange, vPodsRangeDefault)

		_, vPodsClusterCDIR, _ = net.ParseCIDR(vPodsRangeDefault)
	}

	vpodsClusterInt := big.NewInt(0).SetBytes(vPodsClusterCDIR.IP.To4())
	clusterMask := big.NewInt(0).SetBytes(vPodsClusterCDIR.Mask)
	podsNodeInt := big.NewInt(0).SetBytes(podsCIDR.IP.To4())

	//vpodsClusterInt | (podsNodeInt & ~clusterMask)
	vpodsClusterInt.Or(vpodsClusterInt, big.NewInt(0).And(podsNodeInt, big.NewInt(0).Not(clusterMask)))
	ip := net.IP(vpodsClusterInt.Bytes())
	return &net.IPNet{IP: ip, Mask: podsCIDR.Mask}, nil
}

func parseNode(node *v1.Node) (*k8sNode, error) {
	_, podCIDR, err1 := net.ParseCIDR(node.Spec.PodCIDR)
	if err1 != nil {
		return nil, err1
	}

	index := getNodeIndex(podCIDR.IP)

	vPodCIDR, err := calculateVPodsCDIR(podCIDR)
	if err != nil {
		log.Errorf("Error calculating vpodscdir")
		return nil, err
	}

	routerIP := calculateRouterIP(index)

	// get adddress of the node
	var privateIP net.IP

	for _, x := range node.Status.Addresses {
		if x.Type == v1.NodeExternalIP || x.Type == v1.NodeInternalIP {
			privateIP = net.ParseIP(x.Address)
			break
		}
	}

	return &k8sNode{
		Name:          node.Name,
		PrivateIP:     privateIP,
		PublicIP:      privateIP,	// this is the initial value
		RouterIP:      routerIP,
		PodCIDR:       podCIDR,
		VPodCIDR:      vPodCIDR,
		DirectRouting: false}, nil
}

func processNode(got *watch.Event) {
	if got.Object == nil {
		//log.Debug("Watcher received nil object")
		return
	}

	v1Node := got.Object.(*v1.Node)
	typeS := fmt.Sprintf("%s", got.Type)
	switch typeS {
	case "ADDED":
		//log.Debugf("nodewacther: node %s added", endpoint.Name)
		name := v1Node.Name

		// ignore self node
		if name == nodeName {
			return
		}

		node, err := parseNode(v1Node)
		if err != nil {
			return
		}

		if _, ok := nodesStorage.GetNode(name); ok {
			return
		}
		addNode(node)

	case "DELETED":
		//log.Debugf("nodewacther: node %s deleted", endpoint.Name)
		deleteNode(v1Node.Name)

	case "MODIFIED":
		//log.Debugf("nodewacther: node %s modified", endpoint.Name)
		name := v1Node.Name
		// ignore self node
		if name == nodeName {
			return
		}

		new_node, err := parseNode(v1Node)
		if err != nil {
			return
		}

		if node, ok := nodesStorage.GetNode(name); !ok {
			addNode(new_node)
		} else {
			node.Update(new_node)
		}
	}
}
