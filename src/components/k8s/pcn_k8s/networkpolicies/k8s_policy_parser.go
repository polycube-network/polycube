package networkpolicies

import (
	"errors"
	"fmt"
	"net"
	"sync"

	"github.com/polycube-network/polycube/src/components/k8s/utils"

	pcn_controllers "github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/controllers"
	pcn_types "github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/types"
	k8sfirewall "github.com/polycube-network/polycube/src/components/k8s/utils/k8sfirewall"
	log "github.com/sirupsen/logrus"
	core_v1 "k8s.io/api/core/v1"
	networking_v1 "k8s.io/api/networking/v1"
	meta_v1 "k8s.io/apimachinery/pkg/apis/meta/v1"
)

// PcnK8sPolicyParser is the default policy (e.g.: kubernetes') parser
type PcnK8sPolicyParser interface {
	// ParseRules is a convenient method for parsing Ingress and Egress concurrently
	ParseRules(ingress []networking_v1.NetworkPolicyIngressRule, egress []networking_v1.NetworkPolicyEgressRule, currentNamespace string) pcn_types.ParsedRules
	// ParseIngress parses the Ingress section of a policy
	ParseIngress(rules []networking_v1.NetworkPolicyIngressRule, namespace string) pcn_types.ParsedRules
	// ParseEgress parses the Egress section of a policy
	ParseEgress(rules []networking_v1.NetworkPolicyEgressRule, namespace string) pcn_types.ParsedRules
	// ParsePolicyTypes will parse the policy type of the policy
	// and return the appropriate rules and the type of this policy
	ParsePolicyTypes(policySpec *networking_v1.NetworkPolicySpec) ([]networking_v1.NetworkPolicyIngressRule, []networking_v1.NetworkPolicyEgressRule, string)
	// BuildActions builds actions that are going to be used by firewalls
	// so they know how to react to pods.
	BuildActions(ingress []networking_v1.NetworkPolicyIngressRule, egress []networking_v1.NetworkPolicyEgressRule, currentNamespace string) []pcn_types.FirewallAction
	// DoesPolicyAffectPod checks if the provided policy affects the provided pod,
	// returning TRUE if it does
	DoesPolicyAffectPod(policy *networking_v1.NetworkPolicy, pod *core_v1.Pod) bool
}

// K8sPolicyParser is the implementation of the default parser
type K8sPolicyParser struct {
	podController pcn_controllers.PodController
	vPodsRange    *net.IPNet
}

// newK8sPolicyParser starts a new k8s policy parser
func newK8sPolicyParser(podController pcn_controllers.PodController, vPodsRange *net.IPNet) *K8sPolicyParser {
	return &K8sPolicyParser{
		podController: podController,
		vPodsRange:    vPodsRange,
	}
}

// ParseRules is a convenient method for parsing Ingress and Egress concurrently
func (d *K8sPolicyParser) ParseRules(ingress []networking_v1.NetworkPolicyIngressRule, egress []networking_v1.NetworkPolicyEgressRule, currentNamespace string) pcn_types.ParsedRules {
	parsed := pcn_types.ParsedRules{
		Ingress: []k8sfirewall.ChainRule{},
		Egress:  []k8sfirewall.ChainRule{},
	}

	var parseWait sync.WaitGroup
	var lock sync.Mutex

	parseWait.Add(2)

	//-------------------------------------
	// Parse the ingress rules
	//-------------------------------------

	go func() {
		defer parseWait.Done()
		result := d.ParseIngress(ingress, currentNamespace)

		lock.Lock()
		parsed.Ingress = append(parsed.Ingress, result.Ingress...)
		parsed.Egress = append(parsed.Egress, result.Egress...)
		lock.Unlock()
	}()

	//-------------------------------------
	// Parse the egress rules
	//-------------------------------------

	go func() {
		defer parseWait.Done()
		result := d.ParseEgress(egress, currentNamespace)

		lock.Lock()
		parsed.Ingress = append(parsed.Ingress, result.Ingress...)
		parsed.Egress = append(parsed.Egress, result.Egress...)
		lock.Unlock()
	}()

	// Wait for them to finish before doing the rest
	parseWait.Wait()

	return parsed
}

