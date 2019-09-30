package parsers

import (
	"strconv"

	v1beta "github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/pkg/apis/polycube.network/v1beta"
	pcn_types "github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/types"
	"github.com/polycube-network/polycube/src/components/k8s/utils"
	k8sfirewall "github.com/polycube-network/polycube/src/components/k8s/utils/k8sfirewall"
	core_v1 "k8s.io/api/core/v1"
	networking_v1 "k8s.io/api/networking/v1"
)

// reformatPolicyName generate a new name for a policy, based on its parent
// policy name, the direction and the peer number
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

// getBasePolicyFromK8s returns the base ParsedPolicy from a kubernetes
// network policy. It will then be enriched while parsing.
func getBasePolicyFromK8s(policy *networking_v1.NetworkPolicy) pcn_types.ParsedPolicy {
	basePolicy := pcn_types.ParsedPolicy{
		ParentPolicy: pcn_types.ParentPolicy{
			Name:     policy.Name,
			Provider: pcn_types.K8sProvider,
			Priority: int32(defaultPriority),
		},
	}

	// Who to apply to?
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

// getBasePolicyFromPcn returns the base ParsedPolicy from a polycube
// network policy. It will then be enriched while parsing.
func getBasePolicyFromPcn(policy *v1beta.PolycubeNetworkPolicy) pcn_types.ParsedPolicy {
	priority := policy.Priority
	if policy.Priority == 0 {
		priority = v1beta.PolycubeNetworkPolicyPriority(defaultPriority)
	}

	basePolicy := pcn_types.ParsedPolicy{
		ParentPolicy: pcn_types.ParentPolicy{
			Name:     policy.Name,
			Provider: pcn_types.PcnProvider,
			Priority: int32(priority),
		},
	}

	basePolicy.Subject.Namespace = policy.Namespace
	basePolicy.CreationTime = policy.ObjectMeta.CreationTimestamp

	return basePolicy
}

// convertServiceProtocols converts a port in a service into its corresponding
// targetPort
func convertServiceProtocols(servPorts []core_v1.ServicePort) []pcn_types.ProtoPort {
	protoPorts := []pcn_types.ProtoPort{}

	for _, port := range servPorts {
		proto := "tcp"
		if port.Protocol == core_v1.ProtocolUDP {
			proto = "udp"
		}

		protoPorts = append(protoPorts, pcn_types.ProtoPort{
			DPort:    port.TargetPort.IntVal,
			Protocol: proto,
		})
	}

	return protoPorts
}

// pcnIngressIsFilled checks if there is an ingress container in the policy
func pcnIngressIsFilled(ingress v1beta.PolycubeNetworkPolicyIngressRuleContainer) bool {
	// Are there rules?
	if len(ingress.Rules) > 0 {
		return true
	}

	// Should it drop all?
	if ingress.DropAll != nil && *ingress.DropAll {
		return true
	}

	// Should it allow all?
	if ingress.AllowAll != nil && *ingress.AllowAll {
		return true
	}

	return false
}

// pcnIngressIsFilled checks if there is an egress container in the policy
func pcnEgressIsFilled(egress v1beta.PolycubeNetworkPolicyEgressRuleContainer) bool {
	// Are there rules?
	if len(egress.Rules) > 0 {
		return true
	}

	// Should it drop all?
	if egress.DropAll != nil && *egress.DropAll {
		return true
	}

	// Should it allow all?
	if egress.AllowAll != nil && *egress.AllowAll {
		return true
	}

	return false
}

// convertPcnAction is a helper function that converts an action verb from a
// polycube policy to the corresponding firewall action
func convertPcnAction(action v1beta.PolycubeNetworkPolicyRuleAction) string {
	converted := pcn_types.ActionDrop

	switch action {
	case v1beta.AllowAction, v1beta.ForwardAction, v1beta.PassAction, v1beta.PermitAction:
		converted = pcn_types.ActionForward
	}

	return converted
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

// BuildRuleTemplates builds the templates used as base to build the rules
func BuildRuleTemplates(direction, action string, ports []pcn_types.ProtoPort) pcn_types.ParsedRules {
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

// FillTemplates will build the appropriate rules based on the provided templates
func FillTemplates(sourceIP, destinationIP string, rules []k8sfirewall.ChainRule) []k8sfirewall.ChainRule {
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

// getNamespacesList gets all the queries needed to find the namespaces
// specified on a pcn policy onNamespaces field.
func getNamespacesList(onNs *v1beta.PolycubeNetworkPolicyNamespaceSelector) []*pcn_types.ObjectQuery {
	nsQueries := []*pcn_types.ObjectQuery{}

	if len(onNs.WithNames) > 0 {
		for _, ns := range onNs.WithNames {
			nsQueries = append(nsQueries, utils.BuildQuery(ns, nil))
		}
	} else {
		nsQueries = append(nsQueries, utils.BuildQuery("", onNs.WithLabels))
	}

	return nsQueries
}
