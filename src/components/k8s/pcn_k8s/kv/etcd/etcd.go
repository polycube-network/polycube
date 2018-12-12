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

package etcd

import (
	"context"
	"net"
	"os"
	"strconv"
	"strings"
	"time"

	"github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/node"

	"github.com/coreos/etcd/clientv3"
	"github.com/coreos/etcd/etcdserver/api/v3rpc/rpctypes"

	log "github.com/sirupsen/logrus"
)

const (
	NodePrivateMask   string = "privateMask"
	NodePublicIP      string = "publicIP"
	NodeDirectRouting string = "directRouting"
	NodeVpcMode       string = "vpcMode"
	NodeVpc           string = "vpc"

	EtcdURLDefault     string = "http://127.0.0.1:30901"
)

var (
	EtcdURL  string
)

type Etcd struct {
	storage node.NodeStorage
	lastRev int64
}

type KeyValue struct {
	key   string
	value string
}

func (e *Etcd) isVpcRelated(key string) bool {
	list := strings.Split(key, "/")
	if len(list) < 4 {
		return false
	}

	return list[3] == "vpc"
}

func (e *Etcd) IsReady() bool {
	cli, err := clientv3.New(clientv3.Config{
		Endpoints:   []string{EtcdURL},
		DialTimeout: 10 * time.Second,
	})

	if err != nil {
		return false
	}

	defer cli.Close()

	ctx, cancel := context.WithTimeout(context.Background(), 5 * time.Second)
	defer cancel()
	_, err0 := cli.Put(ctx, "", "")
	if err0 != nil {
		switch err0 {
		case rpctypes.ErrEmptyKey:
			return true
		default:
			return false
		}
	}

	return true
}

func (e *Etcd) Init(storage node.NodeStorage) error {
	EtcdURL = os.Getenv("POLYCUBE_ETCD_URL")
	if EtcdURL == "" {
		log.Warningf("POLYCUBE_ETCD_URL not found, defaulting to %s",
			EtcdURLDefault)
		EtcdURL = EtcdURLDefault
	}

	e.storage = storage
	return nil
}

func (e *Etcd) Loop() error {
	// This is just a small optmization, give 10 seconds for the server
	// to perform the configuration of the nodes
	log.Debugf("Initializing etcd...")
	time.Sleep(10 * time.Second)

	cli, err := clientv3.New(clientv3.Config{
		Endpoints:   []string{EtcdURL},
		DialTimeout: 10 * time.Second,
	})
	if err != nil {
		log.Fatal(err)
		return err
	}

	defer cli.Close()

	rch := cli.Watch(context.Background(), "/nodes", clientv3.WithPrefix(),
					 clientv3.WithRev(e.lastRev))
	for wresp := range rch {
		for _, ev := range wresp.Events {
			switch ev.Type {
			case clientv3.EventTypePut:
				e.updateKV(string(ev.Kv.Key), string(ev.Kv.Value))
			case clientv3.EventTypeDelete:
				e.deleteKV(string(ev.Kv.Key))
			}
		}
	}

	// TODO: reopen channel in case it is closed
	return nil
}

func (e *Etcd) getNode(key string) (node.Node, bool) {
	list := strings.Split(key, "/")
	if list[1] != "nodes" {
		return nil, false
	}

	if len(list) < 3 {
		return nil, false
	}

	name := list[2]

	return e.storage.GetNode(name)
}

func (e *Etcd) deleteKV(key string) {
	e.updateKV(key, "")
}

func (e *Etcd) updateKVNode(node node.Node, key string, value string) {
	list := strings.Split(key, "/")
	field := list[3]

	switch field {
	case NodePrivateMask:
		ip := net.ParseIP(value)
		if ip == nil {
			log.Errorf("%s is not a valid value for PrivateMask", value)
			return
		}

		node.SetPrivateMask(net.IPMask(ip))

	case NodePublicIP:
		var ip net.IP
		if value != "" {
			ip = net.ParseIP(value)
			if ip == nil {
				log.Errorf("%s is not a valid value for %s", value, NodePublicIP)
				return
			}
		} else {
			ips, err := net.LookupIP(node.GetName())
			if err == nil {
				ip = ips[0]
			} else {
				ip = node.GetPrivateIP()
			}
		}

		node.SetPublicIP(ip)

	case NodeDirectRouting:
		directRouting := false
		if value != "" {
			s, err := strconv.ParseBool(value)
			if err != nil {
				log.Errorf("%s is not a valid value for DirectRouting", value)
				return
			}
			directRouting = s
		}

		node.SetDirectRouting(directRouting)

	case NodeVpcMode:
		node.SetVpcMode(value)

	case NodeVpc:
		// TODO: pass other elements than list[4]
		node.SetVpc(list[4], value)
	}
}

// key is /nodes/node_name/xxx
func (e *Etcd) updateKV(key string, value string) {
	log.Debugf("updateKV -> %s : %s", key, value)

	node, ok := e.getNode(key)
	if !ok {
		log.Errorf("updateKV: node %s does not exist", key)
		return
	}
	list := strings.Split(key, "/")
	// list[0] is ""
	// list[1] is "nodes"
	// list[2] is the node mame
	// list[3] is the leaf name

	name := list[2]

	if node == nil {
		log.Warnf("Received KV for non existing %s node", name)
		return
	}

	if len(list) < 4 {
		return
	}

	e.updateKVNode(node, key, value)
}

// saves to etcd the fields that are not available on the k8s api server
func (e *Etcd) SaveNode(node node.Node) error {
	cli, err := clientv3.New(clientv3.Config{
		Endpoints:   []string{EtcdURL},
		DialTimeout: 60 * time.Second,
	})
	if err != nil {
		log.Fatal(err)
	}

	defer cli.Close()

	var base string

	base = "/nodes/" + node.GetName() + "/"

	// privateMask
	maskIP := net.IP(node.GetPrivateMask())
	if _, err := cli.Put(context.TODO(), base+NodePrivateMask, maskIP.String()); err != nil {
		//log.Errorf(err.Error())
		log.Errorf("Error saving privateMask for %s. err: %s", node.GetName(), err.Error())
		return err
	}

	return nil
}

func (e *Etcd) GetNode(node node.Node) error {
	cli, err := clientv3.New(clientv3.Config{
		Endpoints:   []string{EtcdURL},
		DialTimeout: 60 * time.Second,
	})
	if err != nil {
		log.Fatal(err)
	}

	defer cli.Close()

	// get initial snapshot
	ctx, cancel := context.WithTimeout(context.Background(), 10*time.Second)
	resp, err := cli.Get(ctx, "/nodes/" + node.GetName(), clientv3.WithPrefix())
	cancel()
	if err != nil {
		log.Fatal(err)
		return err
	}

	e.lastRev = resp.Header.Revision

	for _, kv := range resp.Kvs {
		e.updateKVNode(node, string(kv.Key), string(kv.Value))
	}

	return nil
}

func (e *Etcd) Put(key string, value string) error {
	cli, err := clientv3.New(clientv3.Config{
		Endpoints:   []string{EtcdURL},
		DialTimeout: 60 * time.Second,
	})
	if err != nil {
		log.Fatal(err)
	}

	defer cli.Close()

	if _, err := cli.Put(context.TODO(), key, value); err != nil {
		//log.Errorf(err.Error())
		log.Errorf("Error saving %s %s. err: %s", key, value, err.Error())
		return err
	}

	return nil
}