// ParseIngress parses the Ingress section of a policy
func (d *K8sPolicyParser) ParseIngress(rules []networking_v1.NetworkPolicyIngressRule, namespace string) pcn_types.ParsedRules {

	//-------------------------------------
	// Init
	//-------------------------------------
	l := log.New().WithFields(log.Fields{"by": KPP, "method": "ParseIngress"})
	parsed := pcn_types.ParsedRules{
		Ingress: []k8sfirewall.ChainRule{},
		Egress:  []k8sfirewall.ChainRule{},
	}
	// Ingress in the policy is egress in the firewall
	direction := "egress"

	//-------------------------------------
	// Preliminary checks
	//-------------------------------------

	// Rules is nil?
	if rules == nil {
		return parsed
	}

	// No rules?
	if len(rules) == 0 {
		// Rules is empty: nothing is accepted
		// No need to generate a drop action: the default action is drop anyway.
		return parsed
	}

	//-------------------------------------
	// Actual parsing
	//-------------------------------------
	for _, rule := range rules {

		//-------------------------------------
		// Protocol & Port
		//-------------------------------------
		// First, parse the protocol: so, if an unsupported protocol is listed,
		// we silently ignore it. By doing it this way we don't have
		// to remove rules later on
		generatedPorts := d.parsePorts(rule.Ports)
		if len(rule.Ports) > 0 && len(generatedPorts) == 0 {
			// If this rule consists of only unsupported protocols, then we can't go on!
			// If we did, we would be creating wrong rules!
			// So we're going to skip this rule: the defaul drop will take care of it.
			continue
		}

		//	The fw rules that are going to be generated for this policy rule
		generatedIngressRules := []k8sfirewall.ChainRule{}
		generatedEgressRules := []k8sfirewall.ChainRule{}

		//-------------------------------------
		// Peers
		//-------------------------------------

		// From is {} ?
		if rule.From == nil {
			// From is nil: ALL resources are allowed
			result := d.getConnectionTemplate(direction, "", "", pcn_types.ActionForward, []pcn_types.ProtoPort{})
			generatedIngressRules = append(generatedIngressRules, result.Ingress...)
			generatedEgressRules = append(generatedEgressRules, result.Egress...)
		}

		for i := 0; rule.From != nil && i < len(rule.From); i++ {
			from := rule.From[i]

			//-------------------------------------
			// IPBlock
			//-------------------------------------
			if from.IPBlock != nil {
				ipblock := d.parseIPBlock(from.IPBlock, direction)
				generatedIngressRules = append(generatedIngressRules, ipblock.Ingress...)
				generatedEgressRules = append(generatedEgressRules, ipblock.Egress...)
			}

			//-------------------------------------
			// PodSelector And/Or NamespaceSelector
			//-------------------------------------
			if from.PodSelector != nil || from.NamespaceSelector != nil {
				rulesGot, err := d.parseSelectors(from.PodSelector, from.NamespaceSelector, namespace, direction)

				if err == nil {
					if len(rulesGot.Ingress) > 0 {
						generatedIngressRules = append(generatedIngressRules, rulesGot.Ingress...)
					}
					if len(rulesGot.Egress) > 0 {
						generatedEgressRules = append(generatedEgressRules, rulesGot.Egress...)
					}
				} else {
					l.Errorf("Error while parsing selectors: %s", err)
				}
			}
		}

		//-------------------------------------
		// Finalize
		//-------------------------------------
		rulesWithPorts := d.insertPorts(generatedIngressRules, generatedEgressRules, generatedPorts)
		parsed.Ingress = append(parsed.Ingress, rulesWithPorts.Ingress...)
		parsed.Egress = append(parsed.Egress, rulesWithPorts.Egress...)
	}

	return parsed
}

