package networkpolicies

import (
	//	TODO-ON-MERGE: change these two to the polycube path

	"sync"

	pcn_controllers "github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/controllers"
	pcn_firewall "github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/networkpolicies/pcn_firewall"
	pcn_types "github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/types"
	k8sfirewall "github.com/SunSince90/polycube/src/components/k8s/utils/k8sfirewall"
	core_v1 "k8s.io/api/core/v1"
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

	//-------------------------------------
	//	Subscribe to default policies events
	//-------------------------------------

	manager.defaultPolicyParser = newDefaultPolicyParser(podController, firewallManager, &manager)

	//	Deploy a new default policy
	dnpc.Subscribe(pcn_types.New, manager.DeployDefaultPolicy)

	//	Remove a default policy
	dnpc.Subscribe(pcn_types.Delete, manager.RemoveDefaultPolicy)

	//	Update a policy
	dnpc.Subscribe(pcn_types.Delete, manager.UpdateDefaultPolicy)

	//-------------------------------------
	//	Subscribe to pod events
	//-------------------------------------

	//podController.Subscribe(pcn_types.New, manager.checkNewPod)
	podController.Subscribe(pcn_types.Update, manager.checkNewPod)
	podController.Subscribe(pcn_types.Delete, manager.deleteFirewall)

	return &manager
}

func (manager *NetworkPolicyManager) DeployDefaultPolicy(policy *networking_v1.NetworkPolicy) {
	var l = log.WithFields(log.Fields{
		"by":     PM,
		"method": "DeployDefaultPolicy",
	})
	l.Infoln("Going to deploy default policy", policy.Name)

	manager.defaultPolicyParser.Parse(policy, true)
	//	UPDATE: ignore the returned value, we don't need it for now.
	/*for _, pods := range manager.defaultPolicyParser.Parse(policy, true) {

		manager.Lock()
		for podID := range pods {
			manager.deployedPolicies[policy.Name] = append(manager.deployedPolicies[policy.Name], podID)
		}
		manager.Unlock()
	}*/

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

	//podsInPolicy := []k8s_types.UID{}

	for _, pods := range podsAffected {
		for _, pod := range pods {
			fw := manager.firewallManager.Get(pod.Pod)

			if fw != nil {
				fw.CeasePolicy(policy.Name)

				//	Remove this pod from the deployed policies
				//	UPDATE: read below.
				//podsInPolicy = append(podsInPolicy, pod.Pod.UID)
			}
		}
	}

	//	UPDATE: commenting this for now because it is not needed.
	/*manager.Lock()
	defer manager.Unlock()

	//	Some of those policies were not removed successfully?
	if len(podsInPolicy) > 0 {
		l.Warningln(policy.Name, "was not completely removed")
		manager.deployedPolicies[policy.Name] = podsInPolicy
	} else {
		l.Debugln(policy.Name, "was successfully removed")
		delete(manager.deployedPolicies, policy.Name)
	}*/
}

func (manager *NetworkPolicyManager) UpdateDefaultPolicy(policy *networking_v1.NetworkPolicy) {

	manager.RemoveDefaultPolicy(policy)
	manager.DeployDefaultPolicy(policy)
}

func (manager *NetworkPolicyManager) checkNewPod(pod *core_v1.Pod) {
	var l = log.WithFields(log.Fields{
		"by":     PM,
		"method": "checkNewPod",
	})
	l.Debugln("Going to check if new pod needs policies applied")

	if pod.Status.Phase != core_v1.PodRunning {
		log.Debugln("Pod", pod.Name, "is in", pod.Status.Phase, "so, I'm not going to check it.")
		return
	}
	log.Debugln("Pod", pod.Name, "is in", pod.Status.Phase)
	lastCondition := pod.Status.Conditions[len(pod.Status.Conditions)-1]
	log.Debugf("last condition: %+v\n", lastCondition)

	//	First start the firewall
	manager.firewallManager.GetOrCreate(*pod)

	/*	This is going to be tricky, so here is a brief explanation.
		1) We must see if there are policies that must be applied to this pod.
		2) We must see if there are policies which target this pod: there may be some pods which can accept connections from this new pod,
			but since it is new, they don't have this pod in their rules list.
	*/

	//	Default Policies
	for _, k8sPolicy := range manager.dnpc.GetPolicy("*") {

		//	Does this policy affect this pod?
		go func(currentPolicy networking_v1.NetworkPolicy) {
			if manager.defaultPolicyParser.DoesPolicyAffectPod(&currentPolicy, pod) {

				//	Deploy the policy just for this pod
				ingress, egress := manager.defaultPolicyParser.ParsePolicyTypes(&currentPolicy.Spec)
				ingressChain, egressChain := manager.defaultPolicyParser.ParseRules(ingress, egress, pod.Namespace)
				ingressChain, egressChain = manager.defaultPolicyParser.FillChains(pcn_types.Pod{
					Pod:  *pod,
					Veth: "",
				}, ingressChain, egressChain)
				fw := manager.firewallManager.GetOrCreate(*pod)
				if fw == nil {
					l.Panicln("Could not get firewall fw-", pod.Name, ". Will stop here.")
					return
				}
				fw.EnforcePolicy(currentPolicy.Name, ingressChain.Rule, egressChain.Rule)
			}
		}(k8sPolicy)

		//	Does this policy target this pod in some way?
		go func(currentPolicy networking_v1.NetworkPolicy) {
			iRules, eRules := manager.defaultPolicyParser.DoesPolicyTargetPod(&currentPolicy, pod)

			if len(iRules) > 0 || len(eRules) > 0 {
				for _, fw := range manager.firewallManager.GetAll() {
					if fw.ImplementsPolicy(currentPolicy.Name) {
						fw.EnforcePolicy(currentPolicy.Name, iRules, eRules)
					}
				}
			}
		}(k8sPolicy)
	}

}

func (manager *NetworkPolicyManager) deleteFirewall(pod *core_v1.Pod) {
	var l = log.WithFields(log.Fields{
		"by":     PM,
		"method": "deleteFirewall",
	})
	l.Debugln("Going to destroy the firewall for the pod")

	fw := manager.firewallManager.GetOrCreate(*pod)
	if fw != nil {
		fw.Destroy()
	}
}
