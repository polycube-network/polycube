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
	"net/http"
	"os"
	"os/signal"
	"runtime"
	"syscall"
	"time"

	k8sfilter "github.com/polycube-network/polycube/src/components/k8s/utils/k8sfilter"
	k8switch "github.com/polycube-network/polycube/src/components/k8s/utils/k8switch"

	"github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/kv/etcd"

	//	importing controllers
	//	TODO-ON-MERGE: change the path here to polycube-network
	//"github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/controllers"
	pcn_controllers "github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/controllers"
	networkpolicies "github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/networkpolicies"

	//"github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/controllers"
	//"github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/networkpolicies"

	metav1 "k8s.io/apimachinery/pkg/apis/meta/v1"
	"k8s.io/apimachinery/pkg/types"
	"k8s.io/apimachinery/pkg/watch"
	"k8s.io/client-go/kubernetes"

	//"k8s.io/client-go/rest"
	"k8s.io/client-go/tools/clientcmd"

	log "github.com/sirupsen/logrus"
)

const (
	basePath             = "http://127.0.0.1:9000/polycube/v1"
	vxlanInterface       = "pcn_vxlan"
	stackInterface       = "pcn_stack"
	routerInterface      = "pcn_router"
	polycubeK8sInterface = "pcn_k8s"
	polycubeLBInterface  = "pcn_lb"
	k8switchName         = "k8switch0"

	vPodsRangeDefault            = "10.10.0.0/16"
	vtepsRangeDefault            = "10.18.0.0/16"
	serviceClusterIPRangeDefault = "10.96.0.0/12"
	serviceNodePortRangeDefault  = "30000-32767"
)

var (
	// node where this instance is running
	nodeName string

	// connection to the k8s apif
	clientset *kubernetes.Clientset

	k8switchAPI  *k8switch.K8switchApiService
	k8sfilterAPI *k8sfilter.K8sfilterApiService

	nodeIP string

	nodesStorage k8sNodeStorage
	kvM          *etcd.Etcd

	endpointsWatcher watch.Interface
	nodesWatcher     watch.Interface

	//	--- Controllers
	defaultnpc    *pcn_controllers.DefaultNetworkPolicyController
	podController pcn_controllers.PodController
	nsController  pcn_controllers.NamespaceController
	//	--- /Controllers

	networkPolicyManager networkpolicies.PcnNetworkPolicyManager

	stop bool
)

func cleanup() {
	log.Debugf("Stopping...")
	stop = true
	if nodesWatcher != nil {
		nodesWatcher.Stop()
	}

	if endpointsWatcher != nil {
		endpointsWatcher.Stop()
	}

	if defaultnpc != nil {
		defaultnpc.Stop()
	}
	if podController != nil {
		podController.Stop()
	}
	if nsController != nil {
		nsController.Stop()
	}
}

