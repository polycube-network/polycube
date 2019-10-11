package parsers

import (
	"errors"

	"github.com/polycube-network/polycube/src/components/k8s/utils"
	meta_v1 "k8s.io/apimachinery/pkg/apis/meta/v1"

	pcn_types "github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/types"
	core_v1 "k8s.io/api/core/v1"
	networking_v1 "k8s.io/api/networking/v1"
)

// ParseK8sIngress parses the Ingress section of a policy
func ParseK8sIngress(policy *networking_v1.NetworkPolicy) []pcn_types.ParsedPolicy {
	//-------------------------------------
	// Init
	//-------------------------------------

	policies := []pcn_types.ParsedPolicy{}
	basePolicy := getBasePolicyFromK8s(policy)
	direction := pcn_types.PolicyIncoming
	basePolicy.Direction = direction

	//-------------------------------------
	// Preliminary checks
	//-------------------------------------

	// Is this an ingress policy?
	rules, _, _type := ParseK8sPolicyTypes(&policy.Spec)
	if _type == "egress" {
		return nil
	}

	// No rules?
	if len(rules) == 0 {
		// Rules is empty: nothing is accepted
		// No need to generate a drop action: the default action is drop anyway.
		basePolicy.Name = reformatPolicyName(policy.Name, direction, 0, 0)
		basePolicy.Action = "drop-all"
		return []pcn_types.ParsedPolicy{
			basePolicy,
		}
	}

	//-------------------------------------
	// Actual parsing
	//-------------------------------------
	peerID := 0
	ruleID := 0
	for _, rule := range rules {

		//-------------------------------------
		// Protocol & Port
		//-------------------------------------
		// First, parse the protocol: so, if an unsupported protocol is listed,
		// we silently ignore it. By doing it this way we don't have
		// to remove rules later on
		generatedPorts := parseK8sPorts(rule.Ports, direction)
		if len(rule.Ports) > 0 && len(generatedPorts) == 0 {
			// If this rule consists of only unsupported protocols, then we can't go on!
			// If we did, we would be creating wrong rules!
			// So we're going to skip this rule: the defaul drop will take care of it.
			continue
		}

		//-------------------------------------
		// Build the templates
		//-------------------------------------

		templates := BuildRuleTemplates(direction, pcn_types.ActionForward, generatedPorts)

		//-------------------------------------
		// Set up common fields
		//-------------------------------------

		newPolicy := basePolicy
		newPolicy.Ports = generatedPorts
		newPolicy.Action = "forward"
		newPolicy.Templates = templates
		newPolicy.Priority = int32(peerID)

		//-------------------------------------
		// Peers
		//-------------------------------------

		// From is {} ?
		if rule.From == nil {
			// From is nil: ALL resources are allowed
			newPolicy.Name = reformatPolicyName(policy.Name, direction, ruleID, peerID)
			newPolicy.Templates = BuildRuleTemplates(direction, pcn_types.ActionForward, generatedPorts)
			newPolicy.Action = "forward-all"
			policies = append(policies, newPolicy)
			peerID++
		}

		for i := 0; rule.From != nil && i < len(rule.From); i++ {
			from := rule.From[i]
			newPolicy.Name = reformatPolicyName(policy.Name, direction, i, peerID)

			//-------------------------------------
			// IPBlock
			//-------------------------------------

			if from.IPBlock != nil {
				exceptions, peer := getPeersFromIPBlock(from.IPBlock)

				// Were there any exceptions? This needs a new policy
				if exceptions != nil {
					exceptionsPolicy := newPolicy
					exceptionsPolicy.Action = "drop"
					exceptionsPolicy.Templates = BuildRuleTemplates(direction, pcn_types.ActionDrop, generatedPorts)
					exceptionsPolicy.Peer = *exceptions
					policies = append(policies, exceptionsPolicy)
					peerID++
				}

				// Add the peer now.
				// NOTE: the exceptions must be entered *before* the peer,
				// because they will need to be checked before in the fw list.
				newPolicy.Priority = int32(peerID)
				newPolicy.Name = reformatPolicyName(policy.Name, direction, i, peerID)
				newPolicy.Peer = *peer
			}

			//-------------------------------------
			// PodSelector And/Or NamespaceSelector
			//-------------------------------------
			if from.PodSelector != nil || from.NamespaceSelector != nil {
				peer, err := getPeersFromK8sSelectors(from.PodSelector, from.NamespaceSelector, policy.Namespace)

				if err == nil {
					newPolicy.Peer = *peer
				} else {
					logger.Errorf("Error while parsing selectors: %s", err)
				}
			}

			policies = append(policies, newPolicy)
			peerID++
		}
	}

	return policies
}