// ParseEgress parses the Egress section of a policy
func (d *K8sPolicyParser) ParseEgress(rules []networking_v1.NetworkPolicyEgressRule, namespace string) pcn_types.ParsedRules {
	//-------------------------------------
	// Init
	//-------------------------------------
	l := log.New().WithFields(log.Fields{"by": KPP, "method": "ParseEgress"})
	parsed := pcn_types.ParsedRules{
		Ingress: []k8sfirewall.ChainRule{},
		Egress:  []k8sfirewall.ChainRule{},
	}
	// Egress in the policy is ingress in the firewall
	direction := "ingress"

	//-------------------------------------
	// Preliminary checks
	//-------------------------------------

	// Rules is nil?
	if rules == nil {
		return parsed
	}

	// No rules?
	if len(rules) == 0 {
		// Rules is empty: nothing is accepted
		//	No need to generated a drop rule: the default action is drop anyway.
		return parsed
	}

	//-------------------------------------
	// Actual parsing
	//-------------------------------------

	for _, rule := range rules {

		//-------------------------------------
		// Protocol & Port
		//-------------------------------------
		generatedPorts := d.parsePorts(rule.Ports)
		if len(rule.Ports) > 0 && len(generatedPorts) == 0 {
			continue
		}

		//	The fw rules that are going to be generated for this policy rule
		generatedIngressRules := []k8sfirewall.ChainRule{}
		generatedEgressRules := []k8sfirewall.ChainRule{}

		//-------------------------------------
		// Peers
		//-------------------------------------

		// To is {} ?
		if rule.To == nil {
			result := d.getConnectionTemplate(direction, "", "", pcn_types.ActionForward, []pcn_types.ProtoPort{})
			generatedIngressRules = append(generatedIngressRules, result.Ingress...)
			generatedEgressRules = append(generatedEgressRules, result.Egress...)
		}

		for i := 0; rule.To != nil && i < len(rule.To); i++ {
			to := rule.To[i]

			// IPBlock?
			if to.IPBlock != nil {
				ipblock := d.parseIPBlock(to.IPBlock, direction)
				generatedIngressRules = append(generatedIngressRules, ipblock.Ingress...)
				generatedEgressRules = append(generatedEgressRules, ipblock.Egress...)
			}

			// PodSelector Or NamespaceSelector?
			if to.PodSelector != nil || to.NamespaceSelector != nil {
				rulesGot, err := d.parseSelectors(to.PodSelector, to.NamespaceSelector, namespace, direction)

				if err == nil {
					if len(rulesGot.Ingress) > 0 {
						generatedIngressRules = append(generatedIngressRules, rulesGot.Ingress...)
					}
					if len(rulesGot.Egress) > 0 {
						generatedEgressRules = append(generatedEgressRules, rulesGot.Egress...)
					}
				} else {
					l.Errorf("Error while parsing selectors: %s", err)
				}
			}
		}

		//-------------------------------------
		// Finalize
		//-------------------------------------
		rulesWithPorts := d.insertPorts(generatedIngressRules, generatedEgressRules, generatedPorts)
		parsed.Ingress = append(parsed.Ingress, rulesWithPorts.Ingress...)
		parsed.Egress = append(parsed.Egress, rulesWithPorts.Egress...)
	}

	return parsed
}

// insertPorts will complete the rules by adding the appropriate ports
func (d *K8sPolicyParser) insertPorts(generatedIngressRules, generatedEgressRules []k8sfirewall.ChainRule, generatedPorts []pcn_types.ProtoPort) pcn_types.ParsedRules {
	// Don't make me go through this if there are no ports
	if len(generatedPorts) == 0 {
		return pcn_types.ParsedRules{
			Ingress: generatedIngressRules,
			Egress:  generatedEgressRules,
		}
	}

	parsed := pcn_types.ParsedRules{
		Ingress: []k8sfirewall.ChainRule{},
		Egress:  []k8sfirewall.ChainRule{},
	}

	// Finally, for each parsed rule, apply the ports that have been found

	// -- Ingress chain
	for i := 0; i < len(generatedIngressRules); i++ {
		rule := generatedIngressRules[i]
		for _, generatedPort := range generatedPorts {
			edited := rule
			edited.Sport = generatedPort.Port
			edited.L4proto = generatedPort.Protocol
			parsed.Ingress = append(parsed.Ingress, edited)
		}
	}

	// -- Egress chain
	for i := 0; i < len(generatedEgressRules); i++ {
		rule := generatedEgressRules[i]
		for _, generatedPort := range generatedPorts {
			edited := rule
			edited.Dport = generatedPort.Port
			edited.L4proto = generatedPort.Protocol
			parsed.Egress = append(parsed.Egress, edited)
		}
	}

	return parsed
}