func main() {
	stop = false

	kvM = &etcd.Etcd{}

	// Lock the OS Thread so we don't accidentally switch namespaces
	runtime.LockOSThread()
	defer runtime.UnlockOSThread()

	log.SetLevel(log.DebugLevel)
	log.Info("Starting...")

	//  connection to k8switch
	cfgK8switch := k8switch.Configuration{BasePath: basePath}
	srK8switch := k8switch.NewAPIClient(&cfgK8switch)
	k8switchAPI = srK8switch.K8switchApi

	// connection to k8s filter
	cfgK8sfilter := k8sfilter.Configuration{BasePath: basePath}
	srK8sfilter := k8sfilter.NewAPIClient(&cfgK8sfilter)
	k8sfilterAPI = srK8sfilter.K8sfilterApi

	kubeconfig := "/var/lib/pcn_k8s/kubeconfig.conf"

	// use the current context in kubeconfig
	config, err := clientcmd.BuildConfigFromFlags("", kubeconfig)
	if err != nil {
		panic(err.Error())
	}

	// creates the in-cluster config
	//config, err := rest.InClusterConfig()
	//if err != nil {
	//	panic(err.Error())
	//}
	// creates the clientset
	var err1 error
	clientset, err1 = kubernetes.NewForConfig(config)
	if err1 != nil {
		panic(err1.Error())
	}

	nodeName = os.Getenv("K8S_NODE_NAME")
	if nodeName == "" {
		panic("K8S_NODE_NAME env variable not found")
	}

	log.Infof("Running in node: %s", nodeName)

	services = make(map[types.UID]service)

	nodesStorage.Init()
	kvM.Init(&nodesStorage)

	// wait until polycubed is ready
	// TODO: implement backoff
	i := 0
	for i = 0; i < 30; {
		if _, err := http.Get("http://127.0.0.1:9000"); err == nil {
			log.Debug("polycubed is ready")
			break
		}
		log.Debug("Waiting for polycubed")
		time.Sleep(5 * time.Second)
	}

	if i == 30 {
		log.Error("error contacting polycubed")
		return
	}

	// wait until etcd is ready
	i = 0
	for i = 0; i < 30; {
		if kvM.IsReady() {
			log.Debug("etcd is ready")
			break
		}

		log.Debug("Waiting for etcd")
		time.Sleep(5 * time.Second)
	}

	if i == 30 {
		log.Error("error contacting etcd")
		return
	}

	node, err := clientset.CoreV1().Nodes().Get(nodeName, metav1.GetOptions{})
	if err != nil {
		panic(err.Error())
	}

	// insert myself as a node
	k8sNode, err := parseNode(node)
	if err != nil {
		panic(err.Error())
	}
	nodesStorage.AddNode(k8sNode)

	http_resp, http_err := http.Get("http://127.0.0.1:9000/polycube/v1/" + k8switchName)
	if http_err != nil || http_resp.StatusCode != 200 {
		log.Debug("configuring polycube")

		// read configuration for this node
		kvM.GetNode(k8sNode)
		k8sNode.Print()

		if err := k8sNode.Init(); err != nil {
			panic(err.Error())
		}

		if err := kvM.SaveNode(k8sNode); err != nil {
			log.Error("Error saving node")
		}

		c := make(chan os.Signal)
		signal.Notify(c, os.Interrupt, syscall.SIGTERM)
		go func() {
			<-c
			cleanup()
		}()
	}

	log.Debugf("init main loop")
	var err0 error
	// create watcher for endpoints
	endpointsWatcher, err0 = clientset.CoreV1().Endpoints("").Watch(metav1.ListOptions{})
	if err0 != nil {
		panic(err0.Error())
	}
	// 	 create watcher for nodes
	nodesWatcher, err0 = clientset.CoreV1().Nodes().Watch(metav1.ListOptions{})
	if err0 != nil {
		panic(err0.Error())
	}

	//	Set up the network policy controller (for the kubernetes policies)
	defaultnpc = pcn_controllers.NewDefaultNetworkPolicyController(nodeName, clientset)

	//	Get the namespace controller
	nsController = pcn_controllers.NewNsController(nodeName, clientset)

	//	Get the pod controller
	podController = pcn_controllers.NewPodController(nodeName, clientset, nsController)

	// kv handler
	go kvM.Loop()

	//	Start the default network policy controller
	go defaultnpc.Run()

	//	Start the namespace controller
	go nsController.Run()

	//	Start the pod controller
	go podController.Run()

	//	Start the policy manager
	networkPolicyManager = networkpolicies.StartNetworkPolicyManager(defaultnpc, podController, nodeName)

	// read and process all notifications for both, pods and enpoints
	// Notice that a notification is processed at the time, so
	// no synchronization is needed.
	//handleLoop:
	for !stop {
		select {
		case got, ok := <-endpointsWatcher.ResultChan():
			if !ok && !stop {
				log.Warn("endpoints channel was closed")
				var err error
				endpointsWatcher, err = clientset.CoreV1().Endpoints("").Watch(metav1.ListOptions{})
				if err != nil {
					panic(err.Error())
				}
				break
				//break handleLoop
			}
			processEndpoint(&got)
		case got, ok := <-nodesWatcher.ResultChan():
			if !ok && !stop {
				log.Warn("nodes channel was closed")
				var err error
				nodesWatcher, err = clientset.CoreV1().Nodes().Watch(metav1.ListOptions{})
				if err != nil {
					panic(err.Error())
				}
				break
			}
			processNode(&got)
		}
	}

	deleteNodes()
	k8sNode.Uninit()
	defaultnpc.Stop()
	nsController.Stop()
	podController.Stop()
	log.Debugf("Bye bye")
}
