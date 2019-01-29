package networkpolicies

import (
	"github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/types/polycubepod"

	//	TODO-ON-MERGE: change these two to the polycube path
	"errors"
	"strings"

	"github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/controllers"
	events "github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/types/events"
	podquery "github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/types/podquery"
	k8sfirewall "github.com/SunSince90/polycube/src/components/k8s/utils/k8sfirewall"

	log "github.com/sirupsen/logrus"
	core_v1 "k8s.io/api/core/v1"
	networking_v1 "k8s.io/api/networking/v1"
	meta_v1 "k8s.io/apimachinery/pkg/apis/meta/v1"
)

type NetworkPolicyManager struct {
	logBy string

	dnpc *controllers.DefaultNetworkPolicyController

	podController controllers.PodController
}

type parsedProtoPort struct {
	protocol string
	port     int32
}

//	TODO: this is just temporary, maybe do this with configMaps?
const (
	//	Just for test
	supportedProtocols = "TCP,UDP"
	//supportedProtocols = "TCP,UDP,ICMP"
)

func NewNetworkPolicyManager(dnpc *controllers.DefaultNetworkPolicyController, podController controllers.PodController) *NetworkPolicyManager {

	var l = log.WithFields(log.Fields{
		"by":     "Network Policy Manager",
		"method": "ParseDefaultPolicy()",
	})

	//https://play.golang.org/p/0AaBhB1MHBc

	//	Create the resource
	manager := &NetworkPolicyManager{
		logBy: "Network Policy Manager",
	}

	//	For use with k8s
	if dnpc != nil {
		manager.dnpc = dnpc
		//	Let me listen for default network policies deployments
		dnpc.Subscribe(events.New, manager.ManageDefaultPolicy)
		//dnpc.Subscribe(events.Update, manager.ParseDefaultPolicy)
		//dnpc.Subscribe(events.Delete, manager.ParseDefaultPolicy)
	}

	if podController != nil {
		manager.podController = podController

		//podController.Subscribe()
	} else {
		l.Warningln("Warning: pod controller is nil, functions involving pods and namespaces won't work")
	}

	return manager
}

func (manager *NetworkPolicyManager) ManageDefaultPolicy(policy *networking_v1.NetworkPolicy) {

	//	Get the firewall instance
	//firewall, err := manager.ParseDefaultPolicy(policy)
}