// ParseK8sEgress parses the Egress section of a policy
func ParseK8sEgress(policy *networking_v1.NetworkPolicy) []pcn_types.ParsedPolicy {
	//-------------------------------------
	// Init
	//-------------------------------------

	policies := []pcn_types.ParsedPolicy{}
	basePolicy := getBasePolicyFromK8s(policy)
	direction := pcn_types.PolicyOutgoing
	basePolicy.Direction = direction

	//-------------------------------------
	// Preliminary checks
	//-------------------------------------

	// Is this an ingress policy?
	_, rules, _type := ParseK8sPolicyTypes(&policy.Spec)
	if _type == "ingress" {
		return nil
	}

	if len(rules) == 0 {
		// Rules is empty: nothing is accepted
		// No need to generate a drop action: the default action is drop anyway.
		basePolicy.Name = reformatPolicyName(policy.Name, direction, 0, 0)
		basePolicy.Action = "drop-all"
		return []pcn_types.ParsedPolicy{
			basePolicy,
		}
	}

	//-------------------------------------
	// Actual parsing
	//-------------------------------------
	peerID := 0
	ruleID := 0
	for _, rule := range rules {

		//-------------------------------------
		// Protocol & Port
		//-------------------------------------
		// First, parse the protocol: so, if an unsupported protocol is listed,
		// we silently ignore it. By doing it this way we don't have
		// to remove rules later on
		generatedPorts := parseK8sPorts(rule.Ports, direction)
		if len(rule.Ports) > 0 && len(generatedPorts) == 0 {
			// If this rule consists of only unsupported protocols, then we can't go on!
			// If we did, we would be creating wrong rules!
			// So we're going to skip this rule: the defaul drop will take care of it.
			continue
		}

		//-------------------------------------
		// Build the templates
		//-------------------------------------

		templates := BuildRuleTemplates(direction, pcn_types.ActionForward, generatedPorts)

		//-------------------------------------
		// Set up common fields
		//-------------------------------------

		newPolicy := basePolicy
		newPolicy.Ports = generatedPorts
		newPolicy.Action = "forward"
		newPolicy.Templates = templates
		newPolicy.Priority = int32(peerID)

		//-------------------------------------
		// Peers
		//-------------------------------------

		// To is {} ?
		if rule.To == nil {
			// To is nil: ALL resources are allowed
			newPolicy.Name = reformatPolicyName(policy.Name, direction, ruleID, peerID)
			newPolicy.Templates = BuildRuleTemplates(direction, pcn_types.ActionForward, generatedPorts)
			newPolicy.Action = "forward-all"
			policies = append(policies, newPolicy)
			peerID++
		}

		for i := 0; rule.To != nil && i < len(rule.To); i++ {
			to := rule.To[i]
			newPolicy.Name = reformatPolicyName(policy.Name, direction, i, peerID)

			//-------------------------------------
			// IPBlock
			//-------------------------------------

			if to.IPBlock != nil {
				exceptions, peer := getPeersFromIPBlock(to.IPBlock)

				// Were there any exceptions? This needs a new policy
				if exceptions != nil {
					exceptionsPolicy := newPolicy
					exceptionsPolicy.Action = "drop"
					exceptionsPolicy.Templates = BuildRuleTemplates(direction, pcn_types.ActionDrop, generatedPorts)
					exceptionsPolicy.Peer = *exceptions
					policies = append(policies, exceptionsPolicy)
					peerID++
				}

				// Add the peer now.
				// NOTE: the exceptions must be entered *before* the peer,
				// because they will need to be checked before in the fw list.
				newPolicy.Priority = int32(peerID)
				newPolicy.Peer = *peer
			}

			//-------------------------------------
			// PodSelector And/Or NamespaceSelector
			//-------------------------------------
			if to.PodSelector != nil || to.NamespaceSelector != nil {
				peer, err := getPeersFromK8sSelectors(to.PodSelector, to.NamespaceSelector, policy.Namespace)

				if err == nil {
					newPolicy.Peer = *peer
				} else {
					logger.Errorf("Error while parsing selectors: %s", err)
				}
			}

			policies = append(policies, newPolicy)
			peerID++
		}
	}

	return policies
}

