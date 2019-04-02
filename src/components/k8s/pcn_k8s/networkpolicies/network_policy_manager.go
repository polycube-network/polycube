package networkpolicies

import (
	"sync"

	//	TODO-ON-MERGE: change these to the polycube path
	pcn_controllers "github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/controllers"
	pcn_firewall "github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/networkpolicies/pcn_firewall"
	pcn_types "github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/types"
	k8sfirewall "github.com/SunSince90/polycube/src/components/k8s/utils/k8sfirewall"

	log "github.com/sirupsen/logrus"
	core_v1 "k8s.io/api/core/v1"
	networking_v1 "k8s.io/api/networking/v1"
	k8s_types "k8s.io/apimachinery/pkg/types"
)

type PcnNetworkPolicyManager interface {
}

type NetworkPolicyManager struct {
	dnpc          *pcn_controllers.DefaultNetworkPolicyController
	podController pcn_controllers.PodController

	defaultPolicyParser PcnDefaultPolicyParser
	deployedPolicies    map[string][]k8s_types.UID
	checkedPods         map[k8s_types.UID]*checkedPod
	node                string
	lock                sync.Mutex
	checkPodsLock       sync.Mutex
	log                 *log.Logger
	localFirewalls      map[k8s_types.UID]pcn_firewall.PcnFirewall
	fwAPI               k8sfirewall.FirewallAPI
}

type checkedPod struct {
	lastKnownIP string
	sync.Mutex
}

func StartNetworkPolicyManager(basePath string, dnpc *pcn_controllers.DefaultNetworkPolicyController, podController pcn_controllers.PodController, nodeName string) PcnNetworkPolicyManager {
	l := log.NewEntry(log.New())
	l.WithFields(log.Fields{"by": PM, "method": "StartNetworkPolicyManager()"})
	l.Infoln("Starting Network Policy Manager")

	cfgK8firewall := k8sfirewall.Configuration{BasePath: basePath}
	srK8firewall := k8sfirewall.NewAPIClient(&cfgK8firewall)
	fwAPI := srK8firewall.FirewallApi

	manager := NetworkPolicyManager{
		dnpc:             dnpc,
		podController:    podController,
		deployedPolicies: map[string][]k8s_types.UID{},
		checkedPods:      map[k8s_types.UID]*checkedPod{},
		node:             nodeName,
		localFirewalls:   map[k8s_types.UID]pcn_firewall.PcnFirewall{},
		log:              log.New(),
		fwAPI:            fwAPI,
	}

	//-------------------------------------
	//	Subscribe to default policies events
	//-------------------------------------

	manager.defaultPolicyParser = newDefaultPolicyParser(podController, nodeName)

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
	podController.Subscribe(pcn_types.Update, pcn_types.ObjectQuery{}, pcn_types.ObjectQuery{}, pcn_types.PodRunning, manager.checkNewPod)
	podController.Subscribe(pcn_types.Delete, pcn_types.ObjectQuery{}, pcn_types.ObjectQuery{}, pcn_types.PodAnyPhase, manager.manageDeletedPod)

	return &manager
}

