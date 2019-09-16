package parsers

import (
	"strconv"

	pcn_types "github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/types"
	k8sfirewall "github.com/polycube-network/polycube/src/components/k8s/utils/k8sfirewall"
	networking_v1 "k8s.io/api/networking/v1"
)

func reformatPolicyName(name, direction string, n, count int) string {
	stringN := strconv.FormatInt(int64(n), 10)
	stringCount := strconv.FormatInt(int64(count), 10)
	name += "#" + stringN

	if direction == pcn_types.PolicyIncoming {
		name += "i"
	} else {
		name += "e"
	}

	name += "p" + stringCount

	return name
}

func getBasePolicyFromK8s(policy *networking_v1.NetworkPolicy) pcn_types.ParsedPolicy {
	basePolicy := pcn_types.ParsedPolicy{
		ParentPolicy: pcn_types.ParentPolicy{
			Name:     policy.Name,
			Provider: pcn_types.K8sProvider,
			Priority: int32(5),
		},
	}

	// Who to apply to?
	// Hmm can policy.
	if len(policy.Spec.PodSelector.MatchLabels) > 0 {
		basePolicy.Subject.Query = &pcn_types.ObjectQuery{
			By:     "labels",
			Labels: policy.Spec.PodSelector.MatchLabels,
		}
	}

	basePolicy.Subject.Namespace = policy.Namespace
	basePolicy.CreationTime = policy.ObjectMeta.CreationTimestamp

	return basePolicy
}

func swapPortsDirection(ports pcn_types.ProtoPort) pcn_types.ProtoPort {
	swappedPorts := ports
	swappedPorts.DPort, swappedPorts.SPort = ports.SPort, ports.DPort

	return swappedPorts
}

// insertPorts will complete the rules by adding the appropriate ports
func insertPorts(rules []k8sfirewall.ChainRule, ports []pcn_types.ProtoPort) []k8sfirewall.ChainRule {
	// Don't make me go through this if there are no ports
	if len(ports) == 0 {
		return rules
	}

	newRules := make([]k8sfirewall.ChainRule, len(rules)*len(ports))

	// Finally, apply the ports that have been found
	t := 0
	for i := 0; i < len(rules); i++ {
		for _, generatedPort := range ports {
			newRules[t] = rules[i]
			newRules[t].Sport = generatedPort.SPort
			newRules[t].Dport = generatedPort.DPort
			newRules[t].L4proto = generatedPort.Protocol
			t++
		}
	}

	return newRules
}

func buildRuleTemplates(direction, action string, ports []pcn_types.ProtoPort) pcn_types.ParsedRules {
	// The rules to return
	rulesToReturn := pcn_types.ParsedRules{}

	// The rule to fill
	rule := k8sfirewall.ChainRule{
		Action:    string(action),
		Conntrack: pcn_types.ConnTrackNew,
	}

	// Fill them with the ports
	rulesWithPorts := insertPorts([]k8sfirewall.ChainRule{
		rule,
	}, ports)

	// --- Incoming packetrs
	if direction == pcn_types.PolicyIncoming {
		rulesToReturn.Incoming = rulesWithPorts
	} else {
		// --- Outgoing direction
		rulesToReturn.Outgoing = rulesWithPorts
	}

	return rulesToReturn
}

func fillTemplates(sourceIP, destinationIP string, rules []k8sfirewall.ChainRule) []k8sfirewall.ChainRule {
	if len(rules) == 0 {
		return rules
	}

	if len(sourceIP) == 0 && len(destinationIP) == 0 {
		return rules
	}

	newRules := make([]k8sfirewall.ChainRule, len(rules))
	for i, rule := range rules {
		newRules[i] = rule
		newRules[i].Src = sourceIP
		newRules[i].Dst = destinationIP
	}

	return newRules
}
