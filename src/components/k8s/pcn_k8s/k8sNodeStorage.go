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
	"github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/node"
)

type k8sNodeStorage struct {
	nodes map[string]*k8sNode
}

func (s *k8sNodeStorage) Init() error {
	s.nodes = make(map[string]*k8sNode)
	return nil
}

func (s *k8sNodeStorage) GetNode(name string) (node.Node, bool) {
	n, ok := s.nodes[name]
	return n, ok
}

func (s *k8sNodeStorage) GetAllNodes() []node.Node {
	m := make([]node.Node, 0, len(s.nodes))
	for _, v := range s.nodes {
		m = append(m, v)
	}
	return m
}

func (s *k8sNodeStorage) AddNode(node node.Node) error {
	n, ok := node.(*k8sNode)
	if !ok {
		return errors.New("Type is not k8sNode")
	}
	s.nodes[node.GetName()] = n
	return nil
}

func (s *k8sNodeStorage) DeleteNode(name string) {
	delete(s.nodes, name)
}
