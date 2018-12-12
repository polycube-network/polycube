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
	"fmt"
	"net"
	"os"

	"github.com/containernetworking/plugins/pkg/ip"
	metav1 "k8s.io/apimachinery/pkg/apis/meta/v1"
	"k8s.io/client-go/kubernetes"
	"k8s.io/client-go/tools/clientcmd"

	log "github.com/sirupsen/logrus"
)

const confFormat = `
{
	"cniVersion": "0.2.0",
	"name": "mynet",
	"type": "polycube",
	"mtu": %s,
	"vclustercdir": "%s",
	"ipam": {
		"type": "host-local",
		"subnet": "%s",
		"rangeStart": "%s",
		"routes": [
			{ "dst": "0.0.0.0/0" }
		]
	}
}
`

func checkError(where string, err error) {
	if err != nil {
		log.Errorf(where + " " + err.Error())
		panic(where + " " + err.Error())
	}
}

// .1 is used for the gateway
// .2 is used for the NAT
func calcRangeStart(ipn *net.IPNet) net.IP {
	nid := ipn.IP.Mask(ipn.Mask)

	for i := 0; i < 3; i++ {
		nid = ip.NextIP(nid)
	}
	return nid
}

func main() {
	log.SetLevel(log.DebugLevel)
	if len(os.Args) != 2 {
		panic("usage: %d polycubeConfpath")
	}

	kubeconfig := "/var/lib/pcn_k8s/kubeconfig.conf"

	// use the current context in kubeconfig
	config, err := clientcmd.BuildConfigFromFlags("", kubeconfig)
	if err != nil {
		panic(err.Error())
	}

	//// creates the in-cluster config
	//config, err := rest.InClusterConfig()
	//checkError("create k8s client", err)

	// creates the clientset
	clientset, err := kubernetes.NewForConfig(config)
	checkError("create config", err)

	nodeName := os.Getenv("K8S_NODE_NAME")
	if nodeName == "" {
		panic("K8S_NODE_NAME env variable not found")
	}

	var mtu string
	mtu = os.Getenv("POLYCUBE_MTU")
	if mtu == "" {
		log.Warning("POLYCUBE_MTU env variable not found")
		mtu = "1450"
	}

	var vClusterCIDR string
	vClusterCIDR = os.Getenv("POLYCUBE_VPODS_RANGE")
	if vClusterCIDR == "" {
		log.Warning("POLYCUBE_VPODS_RANGE env variable not found")
		vClusterCIDR = "10.10.0.0/16"
	}

	node, err := clientset.CoreV1().Nodes().Get(nodeName, metav1.GetOptions{})
	checkError("get node", err)

	_, podcidr, _ := net.ParseCIDR(node.Spec.PodCIDR)
	rangeStart := calcRangeStart(podcidr)

	f, err := os.Create(os.Args[1])
	checkError("create cni config file", err)
	defer f.Close()

	fmt.Fprintf(f,
		confFormat,
		mtu,
		vClusterCIDR,
		node.Spec.PodCIDR,
		rangeStart.String())
}
