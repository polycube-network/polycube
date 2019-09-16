package types

import (
	k8sfirewall "github.com/polycube-network/polycube/src/components/k8s/utils/k8sfirewall"
)

const (
	ActionForward        = "forward"
	ActionDrop           = "drop"
	ConnTrackNew         = "new"
	ConnTrackEstablished = "established"
	ConnTrackInvalid     = "invalid"
	ChainIncoming        = "egress"
	ChainOutgoing        = "ingress"
	PolicyIncoming       = "ingress"
	PolicyOutgoing       = "egress"
	K8sProvider          = "k8s"
	PcnProvider          = "pcn"
)

// ParsedRules contains a list of the rules parsed for each direction
type ParsedRules struct {
	Incoming []k8sfirewall.ChainRule
	Outgoing []k8sfirewall.ChainRule
}
