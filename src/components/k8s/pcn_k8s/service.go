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

	v1 "k8s.io/api/core/v1"
	metav1 "k8s.io/apimachinery/pkg/apis/meta/v1"
	"k8s.io/apimachinery/pkg/types"
	"k8s.io/apimachinery/pkg/watch"

	log "github.com/sirupsen/logrus"
)

type backend struct {
	IP   string
	Port int32
}

type servicePortKey struct {
	Port  int32
	Proto string
}

type servicePort struct {
	Port     int32
	Proto    string
	Name     string
	Nodeport int32
	Backends map[backend]bool // backends implementing this port
}

type service struct {
	UID   types.UID
	Name  string
	Type  string
	VIP   string
	ExternalTrafficPolicy string
	Ports map[servicePortKey]servicePort // different ports exposed by the service
}

var (
	// all services that are active in the cluster
	services map[types.UID]service
)

/**** services ****/
func printService(entry service) {
	log.Debugf("uid: %s", entry.UID)
	log.Debugf("name: %s", entry.Name)
	log.Debugf("vip: %s", entry.VIP)
	log.Debugf("ports: ")
	for _, y := range entry.Ports {
		log.Debugf("--%s, %d, %s, %d", y.Name, y.Port, y.Proto, y.Nodeport)
		log.Debugf("backends: ")
		for x := range y.Backends {
			log.Debugf("--%s:%d", x.IP, x.Port)
		}
	}
}

func parseService(endpoint *v1.Endpoints) (service, error) {
	uid := endpoint.UID
	name := endpoint.Name
	namespace := endpoint.Namespace
	svc, err := clientset.CoreV1().Services(namespace).Get(name, metav1.GetOptions{})
	if err != nil {
		return service{}, err
	}

	if svc.Spec.Type != v1.ServiceTypeClusterIP &&
		svc.Spec.Type != v1.ServiceTypeNodePort {
		return service{}, errors.New("Service type is not supported")
	}

	type_ := svc.Spec.Type
	vip := svc.Spec.ClusterIP
	externalTrafficPolicy := svc.Spec.ExternalTrafficPolicy

	servicePorts := make(map[servicePortKey]servicePort)
	// parse different ports
	for _, port := range svc.Spec.Ports {
		port_ := servicePort{Name: port.Name,
			Port:     port.Port,
			Nodeport: port.NodePort,
			Proto:    string(port.Protocol)}

		port_.Backends = make(map[backend]bool)

		addresses := make(map[string]bool)
		endpoindPorts := make(map[int32]bool)

		// get endpoints that implement this port
		for _, subset := range endpoint.Subsets {
			// get IP addresses
			for _, addr := range subset.Addresses {
				addresses[addr.IP] = true
			}

			// get ports
			for _, endpointPort := range subset.Ports {
				if port.Name == "" || port.Name == endpointPort.Name {
					endpoindPorts[endpointPort.Port] = true
				}
			}
		}

		// perform address x ports (cartesian product)
		for address := range addresses {
			for endpointPort := range endpoindPorts {
				port_.Backends[backend{address, endpointPort}] = true
			}
		}

		servicePorts[servicePortKey{port_.Port, port_.Proto}] = port_
	}

	return service{
		UID:   uid,
		Name:  name,
		Type:  string(type_),
		VIP:   vip,
		ExternalTrafficPolicy: string(externalTrafficPolicy),
		Ports: servicePorts}, nil
}

func addService(endpoint *v1.Endpoints) {
	uid := endpoint.UID
	// in case the service is already present, update it.
	if _, ok := services[uid]; ok {
		updateService(endpoint)
		return
	}

	service, err := parseService(endpoint)
	if err != nil {
		//log.Debugf("Error parsing service: %s", err.Error())
		return
	}
	services[service.UID] = service
	log.Debugf("Adding service %s", endpoint.Name)
	printService(service)

	addNodeService(service)
}

func deleteService(endpoint *v1.Endpoints) {
	log.Debugf("Deleting service %s", endpoint.Name)

	service := services[endpoint.UID]

	delNodeService(service)

	delete(services, endpoint.UID)
}

