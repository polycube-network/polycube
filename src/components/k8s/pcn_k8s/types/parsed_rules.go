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
)

// ParsedRules contains a list of the rules parsed for each direction
type ParsedRules struct {
	Ingress []k8sfirewall.ChainRule
	Egress  []k8sfirewall.ChainRule
}
