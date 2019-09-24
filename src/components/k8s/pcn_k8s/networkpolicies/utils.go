package networkpolicies

import (
	"strings"

	pcn_controllers "github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/controllers"
	parsers "github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/networkpolicies/parsers"
	pcn_fw "github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/networkpolicies/pcn_firewall"
	pcn_types "github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/types"
	"github.com/polycube-network/polycube/src/components/k8s/utils"
)

func checkFwManger(policy *pcn_types.ParsedPolicy, fw pcn_fw.PcnFirewallManager) bool {
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

	if utils.AreLabelsContained(policy.Subject.Query.Labels, podLabels) {
		return true
	}

	return false
}

func buildRules(policy *pcn_types.ParsedPolicy) pcn_types.ParsedRules {
	rules := pcn_types.ParsedRules{}

	// Restricts all?
	if strings.HasSuffix(policy.Action, "-all") {
		return policy.Templates
	}

	// Is IPBlock?
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

	// Pod?
	podPeer := func() pcn_types.ParsedRules {
		rulesToReturn := pcn_types.ParsedRules{}

		// Get its services, but only if outgoing
		if policy.Direction == pcn_types.PolicyOutgoing {
			services, err := pcn_controllers.Services().List(policy.Peer.Peer, policy.Peer.Namespace)
			if err != nil {
				logger.Warningf("Could not get services while building rules. Going to skip.")
			} else {
				// For each service that applies to this pod (usually one,
				// but who knows), insert also the service's Cluster IP
				for _, serv := range services {
					rulesToReturn.Outgoing = append(rulesToReturn.Outgoing, parsers.FillTemplates("", serv.Spec.ClusterIP, policy.Templates.Outgoing)...)
				}
			}
		}

		// Get the pods list
		pods, err := pcn_controllers.Pods().List(policy.Peer.Peer, policy.Peer.Namespace, nil)
		if err != nil {
			logger.Errorf("Could not find peers for policy %s. Will stop here.", policy.Name)
			return pcn_types.ParsedRules{}
		}

		// For each pod found, generate the rules
		for _, pod := range pods {
			ips := []string{pod.Status.PodIP}
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

	if len(policy.Peer.IPBlock) > 0 {
		rules = ipbLock()
	} else {
		rules = podPeer()
	}

	return rules
}