// ParsePolicyTypes will parse the policy type of the policy
// and return the appropriate rules and the type of this policy
func (d *K8sPolicyParser) ParsePolicyTypes(policySpec *networking_v1.NetworkPolicySpec) ([]networking_v1.NetworkPolicyIngressRule, []networking_v1.NetworkPolicyEgressRule, string) {
	var ingress []networking_v1.NetworkPolicyIngressRule
	var egress []networking_v1.NetworkPolicyEgressRule

	ingress = nil
	egress = nil
	policyType := "*"

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
				policyType = "*"
			}
		}
	}

	return ingress, egress, policyType
}

// parseIPBlock will parse the IPBlock from the network policy
// and return the correct rules
func (d *K8sPolicyParser) parseIPBlock(block *networking_v1.IPBlock, k8sDirection string) pcn_types.ParsedRules {
	parsed := pcn_types.ParsedRules{
		Ingress: []k8sfirewall.ChainRule{},
		Egress:  []k8sfirewall.ChainRule{},
	}

	// Actually, these two cannot happen with kubernetes
	if block == nil {
		return parsed
	}

	if len(block.CIDR) == 0 {
		return parsed
	}

	// Loop through all exceptions
	for _, exception := range block.Except {
		exceptionRule := k8sfirewall.ChainRule{
			Action:    pcn_types.ActionDrop,
			Conntrack: pcn_types.ConnTrackNew,
		}

		if k8sDirection == "ingress" {
			exceptionRule.Dst = exception
			parsed.Ingress = append(parsed.Ingress, exceptionRule)
		} else {
			exceptionRule.Src = exception
			parsed.Egress = append(parsed.Egress, exceptionRule)
		}
	}

	// Add the default one
	cidrRules := pcn_types.ParsedRules{}
	if k8sDirection == "ingress" {
		cidrRules = d.getConnectionTemplate(k8sDirection, block.CIDR, "", pcn_types.ActionForward, []pcn_types.ProtoPort{})
	} else {
		cidrRules = d.getConnectionTemplate(k8sDirection, "", block.CIDR, pcn_types.ActionForward, []pcn_types.ProtoPort{})
	}

	parsed.Ingress = append(parsed.Ingress, cidrRules.Ingress...)
	parsed.Egress = append(parsed.Egress, cidrRules.Egress...)

	return parsed
}

// parsePorts will parse the protocol and port and get the desired ports
// in a format that the firewall will understand
func (d *K8sPolicyParser) parsePorts(ports []networking_v1.NetworkPolicyPort) []pcn_types.ProtoPort {
	// Init
	generatedPorts := []pcn_types.ProtoPort{}

	for _, port := range ports {

		// If protocol is nil, then we have to get all protocols
		if port.Protocol == nil {

			// If the port is not nil, default port is not 0
			var defaultPort int32
			if port.Port != nil {
				defaultPort = int32(port.Port.IntValue())
			}

			generatedPorts = append(generatedPorts, pcn_types.ProtoPort{
				Port: defaultPort,
			})

		} else {
			// else parse the protocol
			supported, proto, port := d.parseProtocolAndPort(port)

			// Our firewall does not support SCTP, so we check if protocol is supported
			if supported {
				generatedPorts = append(generatedPorts, pcn_types.ProtoPort{
					Protocol: proto,
					Port:     port,
				})
			}
		}
	}

	return generatedPorts
}

