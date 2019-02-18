package networkpolicies

import (
	//	TODO-ON-MERGE: change these two to the polycube path

	"sync"

	pcn_controllers "github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/controllers"
	pcn_firewall "github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/networkpolicies/pcn_firewall"
	pcn_types "github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/types"
	k8sfirewall "github.com/SunSince90/polycube/src/components/k8s/utils/k8sfirewall"
	networking_v1 "k8s.io/api/networking/v1"
	k8s_types "k8s.io/apimachinery/pkg/types"

	log "github.com/sirupsen/logrus"
)

type PcnNetworkPolicyManager interface {
}

type NetworkPolicyManager struct {
	dnpc            *pcn_controllers.DefaultNetworkPolicyController
	podController   pcn_controllers.PodController
	firewallManager pcn_firewall.Manager

	defaultPolicyParser PcnDefaultPolicyParser
	deployedPolicies    map[string][]k8s_types.UID
	sync.Mutex
}

type ParsedPolicy struct {
	ingressChain k8sfirewall.Chain
	egressChain  k8sfirewall.Chain
}

func StartNetworkPolicyManager(dnpc *pcn_controllers.DefaultNetworkPolicyController, podController pcn_controllers.PodController, firewallManager pcn_firewall.Manager) PcnNetworkPolicyManager {
	var l = log.WithFields(log.Fields{
		"by":     PM,
		"method": "StartNetworkPolicyManager()",
	})
	l.Infoln("Starting Network Policy Manager")

	manager := NetworkPolicyManager{
		dnpc:             dnpc,
		podController:    podController,
		firewallManager:  firewallManager,
		deployedPolicies: map[string][]k8s_types.UID{},
	}

	manager.defaultPolicyParser = newDefaultPolicyParser(podController, firewallManager, &manager)

	//	Deploy a new default policy
	dnpc.Subscribe(pcn_types.New, manager.DeployDefaultPolicy)

	//	Remove a default policy
	dnpc.Subscribe(pcn_types.Delete, manager.RemoveDefaultPolicy)

	//	Update a policy
	dnpc.Subscribe(pcn_types.Delete, manager.UpdateDefaultPolicy)

	return &manager
}

func (manager *NetworkPolicyManager) DeployDefaultPolicy(policy *networking_v1.NetworkPolicy) {
	var l = log.WithFields(log.Fields{
		"by":     PM,
		"method": "DeployDefaultPolicy",
	})
	l.Infoln("Going to deploy default policy", policy.Name)

	for _, pods := range manager.defaultPolicyParser.Parse(policy, true) {

		manager.Lock()
		for podID := range pods {
			manager.deployedPolicies[policy.Name] = append(manager.deployedPolicies[policy.Name], podID)
		}
		manager.Unlock()
	}

	l.Debugln(policy.Name, "deployed.")
}

func (manager *NetworkPolicyManager) RemoveDefaultPolicy(policy *networking_v1.NetworkPolicy) {
	var l = log.WithFields(log.Fields{
		"by":     PM,
		"method": "RemoveDefaultPolicy",
	})
	l.Infoln("Going to remove default policy", policy.Name)
	podsAffected, err := manager.defaultPolicyParser.GetPodsAffected(policy)

	if err != nil {
		//	err
		return
	}

	podsInPolicy := []k8s_types.UID{}

	for _, pods := range podsAffected {
		for _, pod := range pods {
			fw := manager.firewallManager.Get(pod.Pod)

			if fw != nil {
				fw.CeasePolicy(policy.Name)

				//	Remove this pod from the deployed policies
				podsInPolicy = append(podsInPolicy, pod.Pod.UID)
			}
		}
	}

	manager.Lock()
	defer manager.Unlock()

	//	Some of those policies were not removed successfully?
	if len(podsInPolicy) > 0 {
		l.Warningln(policy.Name, "was not completely removed")
		manager.deployedPolicies[policy.Name] = podsInPolicy
	} else {
		l.Debugln(policy.Name, "was successfully removed")
		delete(manager.deployedPolicies, policy.Name)
	}
}

func (manager *NetworkPolicyManager) UpdateDefaultPolicy(policy *networking_v1.NetworkPolicy) {

	manager.RemoveDefaultPolicy(policy)
	manager.DeployDefaultPolicy(policy)
}