func (manager *NetworkPolicyManager) ParseDefaultPolicy(policy *networking_v1.NetworkPolicy) ([]k8sfirewall.Firewall, error) {

	var l = log.WithFields(log.Fields{
		"by":     manager.logBy,
		"method": "ParseDefaultPolicy()",
	})

	generatedFirewalls := []k8sfirewall.Firewall{}

	var ingress []networking_v1.NetworkPolicyIngressRule
	var parsedIngressChain k8sfirewall.Chain
	var egress []networking_v1.NetworkPolicyEgressRule
	var parsedEgressChain k8sfirewall.Chain

	l.Infof("Network Policy Manager is going to parse the default policy")

	//-------------------------------------
	//	Parse the basics
	//-------------------------------------

	//	Get the specs
	spec := policy.Spec

	//	Get the namespace (for us, '*' means all namespaces)
	namespace := "*"
	if len(policy.ObjectMeta.Namespace) > 0 {
		namespace = policy.Namespace
	}

	//	Get the pods this policy must be applied to
	podsAffected, err := manager.getPodsFromDefaultSelectors(&policy.Spec.PodSelector, nil, namespace)

	//	Something happened?
	if err != nil {
		return nil, err
	}

	//	No pods found?
	if len(podsAffected) < 1 {
		l.Infoln("Policy deployed but no pods has been found.")
		return []k8sfirewall.Firewall{}, nil
	}

	//	Documentation is not very specific about the possibility of PolicyTypes being [], so I made this dumb piece of code just in case
	if spec.PolicyTypes == nil {
		ingress = spec.Ingress
	} else {
		if len(spec.PolicyTypes) < 1 {
			ingress = spec.Ingress
		} else {
			policyTypes := spec.PolicyTypes

			//	https://godoc.org/k8s.io/api/networking/v1#NetworkPolicySpec =\
			for _, val := range policyTypes {
				switch val {
				case "Ingress":
					ingress = spec.Ingress
				case "Egress":
					egress = spec.Egress
				case "Ingress,Egress":
					ingress = spec.Ingress
					egress = spec.Egress
				}
			}
		}
	}

	//	What to do when both are nil? Documentation is not clear about this...
	//	Let's just return the default action (when everything is empty, k8sfirewall generates a default)
	if ingress == nil && egress == nil {
		for range podsAffected {
			generatedFirewalls = append(generatedFirewalls, k8sfirewall.Firewall{
				Chain: []k8sfirewall.Chain{
					k8sfirewall.Chain{
						Name: "ingress",

						//	TODO: do this with ConfigMaps
						Default_: "drop",
					},
					k8sfirewall.Chain{
						Name: "egress",

						//	TODO: do this with ConfigMaps
						Default_: "drop",
					},
				},
			})
		}

		return generatedFirewalls, nil
	}

	//-------------------------------------
	//	Parse the ingress rules
	//-------------------------------------

	if ingress != nil {
		parsedIngressChain = manager.parseDefaultIngressRules(spec.Ingress, namespace)
		/*generatedRules := manager.parseDefaultIngressRules(spec.Ingress, namespace)
		generatedFirewall.Chain = append(generatedFirewall.Chain, generatedRules)*/
	}

	//-------------------------------------
	//	Parse the egress rules
	//-------------------------------------

	if egress != nil {
		parsedEgressChain = manager.parseDefaultEgressRules(spec.Egress, namespace)
		/*generatedRules := manager.parseDefaultEgressRules(spec.Egress, namespace)
		generatedFirewall.Chain = append(generatedFirewall.Chain, generatedRules)*/
	}

	//-------------------------------------
	//	Put everything together
	//-------------------------------------

	for _, pod := range podsAffected {

		firewall := k8sfirewall.Firewall{}

		//	Insert the ingress rules
		if ingress != nil {

			//	Make a copy to work with, this way we will not iterate through firewall.Chain[i] (we don't know which one is)
			ingressChain := parsedIngressChain

			//	Complete the ingress rules: insert the dst (this pod)
			for i := 0; i < len(ingressChain.Rule); i++ {
				//	Make sure not to target myself
				if ingressChain.Rule[i].Src != pod.Pod.Status.PodIP {
					ingressChain.Rule[i].Dst = pod.Pod.Status.PodIP
				}
			}
			firewall.Chain = append(firewall.Chain, ingressChain)
		}

		//	Insert the egress rules
		if egress != nil {

			//	Make a copy
			egressChain := parsedEgressChain

			//	Complete the egress rules: insert the src (this pod)
			for i := 0; i < len(egressChain.Rule); i++ {
				//	Make sure not to target myself
				if egressChain.Rule[i].Dst != pod.Pod.Status.PodIP {
					egressChain.Rule[i].Src = pod.Pod.Status.PodIP
				}
			}
			firewall.Chain = append(firewall.Chain, egressChain)
		}

		generatedFirewalls = append(generatedFirewalls, firewall)
	}

	//l.Infof("generated firewall: %+v", generatedFirewall)
	return generatedFirewalls, nil
}