// parseProtocolAndPort parses the protocol in order to know
// if it is supported by the firewall
func (d *K8sPolicyParser) parseProtocolAndPort(pp networking_v1.NetworkPolicyPort) (bool, string, int32) {
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

// parseSelectors will parse the PodSelector
// or the NameSpaceSelector of a policy.
// It returns the appropriate rules for the specified pods
func (d *K8sPolicyParser) parseSelectors(podSelector, namespaceSelector *meta_v1.LabelSelector, namespace, direction string) (pcn_types.ParsedRules, error) {

	// init
	rules := pcn_types.ParsedRules{
		Ingress: []k8sfirewall.ChainRule{},
		Egress:  []k8sfirewall.ChainRule{},
	}

	// First build the query
	podQuery, nsQuery, err := d.buildPodQueries(podSelector, namespaceSelector, namespace)
	if err != nil {
		return rules, err
	}

	// Now get the pods
	podsFound, err := d.podController.GetPods(podQuery, nsQuery, nil)
	if err != nil {
		return rules, fmt.Errorf("Error while trying to get pods %s", err.Error())
	}

	// Now build the pods
	for _, pod := range podsFound {
		parsed := pcn_types.ParsedRules{}
		podIPs := []string{pod.Status.PodIP, utils.GetPodVirtualIP(d.vPodsRange, pod.Status.PodIP)}

		for _, podIP := range podIPs {
			if len(podIP) == 0 {
				continue
			}
			_parsed := pcn_types.ParsedRules{}

			if direction == "ingress" {
				_parsed = d.getConnectionTemplate(direction, podIP, "", pcn_types.ActionForward, []pcn_types.ProtoPort{})
			} else {
				_parsed = d.getConnectionTemplate(direction, "", podIP, pcn_types.ActionForward, []pcn_types.ProtoPort{})
			}

			parsed.Ingress = append(parsed.Ingress, _parsed.Ingress...)
			parsed.Egress = append(parsed.Egress, _parsed.Egress...)
		}

		rules.Ingress = append(rules.Ingress, parsed.Ingress...)
		rules.Egress = append(rules.Egress, parsed.Egress...)
	}

	return rules, nil
}

// buildPodQueries builds the queries to be directed to the pod controller,
// in order to get the desired pods.
func (d *K8sPolicyParser) buildPodQueries(podSelector, namespaceSelector *meta_v1.LabelSelector, namespace string) (*pcn_types.ObjectQuery, *pcn_types.ObjectQuery, error) {
	// Init
	var queryPod *pcn_types.ObjectQuery
	var queryNs *pcn_types.ObjectQuery

	// If podSelector is empty (len = 0): select everything and then forward

	// Build the query
	if podSelector != nil {

		// This is not supported yet...
		if podSelector.MatchExpressions != nil {
			return nil, nil, errors.New("MatchExpressions on pod selector is not supported yet")
		}

		queryPod = utils.BuildQuery("", podSelector.MatchLabels)
	}

	// Namespace selector
	if namespaceSelector != nil {

		// Match expressions?
		if namespaceSelector.MatchExpressions != nil {
			// This is not supported yet...
			return nil, nil, errors.New("MatchExpressions on namespace selector is not supported yet")
		}

		queryNs = utils.BuildQuery("", namespaceSelector.MatchLabels)
	} else {
		// If namespace selector is nil,
		// we're going to use the one we found on the policy
		if len(namespace) < 0 {
			return nil, nil, errors.New("Namespace name not provided")
		}

		queryNs = utils.BuildQuery(namespace, nil)
	}

	return queryPod, queryNs, nil
}

// DoesPolicyAffectPod checks if the provided policy affects the provided pod,
// returning TRUE if it does
func (d *K8sPolicyParser) DoesPolicyAffectPod(policy *networking_v1.NetworkPolicy, pod *core_v1.Pod) bool {

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

// getConnectionTemplate builds a rule template based on connections
func (d *K8sPolicyParser) getConnectionTemplate(direction, src, dst, action string, ports []pcn_types.ProtoPort) pcn_types.ParsedRules {
	// The rules to return
	rulesToReturn := pcn_types.ParsedRules{}

	// --- Ingress direction
	if direction == "ingress" {
		rulesToReturn.Ingress = []k8sfirewall.ChainRule{
			k8sfirewall.ChainRule{
				Src:       src,
				Dst:       dst,
				Action:    action,
				Conntrack: pcn_types.ConnTrackNew,
			},
		}
	} else {
		// --- Egress direction
		rulesToReturn.Egress = []k8sfirewall.ChainRule{
			k8sfirewall.ChainRule{
				Src:       dst,
				Dst:       src,
				Action:    action,
				Conntrack: pcn_types.ConnTrackNew,
			},
		}
	}

	// Are there any ports?
	if len(ports) > 0 {
		rulesToReturn = d.insertPorts(rulesToReturn.Ingress, rulesToReturn.Egress, ports)
	}

	return rulesToReturn
}

// buildActionKey returns a key to be used in the firewall actions
// (to know how they should react to a pod event)
func (d *K8sPolicyParser) buildActionKey(podLabels, nsLabels map[string]string, nsName string) string {
	key := ""

	// BTW, pods and namespaces usally have very few keys
	// (e.g.: including those appended by k8s as well, they should be less than 10)

	//-------------------------------------
	// Namespace
	//-------------------------------------

	// Namespace name always has precedence over labels
	if len(nsName) > 0 {
		key += "nsName:" + nsName
	} else {

		if len(nsLabels) > 0 {
			key += "nsLabels:" + utils.ImplodeLabels(nsLabels, ",", true)
		} else {
			key += "nsName:"
		}
	}

	key += "|"

	//-------------------------------------
	// Pod
	//-------------------------------------

	// Pod labels
	key += "podLabels:"
	if len(podLabels) == 0 {
		return key
	}

	key += utils.ImplodeLabels(podLabels, ",", true)
	return key
}

// BuildActions builds actions that are going to be used by firewalls
// so they know how to react to pods.
func (d *K8sPolicyParser) BuildActions(ingress []networking_v1.NetworkPolicyIngressRule, egress []networking_v1.NetworkPolicyEgressRule, currentNamespace string) []pcn_types.FirewallAction {
	fwActions := []pcn_types.FirewallAction{}

	selectorsChecker := func(podSelector, namespaceSelector *meta_v1.LabelSelector) (bool, map[string]string, map[string]string) {
		// If no selectors, then don't do anything
		if podSelector == nil && namespaceSelector == nil {
			return false, nil, nil
		}

		// Matchexpression is not supported
		if (podSelector != nil && len(podSelector.MatchExpressions) > 0) ||
			(namespaceSelector != nil && len(namespaceSelector.MatchExpressions) > 0) {
			return false, nil, nil
		}

		p := map[string]string{}
		n := map[string]string{}
		if podSelector != nil {
			p = podSelector.MatchLabels
		}

		if namespaceSelector != nil {
			if namespaceSelector.MatchLabels != nil {
				n = namespaceSelector.MatchLabels
			}
		} else {
			n = nil
		}

		return true, p, n
	}

	//-------------------------------------
	// Ingress
	//-------------------------------------

	if ingress != nil {
		for _, i := range ingress {

			ports := d.parsePorts(i.Ports)

			for _, f := range i.From {
				action := pcn_types.FirewallAction{}

				ok, pod, ns := selectorsChecker(f.PodSelector, f.NamespaceSelector)

				if ok {
					action.PodLabels = pod
					action.NamespaceLabels = ns
					if ns == nil {
						action.NamespaceLabels = map[string]string{}
						action.NamespaceName = currentNamespace
					}

					action.Templates = d.getConnectionTemplate("egress", "", "", pcn_types.ActionForward, ports)
					action.Key = d.buildActionKey(action.PodLabels, action.NamespaceLabels, action.NamespaceName)
					fwActions = append(fwActions, action)
				}
			}
		}
	}

	//-------------------------------------
	// Egress
	//-------------------------------------

	if egress != nil {
		for _, e := range egress {

			ports := d.parsePorts(e.Ports)

			for _, t := range e.To {

				action := pcn_types.FirewallAction{}
				ok, pod, ns := selectorsChecker(t.PodSelector, t.NamespaceSelector)

				if ok {
					action.PodLabels = pod
					action.NamespaceLabels = ns
					if ns == nil {
						action.NamespaceLabels = map[string]string{}
						action.NamespaceName = currentNamespace
					}

					action.Templates = d.getConnectionTemplate("ingress", "", "", pcn_types.ActionForward, ports)
					action.Key = d.buildActionKey(action.PodLabels, action.NamespaceLabels, action.NamespaceName)
					fwActions = append(fwActions, action)
				}
			}
		}
	}
	return fwActions
}