// ParseK8sPolicyTypes will parse the policy type of the policy
// and return the appropriate rules and the type of this policy
func ParseK8sPolicyTypes(policySpec *networking_v1.NetworkPolicySpec) ([]networking_v1.NetworkPolicyIngressRule, []networking_v1.NetworkPolicyEgressRule, string) {
	var ingress []networking_v1.NetworkPolicyIngressRule
	var egress []networking_v1.NetworkPolicyEgressRule

	ingress = nil
	egress = nil
	policyType := ""

	// What if spec is not even there?
	if policySpec == nil {
		return nil, nil, "ingress"
	}

	// Documentation is not very specific about the possibility of PolicyTypes
	// being [], so I made this dumb piece of code just in case
	if policySpec.PolicyTypes == nil {
		ingress = policySpec.Ingress
		policyType = "ingress"
	} else {
		if len(policySpec.PolicyTypes) == 0 {
			ingress = policySpec.Ingress
			policyType = "ingress"
		} else {
			policyTypes := policySpec.PolicyTypes

			for _, val := range policyTypes {
				// Can't use if-else because user may disable validation and insert
				// trash values
				if val == networking_v1.PolicyTypeIngress {
					ingress = policySpec.Ingress
					policyType = "ingress"
				}
				if val == networking_v1.PolicyTypeEgress {
					egress = policySpec.Egress
					policyType = "egress"
				}
			}

			if ingress != nil && egress != nil {
				policyType = ""
			}
		}
	}

	return ingress, egress, policyType
}

func getPeersFromIPBlock(block *networking_v1.IPBlock) (*pcn_types.PolicyPeer, *pcn_types.PolicyPeer) {
	var exceptions *pcn_types.PolicyPeer

	// First get exceptions, if any
	if len(block.Except) > 0 {
		exceptions = &pcn_types.PolicyPeer{
			IPBlock: block.Except,
		}
	}

	// Then get the allowed IPBlock
	peer := pcn_types.PolicyPeer{
		IPBlock: []string{
			block.CIDR,
		},
	}

	return exceptions, &peer
}

func getPeersFromK8sSelectors(podSelector, namespaceSelector *meta_v1.LabelSelector, namespace string) (*pcn_types.PolicyPeer, error) {
	// Init
	var queryPod *pcn_types.ObjectQuery
	var queryNs *pcn_types.ObjectQuery

	// If podSelector is empty (len = 0): select everything and then forward

	// Build the query
	if podSelector != nil {

		// This is not supported yet...
		if podSelector.MatchExpressions != nil {
			return nil, errors.New("MatchExpressions on pod selector is not supported yet")
		}

		queryPod = utils.BuildQuery("", podSelector.MatchLabels)
	}

	// Namespace selector
	if namespaceSelector != nil {

		// Match expressions?
		if namespaceSelector.MatchExpressions != nil {
			// This is not supported yet...
			return nil, errors.New("MatchExpressions on namespace selector is not supported yet")
		}

		queryNs = utils.BuildQuery("", namespaceSelector.MatchLabels)
	} else {
		// If namespace selector is nil,
		// we're going to use the one we found on the policy
		if len(namespace) == 0 {
			return nil, errors.New("Namespace name not provided")
		}

		queryNs = utils.BuildQuery(namespace, nil)
	}

	// Get its key. It is going to be used by the firewall manager to identify
	// the peer
	Key := utils.BuildObjectKey(queryPod, "pod")
	nsKey := utils.BuildObjectKey(queryNs, "namespace")

	return &pcn_types.PolicyPeer{
		Peer:      queryPod,
		Namespace: queryNs,
		Key:       Key + "|" + nsKey,
	}, nil
}