func (manager *NetworkPolicyManager) parseDefaultIngressRules(rules []networking_v1.NetworkPolicyIngressRule, namespace string) k8sfirewall.Chain {

	//-------------------------------------
	//	Init
	//-------------------------------------

	var l = log.WithFields(log.Fields{
		"by":     manager.logBy,
		"method": "parseDefaultIngressRules()",
	})
	var ingressChain = k8sfirewall.Chain{
		Name:     "ingress",
		Default_: "drop",
		Rule:     []k8sfirewall.ChainRule{},
	}

	l.Debugln("Network Policy Manager is going to parse the ingress rules")

	//-------------------------------------
	//	Preliminary checks
	//-------------------------------------

	//	Rules is nil?
	if rules == nil {
		l.Debugln("rules is nil: nothing is accepted")

		ingressChain.Default_ = "drop"
		return ingressChain
	}

	//	No rules?
	//	Same as above, but I kept it separate to improve readability
	if len(rules) < 1 {
		l.Debugln("rules is empty: nothing is accepted")

		ingressChain.Default_ = "drop"
		return ingressChain
	}

	//-------------------------------------
	//	Actual parsing
	//-------------------------------------

	for _, rule := range rules {

		//	The rules generated in this iteration.
		var incompleteRules []k8sfirewall.ChainRule

		//	The ports generated in this iteration
		var generatedPorts = []parsedProtoPort{}

		//	Tells if we can go on parsing rules
		var proceed = true

		//-------------------------------------
		//	Protocol & Port
		//-------------------------------------

		//	First, parse the protocol: so that if an unsupported protocol is listed, we silently ignore it.
		//	By doing it this way we don't have to remove rules later on
		generatedPorts = manager.generatePorts(rule.Ports)

		//	If this rule consists of only unsupported protocols, then we can't go on!
		//	If we did, we would be creating wrong rules!
		//	We just need to ignore the rules, for now.
		//	But if there is at least one supported protocol, then we can proceed
		if len(rule.Ports) > 0 && len(generatedPorts) == 0 {
			proceed = false
		}

		//-------------------------------------
		//	Peers
		//-------------------------------------

		//	From is {} ?
		if rule.From == nil {
			l.Debugln("From is nil: ALL resources are allowed")

			//	TODO: is this correct?
			//	If we have at least one port...
			if len(generatedPorts) > 0 {
				incompleteRules = append(incompleteRules, k8sfirewall.ChainRule{
					Action: "forward",
					//	TODO: check this.
					Src: "0",
				})
			} else {
				//	...	otherwise we need to modify the default behaviour
				ingressChain.Default_ = "forward"
			}
		}

		//	From is [] ?
		/* UPDATE: quoting from official documentation:
			"If this field is empty or missing, this rule matches all sources (traffic not restricted by
			source). If this field is present and contains at least on item, this rule
			allows traffic only if the traffic matches at least one item in the from list."
		So I guess that this can't be empty, that's why I removed it. */
		/*if len(rule.From) < 1 {
			l.Infoln("len(From) is 0, NO resources are allowed")
			incompleteRules = append(incompleteRules, k8sfirewall.ChainRule{
				Action: "drop",
			})
		}*/

		for i := 0; i < len(rule.From) && proceed; i++ {
			from := rule.From[i]

			//	IPBlock?
			if from.IPBlock != nil {
				incompleteRules = append(incompleteRules, manager.parseDefaultIPBlock(from.IPBlock, "ingress")...)
			}

			//	PodSelector Or NamespaceSelector?
			if from.PodSelector != nil || from.NamespaceSelector != nil {

				rulesGot, err := manager.parseDefaultSelectors(from.PodSelector, from.NamespaceSelector, namespace, "ingress")

				if err == nil && len(rulesGot) > 0 {
					incompleteRules = append(incompleteRules, rulesGot...)
				}
			}
		}

		l.Debugf("generated rules: %d", len(incompleteRules))

		//-------------------------------------
		//	Finalize
		//-------------------------------------

		//	Finally, for each parsed rule, apply the ports that have been found
		//	But only if you have at least one port
		for i := 0; i < len(incompleteRules) && len(generatedPorts) > 0; i++ {
			rule := incompleteRules[i]
			for _, generatedPort := range generatedPorts {

				ingressChain.Rule = append(ingressChain.Rule, k8sfirewall.ChainRule{
					Src:     rule.Src,
					L4proto: generatedPort.protocol,
					Sport:   generatedPort.port,
					Action:  rule.Action,
				})
			}
		}

		//	No ports in this? Then just append the rules with ports 0
		if len(generatedPorts) < 1 {
			ingressChain.Rule = append(ingressChain.Rule, incompleteRules...)
		}
	}

	return ingressChain
}

