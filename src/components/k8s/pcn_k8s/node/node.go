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

package node

import (
	"net"
)

// represents a node in the cluster
type Node interface {
	GetName() string

	Print()
	Clean()	error // clean environment

	GetPrivateIP() net.IP
	SetPrivateIP(ip net.IP) error

	GetPrivateMask() net.IPMask
	SetPrivateMask(mask net.IPMask) error

	GetPublicIP() net.IP
	SetPublicIP(ip net.IP) error

	GetDirectRouting() bool
	SetDirectRouting(direct bool) error

	GetVpcMode() string
	SetVpcMode(mode string) error

	SetVpc(key string, value string) error

	Update(new Node) error

	GetPodCIDR() *net.IPNet
	GetVPodCIDR() *net.IPNet

	GetRouterIP() *net.IPNet	// this is vxlan specific, generalize?
}

type NodeStorage interface {
	Init() error
	GetNode(name string) (Node, bool)
	GetAllNodes() []Node
	AddNode(node Node) error
	DeleteNode(name string)
}
