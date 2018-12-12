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

package utils

import (
	"net"

	"github.com/vishvananda/netlink"
)

// Describes the network configuration of the host
type HostNetworkConf struct {
	DefaultIface 	string
	NodeportIface	string
	PrivateMask		net.IPMask
}

// TODO: Improve, it should be used together with the k8s node api
func GetHostNetworkConf(NodeIP string) (conf HostNetworkConf, err error) {
	// get default iface
	var index = 0
	routes, _ := netlink.RouteList(nil, netlink.FAMILY_V4)
	for _, a := range routes {
		if a.Dst == nil {
			index = a.LinkIndex
			break
		}
	}

	nic, _ := netlink.LinkByIndex(index)
	defaultiface := nic.Attrs().Name
	nodeportIface := ""

	var netmask net.IPMask
	// get nodeport iface
	links, _ := netlink.LinkList()
	for _, l := range links {
		addrs, _ := netlink.AddrList(l, netlink.FAMILY_V4)
		if len(addrs) == 0 {
			continue
		}
		if addrs[0].IPNet.IP.String() == NodeIP {
			nodeportIface = l.Attrs().Name
			netmask = addrs[0].IPNet.Mask
		}
	}

	return HostNetworkConf{
		DefaultIface: 	defaultiface,
		NodeportIface: 	nodeportIface,
		PrivateMask: 	netmask,
	}, nil
}
