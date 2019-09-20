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
	if len(policy.Peer.IPBlock) > 0 {
		for _, ip := range policy.Peer.IPBlock {
			if policy.Direction == pcn_types.PolicyIncoming {
				rules.Incoming = append(rules.Incoming, parsers.FillTemplates(ip, "", policy.Templates.Incoming)...)
			} else {
				rules.Outgoing = append(rules.Outgoing, parsers.FillTemplates("", ip, policy.Templates.Outgoing)...)
			}
		}
	} else {
		pods, err := pcn_controllers.Pods().List(policy.Peer.Peer, policy.Peer.Namespace, nil)
		if err != nil {
			logger.Errorf("Could not find peers for policy %s. Will stop here.", policy.Name)
			return pcn_types.ParsedRules{}
		}

		for _, pod := range pods {
			ips := []string{pod.Status.PodIP, utils.GetPodVirtualIP(pod.Status.PodIP)}

			for _, ip := range ips {
				if policy.Direction == pcn_types.PolicyIncoming {
					rules.Incoming = append(rules.Incoming, parsers.FillTemplates(ip, "", policy.Templates.Incoming)...)
				} else {
					rules.Outgoing = append(rules.Outgoing, parsers.FillTemplates("", ip, policy.Templates.Outgoing)...)
				}
			}
		}
	}

	return rules
}
