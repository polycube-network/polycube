package parsers

import (
	v1beta "github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/pkg/apis/polycube.network/v1beta"
	pcn_types "github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/types"
	"github.com/polycube-network/polycube/src/components/k8s/utils"
	core_v1 "k8s.io/api/core/v1"
)

// ParsePcnIngress parses the Ingress section of a polycube policy
func ParsePcnIngress(policy *v1beta.PolycubeNetworkPolicy, serv *core_v1.Service) []pcn_types.ParsedPolicy {
	//-------------------------------------
	// Init
	//-------------------------------------

	ingressRules := policy.Spec.IngressRules
	if !pcnIngressIsFilled(ingressRules) {
		return nil
	}

	policies := []pcn_types.ParsedPolicy{}
	basePolicy := getBasePolicyFromPcn(policy)
	direction := pcn_types.PolicyIncoming
	basePolicy.Direction = direction
	basePolicy.PreventDirectAccess = (policy.Spec.IngressRules.PreventDirectAccess != nil && *policy.Spec.IngressRules.PreventDirectAccess)

	//-------------------------------------
	// Preliminary checks
	//-------------------------------------

	ports := []pcn_types.ProtoPort{}
	if serv != nil {
		basePolicy.Subject.Query = utils.BuildQuery("", serv.Spec.Selector)
		basePolicy.Subject.IsService = true

		// Get the ports
		ports = convertServiceProtocols(serv.Spec.Ports)
	} else {
		if policy.ApplyTo.Any == nil || (policy.ApplyTo.Any != nil && !*policy.ApplyTo.Any) {
			basePolicy.Subject.Query = utils.BuildQuery("", policy.ApplyTo.WithLabels)
		}
	}

	// Drop Everything?
	if ingressRules.DropAll != nil && *ingressRules.DropAll {
		basePolicy.Name = reformatPolicyName(policy.Name, direction, 0, 0)
		basePolicy.Action = "drop-all"
		basePolicy.Templates = BuildRuleTemplates(direction, pcn_types.ActionDrop, ports)
		return []pcn_types.ParsedPolicy{
			basePolicy,
		}
	}

	// Allow Everything?
	if ingressRules.AllowAll != nil && *ingressRules.AllowAll {
		// Rules is empty: nothing is accepted
		// No need to generate a drop action: the default action is drop anyway.
		basePolicy.Name = reformatPolicyName(policy.Name, direction, 0, 0)
		basePolicy.Templates = BuildRuleTemplates(direction, pcn_types.ActionForward, ports)
		basePolicy.Action = "forward-all"
		return []pcn_types.ParsedPolicy{
			basePolicy,
		}
	}

	//-------------------------------------
	// Actual parsing
	//-------------------------------------
	peerID := 0
	for i, rule := range ingressRules.Rules {

		//-------------------------------------
		// Protocol & Port
		//-------------------------------------

		generatedPorts := ports
		if len(rule.Protocols) > 0 {
			generatedPorts = parsePcnPorts(rule.Protocols)
		}

		//-------------------------------------
		// Build the templates
		//-------------------------------------

		action := convertPcnAction(rule.Action)
		templates := BuildRuleTemplates(direction, action, generatedPorts)

		//-------------------------------------
		// Set up common fields
		//-------------------------------------

		newPolicy := basePolicy
		newPolicy.Ports = generatedPorts
		newPolicy.Action = action
		newPolicy.Templates = templates
		newPolicy.Priority = int32(peerID)

		//-------------------------------------
		// Peers
		//-------------------------------------

		// The World?
		if rule.From.Peer == v1beta.WorldPeer {
			peer := getPeersFromPcnWorld(rule.From.WithIP)
			newPolicy.Peer = peer
			newPolicy.Name = reformatPolicyName(policy.Name, direction, i, peerID)
			policies = append(policies, newPolicy)
			peerID++
		}

		// Pod?
		if rule.From.Peer == v1beta.PodPeer {
			peers := getPeersFromPcnPodSelector(rule.From, policy.Namespace)
			for _, peer := range peers {
				peerPolicy := newPolicy
				peerPolicy.Peer = peer
				peerPolicy.Name = reformatPolicyName(policy.Name, direction, i, peerID)
				policies = append(policies, peerPolicy)
				peerID++
			}
		}
	}

	return policies
}

