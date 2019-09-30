package pcnfirewall

import (
	pcn_types "github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/types"
)

// dirToChain "converts" the policy direction to the appropriate firewall chain
func dirToChain(direction string) string {
	if direction == pcn_types.PolicyIncoming {
		return pcn_types.ChainIncoming
	}

	return pcn_types.ChainOutgoing
}

// calculatePolicyOffset calculates the offset of the given policy in the list
// of a firewall's enforced policies
func calculatePolicyOffset(policy pcn_types.ParsedPolicy, base []policyPriority) int {
	mainOffset := 0

	// First, check its parent (the actual k8s or pcn policy) priority
	for _, currentPolicy := range base {
		if policy.ParentPolicy.Priority < currentPolicy.parentPriority {
			break
		}

		// The two policies have the same priority.
		// Which one was deployed more recently?
		if policy.ParentPolicy.Priority == currentPolicy.parentPriority {
			if policy.CreationTime.After(currentPolicy.timestamp) {
				break
			}

			// They have been deployed at the same time.
			// Which one has the highest priority?
			if policy.CreationTime.Time == currentPolicy.timestamp {
				if policy.Priority < currentPolicy.priority {
					break
				}
			}
		}

		mainOffset++
	}

	return mainOffset
}
