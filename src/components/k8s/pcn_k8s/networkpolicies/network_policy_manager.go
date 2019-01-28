package networkpolicies

import (

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
	supportedProtocols = "TCP,UDP,ICMP"
)

func NewNetworkPolicyManager(dnpc *controllers.DefaultNetworkPolicyController, podController controllers.PodController) *NetworkPolicyManager {

	//https://play.golang.org/p/0AaBhB1MHBc

	//	Create the resource
	manager := &NetworkPolicyManager{
		logBy: "Network Policy Manager",
	}

	//	For use with k8s
	if dnpc != nil {
		manager.dnpc = dnpc
		//	Let me listen for default network policies deployments
		dnpc.Subscribe(events.New, manager.CreateNewFirewallFromDefaultPolicy)
		//dnpc.Subscribe(events.Update, manager.ParseDefaultPolicy)
		//dnpc.Subscribe(events.Delete, manager.ParseDefaultPolicy)
	}

	if podController != nil {
		manager.podController = podController
		//podController.Subscribe()
	}

	return manager
}

func (manager *NetworkPolicyManager) CreateNewFirewallFromDefaultPolicy(policy *networking_v1.NetworkPolicy) {

	//	Get the firewall instance
	//firewall, err := manager.ParseDefaultPolicy(policy)
}

func (manager *NetworkPolicyManager) ParseDefaultPolicy(policy *networking_v1.NetworkPolicy) (*k8sfirewall.Firewall, error) {

	var l = log.WithFields(log.Fields{
		"by":     manager.logBy,
		"method": "ParseDefaultPolicy()",
	})

	generatedFirewall := &k8sfirewall.Firewall{
		Chain: []k8sfirewall.Chain{
			k8sfirewall.Chain{
				Default_: "drop",
				Name:     "egress",
			},
		},
	}

	var ingress []networking_v1.NetworkPolicyIngressRule
	var egress []networking_v1.NetworkPolicyEgressRule

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

	//	Documentation is not very specific if PolicyTypes can be [], so I made this dumb piece of code just in case
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

	//-------------------------------------
	//	Parse the ingress rules
	//-------------------------------------

	if ingress != nil {
		generatedRules := manager.parseDefaultIngressRules(spec.Ingress, namespace)
		generatedFirewall.Chain = append(generatedFirewall.Chain, generatedRules)
	}

	//-------------------------------------
	//	Parse the egress rules
	//-------------------------------------

	if egress != nil {
		generatedRules := manager.parseDefaultEgressRules(spec.Egress, namespace)
		generatedFirewall.Chain = append(generatedFirewall.Chain, generatedRules)
	}

	l.Debugf("generated firewall: %+v", generatedFirewall)
	return generatedFirewall, nil
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

		/*for _, port := range rule.Ports {

			//	If protocol is nil, then we have to get all protocols
			if port.Protocol == nil {

				//	If the port is not nil, default port is not 0
				var defaultPort int32
				if port.Port != nil {
					defaultPort = int32(port.Port.IntValue())
				}

				generatedPorts = []struct {
					protocol string
					port     int32
				}{
					{"TCP", defaultPort},
					{"UDP", defaultPort},
				}
			} else {
				//	else parse the protocol
				supported, proto, port := manager.parseDefaultProtocolAndPort(port)

				//	Our firewall does not support SCTP, so we check if protocol is supported
				if supported {

					var protoPort = struct {
						protocol string
						port     int32
					}{
						proto,
						port,
					}

					generatedPorts = append(generatedPorts, protoPort)
				}
			}
		}*/

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

		//	From is {} ?
		if rule.From == nil {
			l.Infoln("From is nil: ALL resources are allowed")
			incompleteRules = append(incompleteRules, k8sfirewall.ChainRule{
				Action: "forward",
				//	TODO: check this.
				Src: "0",
			})
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
				incompleteRules = append(incompleteRules, manager.parseDefaultIPBlock(from.IPBlock)...)
			}

			//	PodSelector Or NamespaceSelector?
			if from.PodSelector != nil || from.NamespaceSelector != nil {

				rulesGot, err := manager.parseDefaultSelectors(from.PodSelector, from.NamespaceSelector, namespace)

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
				Src: "0",
			})
		}

		for i := 0; i < len(rule.To) && proceed; i++ {
			to := rule.To[i]

			//	IPBlock?
			if to.IPBlock != nil {
				incompleteRules = append(incompleteRules, manager.parseDefaultIPBlock(to.IPBlock)...)
			}

			//	PodSelector Or NamespaceSelector?
			if to.PodSelector != nil || to.NamespaceSelector != nil {

				rulesGot, err := manager.parseDefaultSelectors(to.PodSelector, to.NamespaceSelector, namespace)

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
					Src:     rule.Src,
					L4proto: generatedPort.protocol,
					Sport:   generatedPort.port,
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

func (manager *NetworkPolicyManager) parseDefaultIPBlock(block *networking_v1.IPBlock) []k8sfirewall.ChainRule {

	var l = log.WithFields(log.Fields{
		"by":     manager.logBy,
		"method": "parseDefaultIPBlock()",
	})
	var rules []k8sfirewall.ChainRule

	l.Debugln("Parsing IPBlock...")

	//	Add the default one
	rules = append(rules, k8sfirewall.ChainRule{
		Src:    block.CIDR,
		Action: "forward",
	})

	//	Loop through all exceptions
	for _, exception := range block.Except {
		rules = append(rules, k8sfirewall.ChainRule{
			Src:    exception,
			Action: "drop",
		})
	}

	l.Debugln("Finished parsing IPBlock. Generated rules: ")
	l.Debugf("%+v\n", rules)

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

func (manager *NetworkPolicyManager) parseDefaultSelectors(podSelector, namespaceSelector *meta_v1.LabelSelector, namespace string) ([]k8sfirewall.ChainRule, error) {
	var l = log.WithFields(log.Fields{
		"by":     manager.logBy,
		"method": "parseDefaultSelectors()",
	})
	var rules []k8sfirewall.ChainRule
	var query podquery.Query
	action := "forward"

	l.Debugln("Parsing selectors...")

	//	Build the query
	if podSelector != nil {

		//	This is not supported yet...
		if podSelector.MatchExpressions != nil {
			return rules, errors.New("MatchExpressions on pod selector is not supported yet")
		}

		//	Empty labels means "select everything"
		//	Nil labels means do not select anything. Which, for us, means deny access to those pods (see below)
		if len(podSelector.MatchLabels) < 1 || podSelector.MatchLabels == nil {

			//	Are you here because match labels is nil?
			if podSelector.MatchLabels == nil {
				action = "drop"
			}

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
			return rules, errors.New("MatchExpressions on namespace selector is not supported yet")
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
	}

	for _, pod := range podsFound {
		rules = append(rules, k8sfirewall.ChainRule{
			Src:    pod.Pod.Status.PodIP,
			Action: action,
		})
	}

	return rules, nil
}
