package networkpolicies

import (
	"strings"

	//	TODO-ON-MERGE: change these two to the polycube path

	"sync"

	pcn_controllers "github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/controllers"
	pcn_firewall "github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/networkpolicies/pcn_firewall"
	pcn_types "github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/types"
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
	/*var l = log.WithFields(log.Fields{
		"by":     networkpolicies.PM,
		"method": "StartNetworkPolicyManager()",
	})
	l.Infoln("Starting Network Policy Manager")*/

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
	//podController.Subscribe(pcn_types.Update, manager.checkNewPod)
	podController.Subscribe(pcn_types.Delete, manager.manageDeletedPod)

	return &manager
}

func (manager *NetworkPolicyManager) DeployDefaultPolicy(policy *networking_v1.NetworkPolicy) {
	/*var l = log.WithFields(log.Fields{
		"by":     networkpolicies.PM,
		"method": "DeployDefaultPolicy",
	})
	l.Infoln("Going to deploy default policy", policy.Name)*/

	//	for each pod...
	//	if pod is in my node
	//	if policy affects pod
	//	parse & get rules back

	/*manager.podController.GetPods(pcn_types.)

	if pod.Spec.NodeName != manager.node {
		l.Infoln("pod", pod.Name, "is not in my node: no point in injecting rules. Will stop here.")
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
				//l.Debugln("pod", pod.Name, "is affected by", k8sPolicy.Name)
				//	Deploy the policy just for this pod
				ingress, egress := manager.defaultPolicyParser.ParsePolicyTypes(&k8sPolicy.Spec)
				parsed := manager.defaultPolicyParser.ParseRules(ingress, egress, pod.Namespace)
				parsed = manager.defaultPolicyParser.FillChains(pcn_types.Pod{
					Pod:  *pod,
					Veth: "",
				}, parsed.Ingress, parsed.Egress)

				fw.EnforcePolicy(k8sPolicy.Name, parsed.Ingress, parsed.Egress)
				//l.Debugln("pod", pod.Name, "should now enforce", k8sPolicy.Name)
			}
		}*/

	manager.defaultPolicyParser.Parse(policy, true)
	//	UPDATE: ignore the returned value, we don't need it for now.
	/*for _, pods := range manager.defaultPolicyParser.Parse(policy, true) {

		manager.Lock()
		for podID := range pods {
			manager.deployedPolicies[policy.Name] = append(manager.deployedPolicies[policy.Name], podID)
		}
		manager.Unlock()
	}*/

	//l.Debugln(policy.Name, "deployed.")
}

func (manager *NetworkPolicyManager) RemoveDefaultPolicy(policy *networking_v1.NetworkPolicy) {
	/*var l = log.WithFields(log.Fields{
		"by":     networkpolicies.PM,
		"method": "RemoveDefaultPolicy",
	})
	l.Infoln("Going to remove default policy", policy.Name)*/
	podsAffected, err := manager.defaultPolicyParser.GetPodsAffected(policy)

	if err != nil {
		//	err
		return
	}

	//podsInPolicy := []k8s_types.UID{}

	for _, pods := range podsAffected {
		for _, pod := range pods {

			if pod.Pod.Spec.NodeName == manager.node {
				fw := manager.firewallManager.Get(pod.Pod)

				if fw != nil {
					fw.CeasePolicy(policy.Name)

					//	Remove this pod from the deployed policies
					//	UPDATE: read below.
					//podsInPolicy = append(podsInPolicy, pod.Pod.UID)
				}
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
		l.Debugln("Pod", pod.Name, "is terminating. Will not do anything else.")
		return
	}

	//	Is it running? (if not, it wouldn't have a valid ip yet)
	if pod.Status.Phase != core_v1.PodRunning {
		l.Debugln("Pod", pod.Name, "is in", pod.Status.Phase, "so I'm not going to check it.")
		return
	}

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
		l.Debugln("pod", pod.Name, "has already been checked before: no point in checking it again. Will stop here.")
		return
	}

	l.Debugln("Ok,", pod.Name, "Has never been checked.")

	var policyWait sync.WaitGroup
	policyWait.Add(2)
	k8sPolicies, _ := manager.dnpc.GetPolicies(pcn_types.ObjectQuery{By: "name", Name: "*"})

	//	The most recently deployed policies should have precedence, but I still have to test it with the firewall...
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
			l.Infoln("pod", pod.Name, "is not in my node: no point in injecting rules. Will stop here.")
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
					//l.Debugln("pod", pod.Name, "is affected by", k8sPolicy.Name)
					//	Deploy the policy just for this pod
					ingress, egress := manager.defaultPolicyParser.ParsePolicyTypes(&k8sPolicy.Spec)
					parsed := manager.defaultPolicyParser.ParseRules(ingress, egress, pod.Namespace)
					parsed = manager.defaultPolicyParser.FillChains(pcn_types.Pod{
						Pod:  *pod,
						Veth: "",
					}, parsed.Ingress, parsed.Egress)

					fw.EnforcePolicy(k8sPolicy.Name, parsed.Ingress, parsed.Egress)
					//l.Debugln("pod", pod.Name, "should now enforce", k8sPolicy.Name)
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
							//l.Debugln("going to update a firewall")
							currentFw.EnforcePolicy(currentPolicy.Name, doesIt.Ingress, doesIt.Egress)
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
		"method": "manageDeletedPod",
	})

	if pod == nil {
		l.Errorln("Called with a nil pod! Will stop here.")
		return
	}

	if strings.HasPrefix(pod.Name, "polycube-") {
		l.Infoln("Pod", pod.Name, "is a core polycube component: no point in deleting its ips from other firewalls. Will stop here.")
		return
	}

	l.Debugln("Pod ip to remove", pod.Status.PodIP, pod.Name)
	log.Debugln("###manageDeletedPod:", pod.Name, "with ip", pod.Status.PodIP, "on phase", pod.Status.Phase, pod.Status.Message, pod.Status.Reason)
	log.Debugf("###manageDeletedPod: %s, last status %+v\n", pod.Name, pod.Status.Conditions[len(pod.Status.Conditions)-1])

	fws := manager.firewallManager.GetAll()
	//var fwWait sync.WaitGroup
	for _, fw := range fws {
		if fw.ForPod() != pod.UID {
			fw.RemoveIPReferences(pod.Status.PodIP)
		}
	}

	l.Debugln("Ok, now i'm going to remove pod", pod.Name, "from checkedpods")

	manager.checkPodsLock.Lock()
	delete(manager.checkedPods, pod.UID)
	//	firewall manager, remove entry
	manager.checkPodsLock.Unlock()

	l.Debugln(pod.Name, "correctly removed from checkedpods")
}
