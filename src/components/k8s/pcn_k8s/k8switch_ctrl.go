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
	k8switch "github.com/polycube-network/polycube/src/components/k8s/utils/k8switch"

	log "github.com/sirupsen/logrus"
)

func addNodeService(service service) error {
	for _, port := range service.Ports {
		if err := addNodeServicePort(service, port); err != nil {
			return err
		}
	}

	return nil
}

func delNodeService(service service) error {
	for _, port := range service.Ports {
		if err := delNodeServicePort(service, port); err != nil {
			// try to remove everything without giving errors
			//return err
		}
	}

	return nil
}

func addNodeServicePort(service service, port servicePort) error {
	var backends []k8switch.ServiceBackend
	for i := range port.Backends {
		backends = append(backends, k8switch.ServiceBackend{Ip: i.IP, Port: i.Port})
	}

	s := k8switch.Service{Name: service.Name,
		Vip:     service.VIP,
		Vport:   port.Port,
		Proto:   port.Proto,
		Backend: backends}
	// try to remove possible already existing versions of the service
	delNodeServicePort(service, port)
	_, err := k8switchAPI.CreateK8switchServiceByID(k8switchName, s.Vip, s.Vport, s.Proto, s)
	if err != nil {
		return err
	}

	if service.Type == "NodePort" {
		if service.ExternalTrafficPolicy == "Local" {
			log.Warnf("Service %s has external traffic policy %s, defaulting to kube-proxy",
				service.Name, service.ExternalTrafficPolicy )
			return nil
		}

		s := k8switch.Service{Name: service.Name,
			Vip:     nodeIP,
			Vport:   port.Nodeport,
			Proto:   port.Proto,
			Backend: backends}
		_, err := k8switchAPI.CreateK8switchServiceByID(k8switchName, s.Vip, s.Vport, s.Proto, s)
		if err != nil {
			return err
		}

	}

	return nil
}

func delNodeServicePort(service service, port servicePort) error {
	_, err := k8switchAPI.DeleteK8switchServiceByID(k8switchName, service.VIP,
		port.Port, port.Proto)
	if err != nil {
		return err
	}

	if service.Type == "NodePort" {
		if service.ExternalTrafficPolicy == "Local" {
			return nil
		}
		_, err := k8switchAPI.DeleteK8switchServiceByID(k8switchName, nodeIP,
			port.Nodeport, port.Proto)
		if err != nil {
			return err
		}
	}

	return nil
}

func addNodeServicePortBackend(service service, port servicePort, bck backend) error {
	backend := k8switch.ServiceBackend{Ip: bck.IP, Port: bck.Port}
	_, err := k8switchAPI.CreateK8switchServiceBackendByID(k8switchName, service.VIP,
		port.Port, port.Proto, bck.IP, bck.Port, backend)
	if err != nil {
		return err
	}

	if service.Type == "NodePort" {
		if service.ExternalTrafficPolicy == "Local" {
			return nil
		}

		_, err := k8switchAPI.CreateK8switchServiceBackendByID(k8switchName, nodeIP,
			port.Nodeport, port.Proto, bck.IP, bck.Port, backend)
		if err != nil {
			return err
		}
	}

	return nil
}

func delNodeServicePortBackend(service service, port servicePort, bck backend) error {
	_, err := k8switchAPI.DeleteK8switchServiceBackendByID(k8switchName, service.VIP,
		port.Port, port.Proto, bck.IP, bck.Port)
	if err != nil {
		return err
	}

	if service.Type == "NodePort" {
		if service.ExternalTrafficPolicy == "Local" {
			return nil
		}
		_, err := k8switchAPI.DeleteK8switchServiceBackendByID(k8switchName, nodeIP,
			port.Nodeport, port.Proto, bck.IP, bck.Port)
		if err != nil {
			return err
		}
	}

	return nil
}