// the following cases are considered:
// - a port is added or removed
// - a backend (endpoint) for a port is added or removed
func updateService(endpoint *v1.Endpoints) {
	uid := endpoint.UID
	// in case the service is not present in our db, add it
	if _, ok := services[uid]; !ok {
		addService(endpoint)
		return
	}

	log.Debugf("updating service %s", endpoint.Name)
	serviceOld := services[uid]
	serviceNew, err := parseService(endpoint)
	if err != nil {
		return
	}

	// get list of added and removed ports
	deletedPorts, addedPorts := getServicePortsDiff(serviceOld, serviceNew)
	if len(deletedPorts) > 0 {
		log.Debugf("ports deleted:")
	}
	for _, i := range deletedPorts {
		log.Debugf("--%d:%s", i.Port, i.Proto)
		delNodeServicePort(serviceNew, i)
	}

	if len(addedPorts) > 0 {
		log.Debugf("ports added:")
	}
	for _, i := range addedPorts {
		log.Debugf("--%d:%s", i.Port, i.Proto)
		addNodeServicePort(serviceNew, i)
	}

	// for all ports, get list update backends
	for portOldKey, portOldValue := range serviceOld.Ports {
		portNewValue, ok := serviceNew.Ports[portOldKey]
		if !ok {
			continue
		}
		deletedBackends, addedBackends := getServicePortBackendsDiff(portOldValue, portNewValue)
		if len(deletedBackends) > 0 {
			log.Debugf("addressed deleted:")
		}
		for _, i := range deletedBackends {
			log.Debugf("--%s:%d", i.IP, i.Port)
			delNodeServicePortBackend(serviceNew, portOldValue, i)
		}

		if len(addedBackends) > 0 {
			log.Debugf("addressed added:")
		}
		for _, i := range addedBackends {
			log.Debugf("--%s:%d", i.IP, i.Port)
			addNodeServicePortBackend(serviceNew, portOldValue, i)
		}
	}

	services[uid] = serviceNew
}

func getServicePortsDiff(s1 service, s2 service) ([]servicePort, []servicePort) {
	deleted := []servicePort{}
	added := []servicePort{}

	// elements in s1 not present in s2 (deleted)
	for k, v := range s1.Ports {
		if _, ok := s2.Ports[k]; !ok {
			deleted = append(deleted, v)
		}
	}

	// elements in s2 not present in s1 (added)
	for k, v := range s2.Ports {
		if _, ok := s1.Ports[k]; !ok {
			added = append(added, v)
		}
	}

	return deleted, added
}

// returns deleted, added backends
// WARNING: Only addresses are considered
func getServicePortBackendsDiff(p1 servicePort, p2 servicePort) ([]backend, []backend) {
	deleted := []backend{}
	added := []backend{}

	// elements in s1 not present in s2 (deleted)
	for i := range p1.Backends {
		if _, ok := p2.Backends[i]; !ok {
			deleted = append(deleted, i)
		}
	}

	// elements in s2 not present in s1 (added)
	for i := range p2.Backends {
		if _, ok := p1.Backends[i]; !ok {
			added = append(added, i)
		}
	}

	return deleted, added
}

func processEndpoint(got *watch.Event) {
	if got.Type == watch.Error {
		log.Error("process endpoint received error")
		return
	}

	if got.Object == nil {
		log.Debug("endpoint watcher received nil object")
		return
	}
	endpoint := got.Object.(*v1.Endpoints)

	switch got.Type {
	case watch.Added:
		//log.Debugf("endpointwatcher: endpoint %s added", endpoint.Name)
		addService(endpoint)

	case watch.Deleted:
		//log.Debugf("endpointwatcher: endpoint %s deleted", endpoint.Name)
		deleteService(endpoint)

	case watch.Modified:
		//log.Debugf("endpointwatcher: endpoint %s modified", endpoint.Name)
		updateService(endpoint)
	}
}
