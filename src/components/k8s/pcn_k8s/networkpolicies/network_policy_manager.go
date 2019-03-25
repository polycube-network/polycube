package networkpolicies

import (
	"sync"

	//	TODO-ON-MERGE: change these to the polycube path
	pcn_controllers "github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/controllers"
	pcn_firewall "github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/networkpolicies/pcn_firewall"
	pcn_types "github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/types"

	log "github.com/sirupsen/logrus"
	core_v1 "k8s.io/api/core/v1"
	networking_v1 "k8s.io/api/networking/v1"
	k8s_types "k8s.io/apimachinery/pkg/types"
)

type PcnNetworkPolicyManager interface {
}

type NetworkPolicyManager struct {
	dnpc            *pcn_controllers.DefaultNetworkPolicyController
	podController   pcn_controllers.PodController
	firewallManager pcn_firewall.Manager

	defaultPolicyParser PcnDefaultPolicyParser
	deployedPolicies    map[string][]k8s_types.UID
	checkedPods         map[k8s_types.UID]*checkedPod
	node                string
	lock                sync.Mutex
	checkPodsLock       sync.Mutex
}

type checkedPod struct {
	lastKnownIP string
	sync.Mutex
}

func StartNetworkPolicyManager(dnpc *pcn_controllers.DefaultNetworkPolicyController, podController pcn_controllers.PodController, firewallManager pcn_firewall.Manager, nodeName string) PcnNetworkPolicyManager {
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
		checkedPods:      map[k8s_types.UID]*checkedPod{},
		node:             nodeName,
	}

	//-------------------------------------
	//	Subscribe to default policies events
	//-------------------------------------

	manager.defaultPolicyParser = newDefaultPolicyParser(podController, firewallManager, nodeName)

	//	Deploy a new default policy
	dnpc.Subscribe(pcn_types.New, manager.DeployDefaultPolicy)

	//	Remove a default policy
	dnpc.Subscribe(pcn_types.Delete, manager.RemoveDefaultPolicy)

	//	Update a policy
	dnpc.Subscribe(pcn_types.Update, manager.UpdateDefaultPolicy)

	//-------------------------------------
	//	Subscribe to pod events
	//-------------------------------------

	//podController.Subscribe(pcn_types.New, manager.checkNewPod)
	podController.Subscribe(pcn_types.Update, manager.checkNewPod)
	podController.Subscribe(pcn_types.Delete, manager.manageDeletedPod)

	return &manager
}

func (manager *NetworkPolicyManager) DeployDefaultPolicy(policy *networking_v1.NetworkPolicy) {

	//	TODO: this probably needs rewrite.
	manager.defaultPolicyParser.Parse(policy, true)
}

func (manager *NetworkPolicyManager) RemoveDefaultPolicy(policy *networking_v1.NetworkPolicy) {
	podsAffected, err := manager.defaultPolicyParser.GetPodsAffected(policy)

	if err != nil {
		//	err
		return
	}

	for _, pods := range podsAffected {
		for _, pod := range pods {

			if pod.Spec.NodeName == manager.node {
				fw := manager.firewallManager.Get(pod)

				if fw != nil {
					fw.CeasePolicy(policy.Name)
				}
			}
		}
	}
}

func (manager *NetworkPolicyManager) UpdateDefaultPolicy(policy *networking_v1.NetworkPolicy) {

	manager.RemoveDefaultPolicy(policy)
	manager.DeployDefaultPolicy(policy)
}

