package types

import (
	k8sfirewall "github.com/SunSince90/polycube/src/components/k8s/utils/k8sfirewall"
)

const (
	ActionForward = "forward"
	ActionDrop    = "drop"
)

type ParsedRules struct {
	Ingress []k8sfirewall.ChainRule
	Egress  []k8sfirewall.ChainRule
}
