package types

import (
	meta_v1 "k8s.io/apimachinery/pkg/apis/meta/v1"
)

type ParsedPolicy struct {
	Name                string
	ParentPolicy        ParentPolicy
	Priority            int32
	CreationTime        meta_v1.Time
	Direction           string
	PreventDirectAccess bool
	Subject             PolicySubject
	Peer                PolicyPeer
	Templates           ParsedRules
	Ports               []ProtoPort
	Action              string
}

type ParentPolicy struct {
	Name     string
	Priority int32
	Provider string
}

type PolicySubject struct {
	Query     *ObjectQuery
	IsService bool
	Namespace string
}

type PolicyPeer struct {
	IPBlock   []string
	Peer      *ObjectQuery
	Namespace *ObjectQuery
	Key       string
}

type PeerNamespace struct {
	Labels map[string]string
	Name   string
	Any    bool
}

// Protoport defines the protocol and the port
type ProtoPort struct {
	Protocol string
	SPort    int32
	DPort    int32
}