func (manager *NetworkPolicyManager) DeployDefaultPolicy(policy *networking_v1.NetworkPolicy) {
	l := log.NewEntry(manager.log)
	l.WithFields(log.Fields{"by": PM, "method": "DeployDefaultPolicy(" + policy.Name + ")"})

	//-------------------------------------
	//	The basics
	//-------------------------------------
	nsPods, err := manager.defaultPolicyParser.GetPodsAffected(policy)
	if err != nil {
		l.Errorln("Error while trying to get pods affected by policy.", err)
		return
	}

	//	No pods found?
	if len(nsPods) < 1 {
		l.Infoln("No pods found for policy.", err)
		return
	}

	//	Get the spec, with the ingress & egress rules
	spec := policy.Spec
	ingress, egress, policyType := manager.defaultPolicyParser.ParsePolicyTypes(&spec)

	//-------------------------------------
	//	Parse, Deploy & Set Actions
	//-------------------------------------
	for ns, pods := range nsPods {

		var parsed pcn_types.ParsedRules
		fwActions := pcn_types.FirewallActions{}

		var podsWaitGroup sync.WaitGroup
		podsWaitGroup.Add(2)

		//	Parse...
		go func() {
			defer podsWaitGroup.Done()
			parsed = manager.defaultPolicyParser.ParseRules(ingress, egress, ns)
		}()

		go func() {
			defer podsWaitGroup.Done()
			fwActions = manager.defaultPolicyParser.GetClusterActions(ingress, egress, ns)
		}()

		podsWaitGroup.Wait()

		//	Reusing the waitgroup...
		podsWaitGroup.Add(len(pods))
		for _, pod := range pods {

			//	Deploy...
			go func(currentPod core_v1.Pod) {
				defer podsWaitGroup.Done()

				//	If this pod is not ready or is terminating, then don't proceed
				if currentPod.Status.Phase != core_v1.PodRunning || currentPod.DeletionTimestamp != nil {
					return
				}

				//	Deploy...
				//	Create the firewall (or get it if already exists)
				fw, exists := manager.localFirewalls[pod.UID]
				if !exists {
					l.Errorln("No firewall exists for pod", pod.Status.PodIP)
					return
				}

				//-------------------------------------
				//	Inject the rules and the actions
				//-------------------------------------

				fw.EnforcePolicy(policy.Name, policyType, parsed.Ingress, parsed.Egress, fwActions)

			}(pod)
		}

		podsWaitGroup.Wait()
	}
}

func (manager *NetworkPolicyManager) RemoveDefaultPolicy(policy *networking_v1.NetworkPolicy) {
	podsAffected, err := manager.defaultPolicyParser.GetPodsAffected(policy)

	if err != nil {
		//	err
		return
	}

	//	We are iterating only through pods which are here in this node.
	for _, pods := range podsAffected {
		for _, pod := range pods {
			fw, exists := manager.localFirewalls[pod.UID]
			if exists && fw.ImplementsPolicy(policy.Name) {
				fw.CeasePolicy(policy.Name)
			}
		}
	}
}

func (manager *NetworkPolicyManager) UpdateDefaultPolicy(policy *networking_v1.NetworkPolicy) {

	manager.RemoveDefaultPolicy(policy)
	manager.DeployDefaultPolicy(policy)
}