// ParsePcnEgress parses the Egress section of a polycube policy
func ParsePcnEgress(policy *v1beta.PolycubeNetworkPolicy, serv *core_v1.Service) []pcn_types.ParsedPolicy {
	//-------------------------------------
	// Init
	//-------------------------------------

	egressRules := policy.Spec.EgressRules
	if !pcnEgressIsFilled(egressRules) {
		return nil
	}

	policies := []pcn_types.ParsedPolicy{}
	basePolicy := getBasePolicyFromPcn(policy)
	direction := pcn_types.PolicyOutgoing
	basePolicy.Direction = direction

	//-------------------------------------
	// Preliminary checks
	//-------------------------------------

	ports := []pcn_types.ProtoPort{}
	if serv != nil {
		basePolicy.Subject.Query = utils.BuildQuery("", serv.Spec.Selector)
		basePolicy.Subject.IsService = true

		// Get the ports
		ports = convertServiceProtocols(serv.Spec.Ports)
	}

	// Drop Everything?
	if egressRules.DropAll != nil && *egressRules.DropAll {
		basePolicy.Name = reformatPolicyName(policy.Name, direction, 0, 0)
		basePolicy.Action = "drop-all"
		basePolicy.Templates = BuildRuleTemplates(direction, pcn_types.ActionDrop, ports)
		return []pcn_types.ParsedPolicy{
			basePolicy,
		}
	}

	// Allow Everything?
	if egressRules.AllowAll != nil && *egressRules.AllowAll {
		// Rules is empty: nothing is accepted
		// No need to generate a drop action: the default action is drop anyway.
		basePolicy.Name = reformatPolicyName(policy.Name, direction, 0, 0)
		basePolicy.Templates = BuildRuleTemplates(direction, pcn_types.ActionForward, ports)
		basePolicy.Action = "forward-all"
		return []pcn_types.ParsedPolicy{
			basePolicy,
		}
	}

	//-------------------------------------
	// Actual parsing
	//-------------------------------------
	peerID := 0
	for i, rule := range egressRules.Rules {

		//-------------------------------------
		// Protocol & Port
		//-------------------------------------

		generatedPorts := ports
		if len(rule.Protocols) > 0 {
			generatedPorts = parsePcnPorts(rule.Protocols)
		}

		//-------------------------------------
		// Build the templates
		//-------------------------------------

		action := convertPcnAction(rule.Action)
		templates := BuildRuleTemplates(direction, action, generatedPorts)

		//-------------------------------------
		// Set up common fields
		//-------------------------------------

		newPolicy := basePolicy
		newPolicy.Ports = generatedPorts
		newPolicy.Action = action
		newPolicy.Templates = templates
		newPolicy.Priority = int32(peerID)

		//-------------------------------------
		// Peers
		//-------------------------------------

		// The World?
		if rule.To.Peer == v1beta.WorldPeer {
			peer := getPeersFromPcnWorld(rule.To.WithIP)
			newPolicy.Peer = peer
			newPolicy.Name = reformatPolicyName(policy.Name, direction, i, peerID)
			policies = append(policies, newPolicy)
			peerID++
		}

		// Pod?
		if rule.To.Peer == v1beta.PodPeer {
			peers := getPeersFromPcnPodSelector(rule.To, policy.Namespace)
			for _, peer := range peers {
				peerPolicy := newPolicy
				peerPolicy.Peer = peer
				peerPolicy.Name = reformatPolicyName(policy.Name, direction, i, peerID)
				policies = append(policies, peerPolicy)
				peerID++
			}
		}
	}

	return policies
}

func parsePcnPorts(protoPorts []v1beta.PolycubeNetworkPolicyProtocolContainer) []pcn_types.ProtoPort {
	toReturn := []pcn_types.ProtoPort{}
	for _, pp := range protoPorts {
		toReturn = append(toReturn, pcn_types.ProtoPort{
			SPort:    pp.Ports.Source,
			DPort:    pp.Ports.Destination,
			Protocol: string(pp.Protocol),
		})
	}

	return toReturn
}

func getPeersFromPcnWorld(ips v1beta.PolycubeNetworkPolicyWithIP) pcn_types.PolicyPeer {
	return pcn_types.PolicyPeer{
		IPBlock: ips.List,
	}
}

func getPeersFromPcnPodSelector(peer v1beta.PolycubeNetworkPolicyPeer, namespace string) []pcn_types.PolicyPeer {
	var podQuery *pcn_types.ObjectQuery
	parsedPeers := []pcn_types.PolicyPeer{}

	if peer.Any == nil || (peer.Any != nil && !*peer.Any) {
		podQuery = utils.BuildQuery("", peer.WithLabels)
	}

	key := utils.BuildObjectKey(podQuery, "pod")

	// Get the namespace
	if peer.OnNamespace == nil {
		parsedPeers = append(parsedPeers, pcn_types.PolicyPeer{
			Peer:      podQuery,
			Namespace: utils.BuildQuery(namespace, nil),
			Key:       key + "|" + utils.BuildObjectKey(nil, "ns"),
		})
	} else {
		if peer.OnNamespace.Any != nil && *peer.OnNamespace.Any {
			parsedPeers = append(parsedPeers, pcn_types.PolicyPeer{
				Peer: podQuery,
				Key:  key + "|" + utils.BuildObjectKey(nil, "ns"),
			})
		} else {
			nsList := getNamespacesList(peer.OnNamespace)
			for _, ns := range nsList {
				parsedPeers = append(parsedPeers, pcn_types.PolicyPeer{
					Peer:      podQuery,
					Namespace: ns,
					Key:       key + "|" + utils.BuildObjectKey(ns, "ns"),
				})
			}
		}
	}

	return parsedPeers
}
