package networkpolicies

import (

	//	TODO-ON-MERGE: change these two to the polycube path
	"github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/controllers"
	events "github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/types/events"
	k8sfirewall "github.com/SunSince90/polycube/src/components/k8s/utils/k8sfirewall"

	log "github.com/sirupsen/logrus"
	core_v1 "k8s.io/api/core/v1"
	networking_v1 "k8s.io/api/networking/v1"
)

type NetworkPolicyManager struct {
	logBy string

	dnpc *controllers.DefaultNetworkPolicyController
}

func NewNetworkPolicyManager(dnpc *controllers.DefaultNetworkPolicyController) *NetworkPolicyManager {

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

	return manager
}

func (manager *NetworkPolicyManager) CreateNewFirewallFromDefaultPolicy(policy *networking_v1.NetworkPolicy) {

	//	Get the firewall instance
	firewall := manager.ParseDefaultPolicy(policy)
}

func (manager *NetworkPolicyManager) ParseDefaultPolicy(policy *networking_v1.NetworkPolicy) *k8sfirewall.Firewall {

	var l = log.WithFields(log.Fields{
		"by":     manager.logBy,
		"method": "ParseDefaultPolicy()",
	})

	//	TODO: this is just a test. It mocks the pod controller
	//ipsFound := []string{"192.168.1.1", "192.168.10.10", "192.168.122.35"}

	l.Infof("Network Policy Manager is going to parse the default policy")

	//	Get the annotations
	//	TODO: this is going to be used to check for whitelist/blacklist feature
	//policy.ObjectMeta.GetAnnotations()

	//	Get the specs
	spec := policy.Spec

	//-------------------------------------
	//	Parse the ingress rules
	//-------------------------------------

	ingressChain := manager.parseDefaultIngressRules(spec.Ingress)

	l.Debugln("Generated chain after parsing:")
	l.Debugf("%+v\n", ingressChain)
	//ingress := spec.Ingress

	//	Apparently, when yaml has Ingress: [] this is called, instead of len() < 1
	/*if ingress == nil {
		l.Info("Ingress is null: this resource accepts no connections.")
	} else {
		//	Nothing? This means that this resource doesn't accept anything in ingress
		if len(ingress) < 1 {
			l.Info("There are no ingress rules: this resource accepts no connections.")
		} else {
			l.Info("The following rules have been found")

			for _, ingressRule := range ingress {

				//-------------------------------------
				//	Peer Rules
				//-------------------------------------

				//	TODO: check this, because when ingress: - {} is found, this is called, totally different from the case above!!!
				if ingressRule.From == nil {
					l.Info("from is null: nothing can be accessed.")
				} else {

					//	From is specified but is an empty array => nothing is allowed
					if len(ingressRule.From) < 1 {
						l.Info("Found empty array, resource doesn't accept connections")
					} else {
						for _, peer := range ingressRule.From {

							//-------------------------------------
							//	Select an IP block
							//-------------------------------------

							if peer.IPBlock == nil {
								l.Info("No IPBlock has been specified")
							} else {

								IPBlock := peer.IPBlock
								cidr := IPBlock.CIDR

								l.Info("%s specified", cidr)

								if IPBlock.Except != nil && len(IPBlock.Except) > 0 {
									l.Info("The following exceptions have been found")

									for _, exception := range IPBlock.Except {
										l.Info("%s", exception)
									}
								}
							}

							//-------------------------------------
							//	Select pods
							//-------------------------------------

							if peer.PodSelector == nil {
								l.Info("No pod selector has been specified")
							} else {

								podSelector := peer.PodSelector

								//	Get match labels
								//	TODO: same as above
								if podSelector.MatchLabels == nil {
									l.Info("Empty podSelector: I have to select all pods")
								} else {
									l.Info("I found the following match labels rules")
									for key, value := range podSelector.MatchLabels {
										l.Infof("%s => %s", key, value)
									}
								}

								//	Get Expression match
								if len(podSelector.MatchExpressions) < 1 {
									l.Info("There are no match expressions")
								} else {
									for _, selectorRequirements := range podSelector.MatchExpressions {
										//	TODO: selectorRequirements.Operator returns a LabelSelectorOperator, not a string. So check this
										l.Infof("%s => %s: %s", selectorRequirements.Key, selectorRequirements.Operator, strings.Join(selectorRequirements.Values, ","))
									}
								}

							}

							//-------------------------------------
							//	Select namespaces
							//-------------------------------------

							if peer.NamespaceSelector == nil {
								l.Info("No namespaces has been specified ")
							} else {

								//	Commented just to make the compiler shut up
								//nameSpaceSelector := peer.NamespaceSelector

								//	basically do exactly the same as above. So the above piece of code must be done in a function
								l.Info("Gotta parse the namespace, it seems!")
							}
						}
					}
				}

				//-------------------------------------
				//	Port rules
				//-------------------------------------

				if len(ingressRule.Ports) < 1 {
					l.Info("There a no port rules")
				} else {
					for _, portStruct := range ingressRule.Ports {

						//	TODO: protocol is a struct, not a string
						_protocol := portStruct.Protocol
						var protocol string

						//	TODO: port is a
						_port := portStruct.Port
						var port int32

						switch *_protocol {
						case v1.ProtocolTCP:
							protocol = "TCP"
						case v1.ProtocolUDP:
							protocol = "UDP"
						case v1.ProtocolSCTP:
							protocol = "SCTP"
						}

						//if *_port.Type == intstr.String {
						//	port = *_port.IntVal
						//} else {
						//	port = convert the string to int
						//}
						//	TODO: check if this always works. Or if I must do a type check (like above)
						port = _port.IntVal

						l.Infof("protocol is %s and port is %d", protocol, port)
					}
				}
			}
		}
	}*/
}