func (manager *NetworkPolicyManager) parseDefaultEgressRules(rules []networking_v1.NetworkPolicyEgressRule, namespace string) k8sfirewall.Chain {

	//-------------------------------------
	//	Init
	//-------------------------------------

	var l = log.WithFields(log.Fields{
		"by":     manager.logBy,
		"method": "parseDefaultEgressRules()",
	})
	var egressChain = k8sfirewall.Chain{
		Name:     "egress",
		Default_: "drop",
		Rule:     []k8sfirewall.ChainRule{},
	}

	l.Debugln("Network Policy Manager is going to parse the egress rules")

	//-------------------------------------
	//	Preliminary checks
	//-------------------------------------

	//	Rules is nil?
	if rules == nil {
		l.Debugln("rules is nil: nothing is accepted")

		egressChain.Default_ = "drop"
		return egressChain
	}

	//	No rules?
	//	Same as above, but I kept it separate to improve readability
	if len(rules) < 1 {
		l.Debugln("rules is empty: nothing is accepted")

		egressChain.Default_ = "drop"
		return egressChain
	}

	//-------------------------------------
	//	Actual parsing
	//-------------------------------------

	for _, rule := range rules {

		//	The rules generated in this iteration.
		var incompleteRules []k8sfirewall.ChainRule

		//	The ports generated in this iteration
		var generatedPorts = []parsedProtoPort{}

		//	Tells if we can go on parsing rules
		var proceed = true

		//-------------------------------------
		//	Protocol & Port
		//-------------------------------------

		//	First, parse the protocol: so that if an unsupported protocol is listed, we silently ignore it.
		//	By doing it this way we don't have to remove rules later on
		if len(rule.Ports) > 0 {
			generatedPorts = manager.generatePorts(rule.Ports)
		}

		l.Debugf("generated ports: %d", len(generatedPorts))

		//	If this rule consists of only unsupported protocols, then we can't go on!
		//	If we did, we would be creating wrong rules!
		//	We just need to ignore the rules, for now.
		//	But if there is at least one supported protocol, then we can proceed
		if len(rule.Ports) > 0 && len(generatedPorts) == 0 {
			proceed = false
		}

		//-------------------------------------
		//	Peers
		//-------------------------------------

		//	To is {} ?
		if rule.To == nil {
			l.Infoln("To is nil: ALL resources are allowed")
			incompleteRules = append(incompleteRules, k8sfirewall.ChainRule{
				Action: "forward",
				//	TODO: check this.
				Dst: "0",
			})
		}

		for i := 0; i < len(rule.To) && proceed; i++ {
			to := rule.To[i]

			//	IPBlock?
			if to.IPBlock != nil {
				incompleteRules = append(incompleteRules, manager.parseDefaultIPBlock(to.IPBlock, "egress")...)
			}

			//	PodSelector Or NamespaceSelector?
			if to.PodSelector != nil || to.NamespaceSelector != nil {

				rulesGot, err := manager.parseDefaultSelectors(to.PodSelector, to.NamespaceSelector, namespace, "egress")

				if err == nil && len(rulesGot) > 0 {
					incompleteRules = append(incompleteRules, rulesGot...)
				}
			}
		}

		l.Debugf("generated rules: %d", len(incompleteRules))

		//-------------------------------------
		//	Finalize
		//-------------------------------------

		//	Finally, for each parsed rule, apply the ports that have been found
		//	But only if you have at least one port
		for i := 0; i < len(incompleteRules) && len(generatedPorts) > 0; i++ {
			rule := incompleteRules[i]
			for _, generatedPort := range generatedPorts {
				egressChain.Rule = append(egressChain.Rule, k8sfirewall.ChainRule{
					Dst:     rule.Dst,
					L4proto: generatedPort.protocol,
					Dport:   generatedPort.port,
					Action:  rule.Action,
				})
			}
		}

		//	No ports in this? Then just append the rules with ports 0
		if len(generatedPorts) < 1 {
			egressChain.Rule = append(egressChain.Rule, incompleteRules...)
		}
	}

	return egressChain
}

func (manager *NetworkPolicyManager) parseDefaultIPBlock(block *networking_v1.IPBlock, direction string) []k8sfirewall.ChainRule {

	var l = log.WithFields(log.Fields{
		"by":     manager.logBy,
		"method": "parseDefaultIPBlock()",
	})
	var rules []k8sfirewall.ChainRule

	l.Debugln("Parsing IPBlock...")

	//	Add the default one
	mainBlock := k8sfirewall.ChainRule{
		Action: "forward",
	}

	//	Set the direction
	if direction == "ingress" {
		mainBlock.Src = block.CIDR
	} else {
		mainBlock.Dst = block.CIDR
	}
	rules = append(rules, mainBlock)

	//	Loop through all exceptions
	for _, exception := range block.Except {
		exceptionRule := k8sfirewall.ChainRule{
			Action: "drop",
		}

		if direction == "ingress" {
			exceptionRule.Src = exception
		} else {
			exceptionRule.Dst = exception
		}

		rules = append(rules, exceptionRule)
	}

	return rules
}

func (manager *NetworkPolicyManager) parseDefaultProtocolAndPort(pp networking_v1.NetworkPolicyPort) (bool, string, int32) {

	//	Not sure if port can be nil, but it doesn't harm to do a simple reset
	var port int32
	if pp.Port != nil {
		port = int32(pp.Port.IntValue())
	}

	//	TCP?
	if *pp.Protocol == core_v1.ProtocolTCP {
		return true, "TCP", port
	}

	//	UDP?
	if *pp.Protocol == core_v1.ProtocolUDP {
		return true, "UDP", port
	}

	//	Not supported ¯\_(ツ)_/¯
	return false, "", 0
}

