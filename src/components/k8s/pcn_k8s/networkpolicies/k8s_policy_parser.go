package networkpolicies

import (
	"errors"
	"fmt"
	"net"

	"github.com/polycube-network/polycube/src/components/k8s/utils"

	pcn_controllers "github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/controllers"
	pcn_types "github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/types"
	k8sfirewall "github.com/polycube-network/polycube/src/components/k8s/utils/k8sfirewall"
	log "github.com/sirupsen/logrus"
	core_v1 "k8s.io/api/core/v1"
	networking_v1 "k8s.io/api/networking/v1"
	meta_v1 "k8s.io/apimachinery/pkg/apis/meta/v1"
	"k8s.io/client-go/kubernetes"
)

// PcnK8sPolicyParser is the default policy (e.g.: kubernetes') parser
type PcnK8sPolicyParser interface {
	// ParsePolicyTypes will parse the policy type of the policy
	// and return the appropriate rules and the type of this policy
	ParsePolicyTypes(policySpec *networking_v1.NetworkPolicySpec) ([]networking_v1.NetworkPolicyIngressRule, []networking_v1.NetworkPolicyEgressRule, string)
}

// K8sPolicyParser is the implementation of the default parser
type K8sPolicyParser struct {
	podController pcn_controllers.PodController
	nsController  pcn_controllers.NamespaceController
	vPodsRange    *net.IPNet
	clientset     kubernetes.Interface
}

// newK8sPolicyParser starts a new k8s policy parser
func newK8sPolicyParser(clientset kubernetes.Interface, podController pcn_controllers.PodController, nsController pcn_controllers.NamespaceController, vPodsRange *net.IPNet) *K8sPolicyParser {
	return &K8sPolicyParser{
		podController: podController,
		nsController:  nsController,
		vPodsRange:    vPodsRange,
		clientset:     clientset,
	}
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
			edited.Dport = generatedPort.Port
			edited.L4proto = generatedPort.Protocol
			parsed.Ingress = append(parsed.Ingress, edited)
		}
	}

	// -- Egress chain
	for i := 0; i < len(generatedEgressRules); i++ {
		rule := generatedEgressRules[i]
		for _, generatedPort := range generatedPorts {
			edited := rule
			edited.Sport = generatedPort.Port
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

	// Helper function to avoid repetition
	getIPRule := func(ip, action string) pcn_types.ParsedRules {
		src := ""
		dst := ""

		if k8sDirection == pcn_types.Outgoing {
			dst = ip
		} else {
			src = ip
		}

		return d.getConnectionTemplate(k8sDirection, src, dst, action, []pcn_types.ProtoPort{})
	}

	// Loop through all exceptions
	for _, exception := range block.Except {
		exceptionRule := getIPRule(exception, pcn_types.ActionDrop)
		parsed.Ingress = append(parsed.Ingress, exceptionRule.Ingress...)
		parsed.Egress = append(parsed.Egress, exceptionRule.Egress...)
	}

	// Add the default one
	cidrRules := getIPRule(block.CIDR, pcn_types.ActionForward)
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

	// First, set the service cluster IP
	if direction == pcn_types.Outgoing {
		ips := d.getServiceClusterIPs(podQuery, nsQuery)

		for _, ip := range ips {
			_parsed := d.getConnectionTemplate(direction, "", ip, pcn_types.ActionForward, []pcn_types.ProtoPort{})
			rules.Ingress = append(rules.Ingress, _parsed.Ingress...)
			rules.Egress = append(rules.Egress, _parsed.Egress...)
		}
	}

	// Now build the pods
	for _, pod := range podsFound {
		if len(pod.Status.PodIP) == 0 {
			continue
		}

		parsed := pcn_types.ParsedRules{}
		podIPs := []string{pod.Status.PodIP}

		if direction == pcn_types.Incoming {
			podIPs = append(podIPs, utils.GetPodVirtualIP(d.vPodsRange, pod.Status.PodIP))
		}

		for _, podIP := range podIPs {

			// Prepare the ips (these are defaults)
			src := ""
			dst := ""

			// Fill one or the other depending on the direction
			if direction == pcn_types.Outgoing {
				dst = podIP
			} else {
				src = podIP
			}

			_parsed := d.getConnectionTemplate(direction, src, dst, pcn_types.ActionForward, []pcn_types.ProtoPort{})
			parsed.Ingress = append(parsed.Ingress, _parsed.Ingress...)
			parsed.Egress = append(parsed.Egress, _parsed.Egress...)
		}

		rules.Ingress = append(rules.Ingress, parsed.Ingress...)
		rules.Egress = append(rules.Egress, parsed.Egress...)
	}

	return rules, nil
}

func (d *K8sPolicyParser) getServiceClusterIPs(podQuery, nsQuery *pcn_types.ObjectQuery) []string {
	l := log.New().WithFields(log.Fields{"by": KPP, "method": "getServiceClusterIPs"})

	// Get the namespaces
	namespaces, err := d.nsController.GetNamespaces(nsQuery)
	if err != nil {
		l.Errorf("Could not find namespaces. Returning empty cluster IPs.")
		return []string{}
	}

	ips := []string{}

	// Get the cluster IPs
	// NOTE: if port is specified, this will also get cluster ips that do not
	// have anything to do with that port
	for _, ns := range namespaces {
		list, err := d.clientset.CoreV1().Services(ns.Name).List(meta_v1.ListOptions{})
		if err != nil {
			l.Errorf("Error while trying to get services for namespace %s: %s.", ns.Namespace, err)
			continue
		}

		// Loop through all services
		for _, service := range list.Items {
			// Skip this if it doen't match
			if podQuery != nil && !utils.AreLabelsContained(service.Spec.Selector, podQuery.Labels) {
				continue
			}

			ips = append(ips, service.Spec.ClusterIP)
		}
	}

	return ips
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

// getConnectionTemplate builds a rule template based on connections
func (d *K8sPolicyParser) getConnectionTemplate(direction, src, dst, action string, ports []pcn_types.ProtoPort) pcn_types.ParsedRules {
	// The rules to return
	rulesToReturn := pcn_types.ParsedRules{}

	// The rule to fill
	rule := k8sfirewall.ChainRule{
		Src:       src,
		Dst:       dst,
		Action:    action,
		Conntrack: pcn_types.ConnTrackNew,
	}

	// --- Ingress direction
	if direction == pcn_types.Outgoing {
		rulesToReturn.Ingress = []k8sfirewall.ChainRule{
			rule,
		}
	} else {
		// --- Egress direction
		rulesToReturn.Egress = []k8sfirewall.ChainRule{
			rule,
		}
	}

	// Are there any ports?
	if len(ports) > 0 {
		rulesToReturn = d.insertPorts(rulesToReturn.Ingress, rulesToReturn.Egress, ports)
	}

	return rulesToReturn
}