func (manager *NetworkPolicyManager) checkNewPod(pod *core_v1.Pod) {
	l := log.NewEntry(manager.log)
	l.WithFields(log.Fields{"by": PM, "method": "checkNewPod(" + pod.Name + ")"})

	//-------------------------------------
	//	Basic Checks
	//-------------------------------------

	//	Is this pod from the kube-system?
	//	TODO: does checking pods inside kube-system make sense actually?
	if pod.Namespace == "kube-system" {
		l.Infoln("Pod", pod.Name, "belongs to the kube-system namespaces: no point in checking for policies. Will stop here.")
		return
	}

	//	Did I already create a firewall for this?
	//	Let's do it in a lambda, so we can use defer
	fw, existed := func(uid k8s_types.UID) (pcn_firewall.PcnFirewall, bool) {
		manager.checkPodsLock.Lock()
		defer manager.checkPodsLock.Unlock()

		fw, exists := manager.localFirewalls[uid]
		if !exists {
			manager.localFirewalls[uid] = pcn_firewall.StartFirewall(*pod, manager.fwAPI, manager.podController)
		}
		return fw, exists
	}(pod.UID)

	if existed {
		//	The firewall already existed, it's useless to go on.
		return
	}

	//-------------------------------------
	//	Must this pod enforce any policy?
	//-------------------------------------

	k8sPolicies, _ := manager.dnpc.GetPolicies(pcn_types.ObjectQuery{By: "name", Name: "*"})

	//	TODO: in a thread for each of them?
	for _, kp := range k8sPolicies {
		if manager.defaultPolicyParser.DoesPolicyAffectPod(&kp, pod) && !fw.ImplementsPolicy(kp.Name) {

			var parsed pcn_types.ParsedRules
			fwActions := pcn_types.FirewallActions{}
			ingress, egress, policyType := manager.defaultPolicyParser.ParsePolicyTypes(&kp.Spec)

			var podsWaitGroup sync.WaitGroup
			podsWaitGroup.Add(2)

			//	Parse...
			go func() {
				defer podsWaitGroup.Done()
				parsed = manager.defaultPolicyParser.ParseRules(ingress, egress, pod.Namespace)
			}()

			go func() {
				defer podsWaitGroup.Done()
				fwActions = manager.defaultPolicyParser.GetClusterActions(ingress, egress, pod.Namespace)
			}()

			podsWaitGroup.Wait()
			fw.EnforcePolicy(kp.Name, policyType, parsed.Ingress, parsed.Egress, fwActions)
		}
	}

	//	Have I already checked this before?
	/*manager.checkPodsLock.Lock()
	checked, ok := manager.checkedPods[pod.UID]
	if !ok {
		checked = &checkedPod{}
		manager.checkedPods[pod.UID] = checked
	}
	manager.checkPodsLock.Unlock()*/

	/*checked.Lock()
	defer checked.Unlock()*/

	//	Already checked before?
	/*if checked.lastKnownIP == pod.Status.PodIP {
		//l.Debugln("pod", pod.Name, "has already been checked before: no point in checking it again. Will stop here.")
		return
	}*/

	//l.Debugln("Ok,", pod.Name, "Has never been checked.")

	var policyWait sync.WaitGroup
	policyWait.Add(2)
	//policyWait.Add(2)
	//k8sPolicies, _ := manager.dnpc.GetPolicies(pcn_types.ObjectQuery{By: "name", Name: "*"})
	//	The most recently deployed policies should have precedence, but the firewall doesn't support haed insertion yet.
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
		/*if pod.Spec.NodeName != manager.node {
			//l.Infoln("pod", pod.Name, "is not in my node: no point in injecting rules. Will stop here.")
			return
		}*/

		//	First start the firewall
		/*fw := manager.firewallManager.GetOrCreate(*pod)
		if fw == nil {
			l.Errorln("Could not get firewall for pod", pod.Name, ": I won't be able to inject rules. Will stop here.")
			return
		}*/

		/*for _, k8sPolicy := range policiesList {
			//	Make sure it doesn't already enforce this policy
			if !fw.ImplementsPolicy(k8sPolicy.Name) {
				if manager.defaultPolicyParser.DoesPolicyAffectPod(&k8sPolicy, pod) {
					//	Deploy the policy just for this pod
					ingress, egress, policyType := manager.defaultPolicyParser.ParsePolicyTypes(&k8sPolicy.Spec)
					parsed := manager.defaultPolicyParser.ParseRules(ingress, egress, pod.Namespace)

					fw.EnforcePolicy(k8sPolicy.Name, policyType, parsed.Ingress, parsed.Egress, pcn_types.FirewallActions{})
				}
			}
		}*/
	}(k8sPolicies)

	//-------------------------------------
	//	Check if a policy targets it
	//-------------------------------------
	go func(policiesList []networking_v1.NetworkPolicy) {
		defer policyWait.Done()

		/*for _, currentPolicy := range policiesList {
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
		}*/

	}(k8sPolicies)

	policyWait.Wait()

	//checked.lastKnownIP = pod.Status.PodIP
	return
}

func (manager *NetworkPolicyManager) manageDeletedPod(pod *core_v1.Pod) {
	l := log.NewEntry(manager.log)
	l.WithFields(log.Fields{"by": PM, "method": "manageDeletePod(" + pod.Name + ")"})

	if pod == nil {
		l.Errorln("Called with a nil pod! Will stop here.")
		return
	}

	if pod.Namespace == "kube-system" {
		l.Infoln("Pod", pod.Name, "belongs to the kube-system namespaces: no point in deleting ips. Will stop here.")
		return
	}

	manager.checkPodsLock.Lock()
	defer manager.checkPodsLock.Unlock()
	if fw, exists := manager.localFirewalls[pod.UID]; exists {
		fw.Destroy()
	}
	delete(manager.localFirewalls, pod.UID)
}