// parseK8sPorts will parse the protocol and port and get the desired ports
// in a format that the firewall will understand
func parseK8sPorts(ports []networking_v1.NetworkPolicyPort, direction string) []pcn_types.ProtoPort {
	// Init
	generatedPorts := []pcn_types.ProtoPort{}

	for _, port := range ports {
		sport := int32(0)
		dport := int32(0)

		// If protocol is nil, then we have to get all protocols
		if port.Protocol == nil {

			// If the port is not nil, default port is not 0
			var defaultPort int32
			if port.Port != nil {
				defaultPort = int32(port.Port.IntValue())
			}

			// UPDATE: documentation is not very clear, but probably k8s
			// policies always mean the destination port when specifying
			// the port
			/*if direction == pcn_types.PolicyIncoming {
				dport = defaultPort
			} else {
				sport = defaultPort
			}*/
			dport = defaultPort
			generatedPorts = append(generatedPorts, pcn_types.ProtoPort{
				SPort: sport,
				DPort: dport,
			})
		} else {
			// else parse the protocol
			supported, proto, port := parseProtocolAndPort(port)

			// Our firewall does not support SCTP, so we check if protocol is supported
			if supported {
				// UPDATE: documentation is not very clear, but probably k8s
				// policies always mean the destination port when specifying
				// the port
				/*if direction == pcn_types.PolicyIncoming {
					dport = port
				} else {
					sport = port
				}*/
				dport = port
				generatedPorts = append(generatedPorts, pcn_types.ProtoPort{
					Protocol: proto,
					SPort:    sport,
					DPort:    dport,
				})
			}
		}
	}

	return generatedPorts
}

// parseProtocolAndPort parses the protocol in order to know
// if it is supported by the firewall
func parseProtocolAndPort(pp networking_v1.NetworkPolicyPort) (bool, string, int32) {
	// Not sure if port can be nil, but it doesn't harm to do a simple reset
	var port int32
	if pp.Port != nil {
		port = int32(pp.Port.IntValue())
	}

	switch *pp.Protocol {
	// TCP?
	case core_v1.ProtocolTCP:
		return true, "TCP", port

	// UDP?
	case core_v1.ProtocolUDP:
		return true, "UDP", port
	}

	// Not supported ¯\_(ツ)_/¯
	return false, "", 0
}

// DoesK8sPolicyAffectPod checks if the provided policy affects the provided pod,
// returning TRUE if it does
func DoesK8sPolicyAffectPod(policy *networking_v1.NetworkPolicy, pod *core_v1.Pod) bool {

	// MatchExpressions? (we don't support them yet)
	if len(policy.Spec.PodSelector.MatchExpressions) > 0 {
		return false
	}

	// Not in the same namespace?
	if policy.Namespace != pod.Namespace {
		return false
	}

	// No labels in the policy? (= must be applied by all pods)
	if len(policy.Spec.PodSelector.MatchLabels) == 0 {
		return true
	}

	// No labels in the pod?
	// (if you're here, it means that there are labels in the policy.
	// But this pod has no labels, so this policy does not apply to it)
	if len(pod.Labels) == 0 {
		return false
	}

	// Finally check the labels
	return utils.AreLabelsContained(policy.Spec.PodSelector.MatchLabels, pod.Labels)
}