func (manager *NetworkPolicyManager) generatePorts(ports []networking_v1.NetworkPolicyPort) []parsedProtoPort {

	//	Init
	generatedPorts := []parsedProtoPort{}

	for _, port := range ports {

		//	If protocol is nil, then we have to get all protocols
		if port.Protocol == nil {

			//	If the port is not nil, default port is not 0
			var defaultPort int32
			if port.Port != nil {
				defaultPort = int32(port.Port.IntValue())
			}

			for _, protocol := range strings.Split(supportedProtocols, ",") {
				generatedPorts = append(generatedPorts, parsedProtoPort{protocol, defaultPort})
			}

		} else {
			//	else parse the protocol
			supported, proto, port := manager.parseDefaultProtocolAndPort(port)

			//	Our firewall does not support SCTP, so we check if protocol is supported
			if supported {
				generatedPorts = append(generatedPorts, parsedProtoPort{
					protocol: proto,
					port:     port,
				})
			}
		}
	}

	return generatedPorts
}

func (manager *NetworkPolicyManager) parseDefaultSelectors(podSelector, namespaceSelector *meta_v1.LabelSelector, namespace, direction string) ([]k8sfirewall.ChainRule, error) {
	var l = log.WithFields(log.Fields{
		"by":     manager.logBy,
		"method": "parseDefaultSelectors()",
	})
	rules := []k8sfirewall.ChainRule{}
	action := "forward"

	l.Debugln("Going to parse the default selectors....")

	if podSelector != nil && podSelector.MatchLabels == nil {
		action = "drop"
	}

	podsFound, err := manager.getPodsFromDefaultSelectors(podSelector, namespaceSelector, namespace)

	if err != nil {
		return nil, err
	}

	// No need to check for len of podsFound.
	for _, pod := range podsFound {
		rule := k8sfirewall.ChainRule{
			Action: action,
		}

		if direction == "ingress" {
			rule.Src = pod.Pod.Status.PodIP
		} else {
			rule.Dst = pod.Pod.Status.PodIP
		}

		rules = append(rules, rule)
	}

	return rules, nil
}

func (manager *NetworkPolicyManager) getPodsFromDefaultSelectors(podSelector, namespaceSelector *meta_v1.LabelSelector, namespace string) ([]polycubepod.Pod, error) {
	var l = log.WithFields(log.Fields{
		"by":     manager.logBy,
		"method": "getPodsFromDefaultSelectors()",
	})
	podsFound := []polycubepod.Pod{}
	var query podquery.Query

	l.Debugln("Parsing selectors...")

	//	Build the query
	if podSelector != nil {

		//	This is not supported yet...
		if podSelector.MatchExpressions != nil {
			return nil, errors.New("MatchExpressions on pod selector is not supported yet")
		}

		//	Empty labels means "select everything"
		//	Nil labels means do not select anything. Which, for us, means deny access to those pods (see below)
		if len(podSelector.MatchLabels) < 1 || podSelector.MatchLabels == nil {
			query.Pod = []podquery.QueryObject{
				podquery.QueryObject{
					By:   "name",
					Name: "*",
				},
			}
		} else {
			query.Pod = []podquery.QueryObject{
				podquery.QueryObject{
					By:     "labels",
					Labels: podSelector.MatchLabels,
				},
			}
		}
	} else {
		query.Pod = []podquery.QueryObject{
			podquery.QueryObject{
				By:   "name",
				Name: "*",
			},
		}
	}

	//	Namespace selector
	if namespaceSelector != nil {

		//	Match expressions?
		if namespaceSelector.MatchExpressions != nil {
			//	This is not supported yet...
			return nil, errors.New("MatchExpressions on namespace selector is not supported yet")
		}

		if len(namespaceSelector.MatchLabels) > 0 {
			//	Parse the match labels (like for the pod)
			query.Namespace = []podquery.QueryObject{
				podquery.QueryObject{
					By:     "labels",
					Labels: namespaceSelector.MatchLabels,
				},
			}
		} else {
			//	No labels: as per documentation, this means ALL namespaces
			query.Namespace = []podquery.QueryObject{
				podquery.QueryObject{
					By:   "name",
					Name: "*",
				},
			}
		}
	} else {
		//	If namespace selector is nil, we're going to use the one we found on the policy
		query.Namespace = []podquery.QueryObject{
			podquery.QueryObject{
				By:   "name",
				Name: namespace,
			},
		}
	}

	//	Now get the pods
	podsFound, err := manager.podController.GetPods(query)

	if err != nil {
		l.Errorln("error when trying to get pods:", err)
		return nil, errors.New("error when trying to get pods")
	}

	return podsFound, nil
}