func (manager *NetworkPolicyManager) checkNewPod(pod *core_v1.Pod) {
	var l = log.WithFields(log.Fields{
		"by":     PM,
		"method": "checkNewPod(" + pod.Name + ")",
	})

	//-------------------------------------
	//	Basic Checks
	//-------------------------------------

	//	This is actually useless but who knows....
	if pod == nil {
		l.Errorln("Called with a nil pod! Will stop here.")
		return
	}

	//	Is this pod from the kube-system?
	//	TODO: does checking pods inside kube-system make sense actually?
	if pod.Namespace == "kube-system" {
		l.Infoln("Pod", pod.Name, "belongs to the kube-system namespaces: no point in checking for policies. Will stop here.")
		return
	}

	//	A hack way to detect if the pod is terminating: APIs do not have a way to do that :(
	if pod.ObjectMeta.DeletionTimestamp != nil {
		//l.Debugln("Pod", pod.Name, "is terminating. Will not do anything else.")
		return
	}

	//	Is it running? (if not, it wouldn't have a valid ip yet)
	if pod.Status.Phase != core_v1.PodRunning {
		//l.Debugln("Pod", pod.Name, "is in", pod.Status.Phase, "so I'm not going to check it.")
		return
	}

	//	Have I already checked this before?
	manager.checkPodsLock.Lock()
	checked, ok := manager.checkedPods[pod.UID]
	if !ok {
		checked = &checkedPod{}
		manager.checkedPods[pod.UID] = checked
	}
	manager.checkPodsLock.Unlock()

	checked.Lock()
	defer checked.Unlock()

	//	Already checked before?
	if checked.lastKnownIP == pod.Status.PodIP {
		//l.Debugln("pod", pod.Name, "has already been checked before: no point in checking it again. Will stop here.")
		return
	}

	//l.Debugln("Ok,", pod.Name, "Has never been checked.")

	var policyWait sync.WaitGroup
	policyWait.Add(2)
	k8sPolicies, _ := manager.dnpc.GetPolicies(pcn_types.ObjectQuery{By: "name", Name: "*"})

	//	The most recently deployed policies should have precedence, but the firewall doesn't support insertion in head yet.
	//sort.Slice(k8sPolicies, func(i, j int) bool {return k8sPolicies[i].ObjectMeta.CreationTimestamp < k8sPolicies[j].ObjectMeta.CreationTimestamp})

	/*	This is going to be a bit tricky, so here is a brief explanation.
		1) We must see if there are policies that must be applied to this pod.
		2) We must see if there are policies which target this pod: there may be some pods which can accept connections from this new pod,
			but since it is new, they don't have this pod in their rules list.
	*/

	//-------------------------------------
	//	Check if needs policies applied
	//-------------------------------------
	go func(policiesList []networking_v1.NetworkPolicy) {
		defer policyWait.Done()
		if pod.Spec.NodeName != manager.node {
			//l.Infoln("pod", pod.Name, "is not in my node: no point in injecting rules. Will stop here.")
			return
		}

		//	First start the firewall
		fw := manager.firewallManager.GetOrCreate(*pod)
		if fw == nil {
			l.Errorln("Could not get firewall for pod", pod.Name, ": I won't be able to inject rules. Will stop here.")
			return
		}

		for _, k8sPolicy := range policiesList {
			//	Make sure it doesn't already enforce this policy
			if !fw.ImplementsPolicy(k8sPolicy.Name) {
				if manager.defaultPolicyParser.DoesPolicyAffectPod(&k8sPolicy, pod) {
					//	Deploy the policy just for this pod
					ingress, egress, policyType := manager.defaultPolicyParser.ParsePolicyTypes(&k8sPolicy.Spec)
					parsed := manager.defaultPolicyParser.ParseRules(ingress, egress, pod.Namespace)

					fw.EnforcePolicy(k8sPolicy.Name, policyType, parsed.Ingress, parsed.Egress)
				}
			}
		}
	}(k8sPolicies)

	//-------------------------------------
	//	Check if a policy targets it
	//-------------------------------------
	go func(policiesList []networking_v1.NetworkPolicy) {
		defer policyWait.Done()

		for _, currentPolicy := range policiesList {
			doesIt := manager.defaultPolicyParser.DoesPolicyTargetPod(&currentPolicy, pod)

			if len(doesIt.Ingress) > 0 || len(doesIt.Egress) > 0 {
				for _, currentFw := range manager.firewallManager.GetAll() {
					if currentFw.ForPod() != pod.UID {
						if currentFw.ImplementsPolicy(currentPolicy.Name) {
							_, _, policyType := manager.defaultPolicyParser.ParsePolicyTypes(&currentPolicy.Spec)
							currentFw.EnforcePolicy(currentPolicy.Name, policyType, doesIt.Ingress, doesIt.Egress)
						}
					}
				}
			}
		}

	}(k8sPolicies)

	policyWait.Wait()

	checked.lastKnownIP = pod.Status.PodIP
	return
}

func (manager *NetworkPolicyManager) manageDeletedPod(pod *core_v1.Pod) {
	var l = log.WithFields(log.Fields{
		"by":     PM,
		"method": "manageDeletedPod(" + pod.Name + ")",
	})

	if pod == nil {
		l.Errorln("Called with a nil pod! Will stop here.")
		return
	}

	if pod.Namespace == "kube-system" {
		l.Infoln("Pod", pod.Name, "belongs to the kube-system namespaces: no point in deleting ips. Will stop here.")
		return
	}

	fws := manager.firewallManager.GetAll()
	//var fwWait sync.WaitGroup
	for _, fw := range fws {
		if fw.ForPod() != pod.UID {
			fw.RemoveIPReferences(pod.Status.PodIP)
		}
	}

	manager.checkPodsLock.Lock()
	delete(manager.checkedPods, pod.UID)
	manager.checkPodsLock.Unlock()
}
