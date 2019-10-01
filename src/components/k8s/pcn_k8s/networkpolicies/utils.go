package networkpolicies

import (
	"strings"

	pcn_controllers "github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/controllers"
	parsers "github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/networkpolicies/parsers"
	pcn_fw "github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/networkpolicies/pcn_firewall"
	v1beta "github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/pkg/apis/polycube.network/v1beta"
	pcn_types "github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/types"
	"github.com/polycube-network/polycube/src/components/k8s/utils"
	core_v1 "k8s.io/api/core/v1"
)

// checkFwManager checks if the provided policy should be enforced
// by the provided the firewall manager
func checkFwManager(policy *pcn_types.ParsedPolicy, fw pcn_fw.PcnFirewallManager) bool {
	podLabels, ns := fw.Selector()

	// If the namespaces are different, then skip this
	if ns != policy.Subject.Namespace {
		return false
	}

	// If you're here, it means that the namespaces is ok.
	// We now need to check the labels
	if policy.Subject.Query == nil {
		return true
	}

	// Are the labels ok?
	if utils.AreLabelsContained(policy.Subject.Query.Labels, podLabels) {
		return true
	}

	return false
}

// buildRules gets the pod or the ips from the policy, and generates the rules
// by filling the templates.
func buildRules(policy *pcn_types.ParsedPolicy) pcn_types.ParsedRules {
	rules := pcn_types.ParsedRules{}

	// Restricts all?
	if strings.HasSuffix(policy.Action, "-all") {
		return policy.Templates
	}

	// To keep the function more clean and polished, two internal funcs are
	// created.

	//----------------------------------
	// IP block
	//----------------------------------

	ipbLock := func() pcn_types.ParsedRules {
		rulesToReturn := pcn_types.ParsedRules{}
		for _, ip := range policy.Peer.IPBlock {
			if policy.Direction == pcn_types.PolicyIncoming {
				rulesToReturn.Incoming = append(rulesToReturn.Incoming, parsers.FillTemplates(ip, "", policy.Templates.Incoming)...)
			} else {
				rulesToReturn.Outgoing = append(rulesToReturn.Outgoing, parsers.FillTemplates("", ip, policy.Templates.Outgoing)...)
			}
		}

		return rulesToReturn
	}

	//----------------------------------
	// Pods
	//----------------------------------

	podPeer := func() pcn_types.ParsedRules {
		rulesToReturn := pcn_types.ParsedRules{}
		appliedService := map[string]bool{}

		// Get the pods list
		pods, err := pcn_controllers.Pods().List(policy.Peer.Peer, policy.Peer.Namespace, nil)
		if err != nil {
			logger.Errorf("Could not find peers for policy %s. Will stop here.", policy.Name)
			return pcn_types.ParsedRules{}
		}

		// For each pod found, generate the rules
		for _, pod := range pods {

			// First, insert this pod's service if any
			// Get its services, but only if outgoing
			if policy.Direction == pcn_types.PolicyOutgoing {
				// First, build the key
				podQuery := utils.BuildQuery("", pod.Labels)
				key := utils.BuildObjectKey(policy.Peer.Namespace, "ns") + "|" + utils.BuildObjectKey(podQuery, "pod")

				// Have I already checked this pod's service?
				if _, exists := appliedService[key]; !exists {
					services, err := pcn_controllers.Services().List(nil, policy.Peer.Namespace)
					if err != nil {
						logger.Warningf("Could not get services while building rules. Going to skip it.")
					} else {
						// For each service that applies to this pod (usually one,
						// but who knows), insert also the service's Cluster IP
						for _, serv := range services {
							if len(serv.Spec.Selector) == 0 || !utils.AreLabelsContained(serv.Spec.Selector, pod.Labels) {
								continue
							}
							newPorts := []pcn_types.ProtoPort{}
							// We now have to convert the ports on the policy,
							// to those on the service
							for _, port := range policy.Ports {
								for _, servPort := range serv.Spec.Ports {
									if servPort.TargetPort.IntVal == port.DPort && (strings.ToLower(port.Protocol) == strings.ToLower(string(servPort.Protocol)) || len(port.Protocol) == 0) {
										newPorts = append(newPorts, pcn_types.ProtoPort{
											SPort:    port.SPort,
											DPort:    servPort.Port,
											Protocol: port.Protocol,
										})
									}
								}
							}

							newTemplates := parsers.BuildRuleTemplates(pcn_types.PolicyOutgoing, pcn_types.ActionForward, newPorts)
							rulesToReturn.Outgoing = append(rulesToReturn.Outgoing, parsers.FillTemplates("", serv.Spec.ClusterIP, newTemplates.Outgoing)...)
						}
					}

					// Next iteration, we won't need to check services anymore.
					appliedService[key] = true
				}
			}

			// Now, insert the single pod's ip
			ips := []string{}

			if !policy.PreventDirectAccess {
				// The policy states that the pod should only be accessed by
				// its service...
				ips = append(ips, pod.Status.PodIP)
			}
			if policy.Direction == pcn_types.PolicyIncoming {
				ips = append(ips, utils.GetPodVirtualIP(pod.Status.PodIP))
			}

			for _, ip := range ips {
				if policy.Direction == pcn_types.PolicyIncoming {
					rulesToReturn.Incoming = append(rulesToReturn.Incoming, parsers.FillTemplates(ip, "", policy.Templates.Incoming)...)
				} else {
					rulesToReturn.Outgoing = append(rulesToReturn.Outgoing, parsers.FillTemplates("", ip, policy.Templates.Outgoing)...)
				}
			}
		}

		return rulesToReturn
	}

	//----------------------------------
	// What to do?
	//----------------------------------

	if len(policy.Peer.IPBlock) > 0 {
		rules = ipbLock()
	} else {
		rules = podPeer()
	}

	return rules
}

// getPoliciesForServ gets all the policies for a certain service
func getPoliciesForServ(serv *core_v1.Service) []v1beta.PolycubeNetworkPolicy {
	// The solution does not protect pods in kube-system
	if serv.Namespace == "kube-system" {
		logger.Infof("Service %s belongs to kube-system, so it is going to be skipped. Stopping here.", serv.Name)
		return nil
	}

	if len(serv.Spec.Selector) == 0 {
		logger.Infof("Service %s has no selectors. No point in checking for policies. Stopping here.", serv.Name)
		return nil
	}

	// Get policies
	nsQuery := utils.BuildQuery(serv.Namespace, nil)
	policies, err := pcn_controllers.PcnPolicies().List(nil, nsQuery)
	if err != nil {
		logger.Errorf("Could not get policies on namespace %s.", serv.Namespace)
		return nil
	}
	if len(policies) == 0 {
		logger.Infof("No policies found for namespaces %s. Stopping here.", serv.Namespace)
		return nil
	}

	// Loop through all the found policies and check which one applies to this
	policiesList := []v1beta.PolycubeNetworkPolicy{}
	for _, pol := range policies {
		if pol.ApplyTo.Target == v1beta.ServiceTarget && pol.ApplyTo.WithName == serv.Name {
			policiesList = append(policiesList, pol)
		}
	}

	return policiesList
}
