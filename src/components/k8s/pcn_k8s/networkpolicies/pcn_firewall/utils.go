package pcnfirewall

import (
	pcn_types "github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/types"
)

func dirToChain(direction string) string {
	if direction == pcn_types.PolicyIncoming {
		return "egress"
	}

	return "ingress"
}

func calculateDefaultAction(direction string, policyDirections map[string]string) string {

	count := 0

	for _, dir := range policyDirections {
		if dir == direction {
			count++
		}
	}

	if count == 0 {
		return "forward"
	}

	return "drop"
}

func calculatePolicyOffset(policy pcn_types.ParsedPolicy, base []policyPriority) int {
	mainOffset := 0

	// First, check its parent (the actual k8s or pcn policy) priority
	for _, currentPolicy := range base {
		if policy.ParentPolicy.Priority < currentPolicy.parentPriority {
			break
		}

		if policy.ParentPolicy.Priority == currentPolicy.parentPriority {
			if policy.CreationTime.After(currentPolicy.timestamp) {
				break
			}

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
