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
	"math/big"
	"net"
	"sort"
	"strings"

	pcn_types "github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/types"
	log "github.com/sirupsen/logrus"
	"github.com/vishvananda/netlink"
)

var (
	vPodsRange *net.IPNet
)

// Describes the network configuration of the host
type HostNetworkConf struct {
	DefaultIface  string
	NodeportIface string
	PrivateMask   net.IPMask
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
		DefaultIface:  defaultiface,
		NodeportIface: nodeportIface,
		PrivateMask:   netmask,
	}, nil
}

func SetVPodsRange(vrange *net.IPNet) {
	vPodsRange = vrange
}

// BuildQuery builds a query to be sent to a controller
func BuildQuery(name string, labels map[string]string) *pcn_types.ObjectQuery {
	// All?
	if len(name) == 0 && len(labels) == 0 {
		return nil
	}

	query := &pcn_types.ObjectQuery{}

	// Name?
	if len(name) > 0 {
		query.By = "name"
		query.Name = name
	} else {
		// Labels
		query.By = "labels"
		query.Labels = labels
	}

	return query
}

// GetPodVirtualIP gets a pod's virtual IP starting from the IP assigned to it
func GetPodVirtualIP(ip string) string {
	l := log.New().WithFields(log.Fields{"by": "utils", "method": "GetPodVirtualIP"})

	// transform the virtual pod ip range in bytes
	vpodsClusterInt := big.NewInt(0).SetBytes(vPodsRange.IP.To4())
	clusterMask := big.NewInt(0).SetBytes(vPodsRange.Mask)

	// transform the actual pod ip in bytes
	_, podsCIDR, err := net.ParseCIDR(ip + "/32")
	if err != nil {
		l.Errorf("Could not get virtual ip from %s: %s. Going to return actual IP instead.", ip, err)
		return ip
	}
	podsNodeInt := big.NewInt(0).SetBytes(podsCIDR.IP.To4())

	// OR it
	vpodsClusterInt.Or(vpodsClusterInt, big.NewInt(0).And(podsNodeInt, big.NewInt(0).Not(clusterMask)))
	return net.IP(vpodsClusterInt.Bytes()).String()
}

// ImplodeLabels set labels in a key1=value1,key2=value2 format
// Values are separated by sep and the third value specifies if keys should be
// sorted.
func ImplodeLabels(labels map[string]string, sep string, sortKeys bool) string {
	// Create an array of imploded labels
	// (e.g.: [app=mysql version=2.5 beta=no])
	implodedLabels := []string{}
	for k, v := range labels {
		implodedLabels = append(implodedLabels, k+"="+v)
	}

	// Now we sort the labels. Why do we sort them? Because maps in go do
	// not preserve an alphabetical order. As per documentation, order is not fixed.
	// Two pods may have the exact same labels, but the iteration order in them
	// may differ. So, by sorting them alphabetically we're making them equal.
	if sortKeys {
		sort.Strings(implodedLabels)
	}

	// Join the key and the labels
	return strings.Join(implodedLabels, sep)
}

// AreLabelsContained checks if the labels in the first argument are present
// in the second one and values are the same.
// Basically checks if a map is a sub-map.
func AreLabelsContained(needle map[string]string, haystack map[string]string) bool {
	// Are both empty?
	if len(needle) == 0 && len(haystack) == 0 {
		return true
	}

	if len(needle) == 0 && len(haystack) > 0 {
		return false
	}

	if len(needle) > 0 && len(haystack) == 0 {
		return false
	}

	// Check the maps!
	for key, value := range needle {
		if hValue, exists := haystack[key]; !exists || hValue != value {
			// One wrong/absent label is enough
			return false
		}
	}

	return true
}