func (manager *NetworkPolicyManager) parseDefaultIngressRules(rules []networking_v1.NetworkPolicyIngressRule) k8sfirewall.Chain {

	//-------------------------------------
	//	Init
	//-------------------------------------

	var l = log.WithFields(log.Fields{
		"by":     manager.logBy,
		"method": "parseDefaultIngressRules()",
	})
	var ingressChain = k8sfirewall.Chain{
		Name: "ingress",
		Rule: []k8sfirewall.ChainRule{},
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
		var generatedRules []k8sfirewall.ChainRule

		//	The ports generated in this iteration
		var generatedPorts = []struct {
			protocol string
			port     int32
		}{}

		//	Tells if we can go on parsing rules
		var proceed = true

		//	From is {} ?
		if rule.From == nil {

			l.Debugln("from is nil: everything is accepted")

			ingressChain.Default_ = "forward"
			return ingressChain
		}

		//	From is [] ?
		if len(rule.From) < 1 {

			l.Debugln("from is empty: nothing is accepted")

			ingressChain.Default_ = "drop"
			return ingressChain
		}

		//-------------------------------------
		//	Protocol & Port
		//-------------------------------------

		//	First, parse the protocol: so that if an unsupported protocol is listed, we silently ignore it.
		//	By doing it this way we don't have to remove rules later on
		for _, port := range rule.Ports {

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

		for i := 0; i < len(rule.From) && proceed; i++ {
			from := rule.From[i]

			//	IPBlock?
			if from.IPBlock != nil {
				generatedRules = append(generatedRules, manager.parseDefaultIPBlock(from.IPBlock)...)
			}
		}

		l.Debugf("generated rules: %d", len(generatedRules))

		//-------------------------------------
		//	Finalize
		//-------------------------------------

		//	Finally, for each parsed rule, apply the ports that have been found
		for i := 0; i < len(generatedRules); i++ {
			for _, generatedPort := range generatedPorts {
				generatedRules[i].L4proto = generatedPort.protocol
				generatedRules[i].Sport = generatedPort.port
			}
		}

		//	Now add these rules to the chain, please
		ingressChain.Rule = append(ingressChain.Rule, generatedRules...)
	}

	return ingressChain
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

	//	TCP?
	if *pp.Protocol == core_v1.ProtocolTCP {
		return true, "TCP", pp.Port.IntVal
	}

	//	UDP?
	if *pp.Protocol == core_v1.ProtocolUDP {
		return true, "UDP", pp.Port.IntVal
	}

	//	Not supported ¯\_(ツ)_/¯
	return false, "", 0
}
