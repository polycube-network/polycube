package networkpolicies

import (
	"strings"

	//	TODO-ON-MERGE: change these two to the polycube path
	"github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/controllers"
	events "github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/types/events"

	log "github.com/sirupsen/logrus"
	v1 "k8s.io/api/core/v1"
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
		dnpc:  dnpc,
	}

	//	Let me listen for default network policies deployments
	dnpc.Subscribe(events.New, manager.ParseDefaultPolicy)
	//dnpc.Subscribe(events.Update, manager.ParseDefaultPolicy)
	//dnpc.Subscribe(events.Delete, manager.ParseDefaultPolicy)

	return manager
}

func (manager *NetworkPolicyManager) ParseDefaultPolicy(policy *networking_v1.NetworkPolicy) {

	var l = log.WithFields(log.Fields{
		"by":     manager.logBy,
		"method": "ParseDefaultPolicy()",
	})

	l.Infof("Network Policy Manager is going to parse the default policy")

	//	Get the annotations
	//	TODO: this is going to be used to check for whitelist/blacklist feature
	//policy.ObjectMeta.GetAnnotations()

	//	Get the specs
	spec := policy.Spec

	//-------------------------------------
	//	Parse the ingress rules
	//-------------------------------------

	ingress := spec.Ingress

	//	Apparently, when yaml has Ingress: [] this is called, instead of len() < 1
	if ingress == nil {
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

						/*if *_port.Type == intstr.String {
							port = *_port.IntVal
						} else {
							port = convert the string to int
						}*/
						//	TODO: check if this always works. Or if I must do a type check (like above)
						port = _port.IntVal

						l.Infof("protocol is %s and port is %d", protocol, port)
					}
				}
			}
		}
	}
}
