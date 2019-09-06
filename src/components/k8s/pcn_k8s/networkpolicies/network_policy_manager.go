package networkpolicies

import (
	"sync"
	"time"

	pcn_controllers "github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/controllers"
	pcn_firewall "github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/networkpolicies/pcn_firewall"
	pcn_types "github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/types"
	"github.com/polycube-network/polycube/src/components/k8s/utils"
	core_v1 "k8s.io/api/core/v1"
	networking_v1 "k8s.io/api/networking/v1"
	meta_v1 "k8s.io/apimachinery/pkg/apis/meta/v1"
	k8s_types "k8s.io/apimachinery/pkg/types"
)

// PcnNetworkPolicyManager is a network policy manager
type PcnNetworkPolicyManager interface {
	// DeployK8sPolicy deploys a kubernetes policy in the appropriate fw managers
	DeployK8sPolicy(policy *networking_v1.NetworkPolicy, prev *networking_v1.NetworkPolicy)
	// RemoveK8sPolicy removes (ceases) a kubernetes policy
	// from the appropriate firewall managers
	RemoveK8sPolicy(policy *networking_v1.NetworkPolicy, prev *networking_v1.NetworkPolicy)
	// UpdateK8sPolicy updates a kubernetes policy in the appropriate fw managers
	UpdateK8sPolicy(policy *networking_v1.NetworkPolicy, prev *networking_v1.NetworkPolicy)
}

// NetworkPolicyManager is the implementation of the policy manager
type NetworkPolicyManager struct {
	// k8sPolicyParser is the instance of the default policy parser
	k8sPolicyParser PcnK8sPolicyParser
	// lock is the main lock used in the manager
	lock sync.Mutex
	// localFirewalls is a map of the firewall managers inside this node.
	localFirewalls map[string]pcn_firewall.PcnFirewall
	// flaggedForDeletion contains ids of firewall managers
	// that are scheduled to be deleted.
	// Firewall managers will continue updating rules and parse policies even when
	// they have no pods assigned to them anymore (they just won't inject rules).
	// But if this situation persists for at least unscheduleThreshold minutes,
	// then they are going to be deleted.
	flaggedForDeletion map[string]*time.Timer
	// linkedPods is a map linking local pods to local firewalls.
	// It is used to check if a pod has changed and needs to be unlinked.
	// It is a very rare situation, but it must be handled anyway.
	linkedPods map[k8s_types.UID]string
	// policyUnsubscriptors contains function to be used when unsubscribing
	policyUnsubscriptors map[string]*nsUnsubscriptor
}

// nsUnsubscriptor contains information about namespace events that are
// interesting for some policies.
type nsUnsubscriptor struct {
	nsUnsub map[string]func()
}

// startFirewall is a pointer to the StartFirewall method of the
// pcn_firewall package. It is both used as a shortcut and for testing purposes.
var startFirewall = pcn_firewall.StartFirewall

// StartNetworkPolicyManager will start a new network policy manager.
func StartNetworkPolicyManager(nodeName string) PcnNetworkPolicyManager {
	logger.Infoln("Starting Network Policy Manager")

	manager := NetworkPolicyManager{
		localFirewalls:       map[string]pcn_firewall.PcnFirewall{},
		flaggedForDeletion:   map[string]*time.Timer{},
		linkedPods:           map[k8s_types.UID]string{},
		policyUnsubscriptors: map[string]*nsUnsubscriptor{},
	}

	//-------------------------------------
	// Subscribe to k8s policies events
	//-------------------------------------

	manager.k8sPolicyParser = newK8sPolicyParser()

	// Deploy a new k8s policy
	pcn_controllers.K8sPolicies().Subscribe(pcn_types.New, manager.DeployK8sPolicy)

	// Remove a default policy
	pcn_controllers.K8sPolicies().Subscribe(pcn_types.Delete, manager.RemoveK8sPolicy)

	// Update a policy
	pcn_controllers.K8sPolicies().Subscribe(pcn_types.Update, manager.UpdateK8sPolicy)

	//-------------------------------------
	// Subscribe to pod events
	//-------------------------------------

	pcn_controllers.Pods().Subscribe(pcn_types.Update, nil, nil, &pcn_types.ObjectQuery{Name: nodeName}, pcn_types.PodRunning, manager.checkNewPod)
	pcn_controllers.Pods().Subscribe(pcn_types.Delete, nil, nil, &pcn_types.ObjectQuery{Name: nodeName}, pcn_types.PodAnyPhase, manager.manageDeletedPod)

	//-------------------------------------
	// Set up the firewall api
	//-------------------------------------

	pcn_firewall.SetFwAPI(basePath)

	return &manager
}

// DeployK8sPolicy deploys a kubernetes policy in the appropriate fw managers
func (manager *NetworkPolicyManager) DeployK8sPolicy(policy, _ *networking_v1.NetworkPolicy) {
	manager.lock.Lock()
	defer manager.lock.Unlock()

	logger.Infof("K8s policy %s has been deployed", policy.Name)

	manager.deployK8sPolicyToFw(policy, "")
}

// RemoveK8sPolicy removes (ceases) a kubernetes policy
// from the appropriate firewall managers
func (manager *NetworkPolicyManager) RemoveK8sPolicy(_, policy *networking_v1.NetworkPolicy) {
	manager.lock.Lock()
	defer manager.lock.Unlock()

	logger.Infof("K8s policy %s is going to be removed", policy.Name)

	if len(manager.localFirewalls) == 0 {
		logger.Infoln("There are no active firewall managers in this node. Will stop here.")
		return
	}

	//-------------------------------------
	// Unsubscribe from namespace changes
	//-------------------------------------

	if unsubs, exists := manager.policyUnsubscriptors[policy.Name]; exists {
		for _, unsub := range unsubs.nsUnsub {
			unsub()
		}
		delete(manager.policyUnsubscriptors, policy.Name)
	}

	//-------------------------------------
	// Cease this policy
	//-------------------------------------

	// Get the names of the fw managers that are enforcing this.
	// This is done to avoid launching unnecessary go routines
	fws := []string{}
	for _, fwManager := range manager.localFirewalls {
		if fwManager.IsPolicyEnforced(policy.Name) {
			fws = append(fws, fwManager.Name())
		}
	}

	if len(fws) == 0 {
		logger.Infof("Policy %s was not enforced by anyone in this node. Will stop here.", policy.Name)
		return
	}

	var waiter sync.WaitGroup
	waiter.Add(len(fws))

	// Loop through all firewall managers and make them cease this policy
	// if they were enforcing it.
	for _, fwName := range fws {
		go func(fw string) {
			defer waiter.Done()
			manager.localFirewalls[fw].CeasePolicy(policy.Name)
		}(fwName)
	}

	// We have to wait, otherwise other policies may conflict with this
	waiter.Wait()
}

// UpdateK8sPolicy updates a kubernetes policy in the appropriate fw managers
func (manager *NetworkPolicyManager) UpdateK8sPolicy(policy, _ *networking_v1.NetworkPolicy) {
	manager.lock.Lock()
	defer manager.lock.Unlock()

	logger.Infof("K8s policy %s has been updated. Going to parse it again", policy.Name)

	// Updating a policy is no trivial task.
	// We don't know what changed from its previous state:
	// we are forced to re-parse it to know it.
	// Instead of parsing and checking what's changed and what's stayed,
	// we're going to remove the policy and
	// redeploy it.

	if len(manager.localFirewalls) == 0 {
		logger.Infoln("There are no active firewall managers in this node. Will stop here.")
		return
	}

	//-------------------------------------
	// Remove and redeploy
	//-------------------------------------

	// Get the names of the fw managers that are enforcing this.
	// This is done to avoid launching unnecessary go routines
	fws := []string{}
	for _, fwManager := range manager.localFirewalls {
		if fwManager.IsPolicyEnforced(policy.Name) {
			fws = append(fws, fwManager.Name())
		}
	}

	if len(fws) == 0 {
		logger.Infof("Policy %s was not enforced by anyone in this node. Will stop here.", policy.Name)
		return
	}

	var waiter sync.WaitGroup
	waiter.Add(len(fws))

	for _, fwName := range fws {
		go func(fw string) {
			defer waiter.Done()
			fwManager := manager.localFirewalls[fw]
			fwManager.CeasePolicy(policy.Name)
			manager.deployK8sPolicyToFw(policy, fwManager.Name())
		}(fwName)
	}

	waiter.Wait()
}

// deployK8sPolicyToFw actually deploys the provided kubernetes policy
// to a fw manager. To all appropriate ones if second argument is empty.
func (manager *NetworkPolicyManager) deployK8sPolicyToFw(policy *networking_v1.NetworkPolicy, fwName string) {
	//-------------------------------------
	// Start & basic checks
	//-------------------------------------

	// NOTE: This function must be called while holding a lock.
	// That's also one of the reasons why it is un-exported.

	if len(manager.localFirewalls) == 0 {
		logger.Infoln("There are no active firewall managers in this node. Will stop here.")
		return
	}

	// Does it exist?
	if len(fwName) > 0 {
		if _, exists := manager.localFirewalls[fwName]; !exists {
			logger.Errorf("Firewall manager with name %s does not exists in this node. Will stop here.", fwName)
			return
		}
	}

	//-------------------------------------
	// Parse the policy asynchronously
	//-------------------------------------

	// Get the spec, with the ingress & egress rules
	spec := policy.Spec
	ingress, egress, policyType := manager.k8sPolicyParser.ParsePolicyTypes(&spec)

	var parsed pcn_types.ParsedRules
	fwActions := []pcn_types.FirewallAction{}
	barrier := make(chan struct{})

	// Get the rules
	go func() {
		defer close(barrier)
		parsed = manager.k8sPolicyParser.ParseRules(ingress, egress, policy.Namespace)
	}()

	// Get the actions/templates
	fwActions = manager.k8sPolicyParser.BuildActions(ingress, egress, policy.Namespace)

	// Firewall is transparent: we need to reverse the directions
	oppositePolicyType := policyType
	if oppositePolicyType != "*" {
		if policyType == "ingress" {
			oppositePolicyType = "egress"
		} else {
			oppositePolicyType = "ingress"
		}
	}

	// At this point I can't go on without the rules.
	<-barrier

	//-------------------------------------
	// Enforce the policy
	//-------------------------------------

	// -- To a single firewall manager
	if len(fwName) > 0 {
		fw := manager.localFirewalls[fwName]
		fw.EnforcePolicy(policy.Name, oppositePolicyType, policy.CreationTimestamp, parsed.Ingress, parsed.Egress, fwActions)
		return
	}

	// -- To all the appropriate ones
	// Get their names
	fws := []string{}
	for _, fwManager := range manager.localFirewalls {
		labels, ns := fwManager.Selector()
		pod := core_v1.Pod{
			ObjectMeta: meta_v1.ObjectMeta{
				Namespace: ns,
				Labels:    labels,
			},
		}
		if manager.k8sPolicyParser.DoesPolicyAffectPod(policy, &pod) {
			fws = append(fws, fwManager.Name())
		}
	}

	if len(fws) == 0 {
		logger.Infof("Policy %s does not affect anyone in this node. Will stop here.", policy.Name)
		return
	}

	// -- Actually enforce it
	var waiter sync.WaitGroup
	waiter.Add(len(fws))

	// Loop through all firewall managers that should enforce this policy
	for _, currentFwName := range fws {
		go func(fw string) {
			defer waiter.Done()
			fwManager := manager.localFirewalls[fw]
			fwManager.EnforcePolicy(policy.Name, oppositePolicyType, policy.CreationTimestamp, parsed.Ingress, parsed.Egress, fwActions)
		}(currentFwName)
	}

	// Loop through all firewall actions to get namespace labels. Why?
	// Because if a policy wants to accept pods that are in a namespace
	// with labels X and Y and, later, the admin changes those labels to
	// Z and W then we have to remove all the pods that we were previously
	// accepting!
	// This may need to be improved later on, but since it is a very rare
	// situation, it may stay like this.
	for _, action := range fwActions {
		if len(action.NamespaceLabels) > 0 {
			if _, exists := manager.policyUnsubscriptors[policy.Name]; !exists {
				manager.policyUnsubscriptors[policy.Name] = &nsUnsubscriptor{
					nsUnsub: map[string]func(){},
				}
			}
			// already there?
			nsKey := utils.ImplodeLabels(action.NamespaceLabels, ",", false)
			if _, exists := manager.policyUnsubscriptors[policy.Name].nsUnsub[nsKey]; exists {
				continue
			}

			// Subscribe
			unsub, err := pcn_controllers.Namespaces().Subscribe(pcn_types.Update, func(ns, prev *core_v1.Namespace) {
				manager.handleNamespaceUpdate(ns, prev, action, policy.Name, policy.Namespace)
			})
			if err == nil {
				manager.policyUnsubscriptors[policy.Name].nsUnsub[nsKey] = unsub
			}
		}
	}

	waiter.Wait()
}

// handleNamespaceUpdate checks if a namespace has changed its labels.
// If so, the appropriate firewall should update the policies as well.
func (manager *NetworkPolicyManager) handleNamespaceUpdate(ns, prev *core_v1.Namespace, action pcn_types.FirewallAction, policyName, policyNs string) {
	logger.Infof("Namespace %s has been updated", ns.Name)

	// Function that checks if an update of the policy is needed.
	// Done like this so we can use defer and keep the code more
	// organized
	needsUpdate := func() (*networking_v1.NetworkPolicy, bool) {
		manager.lock.Lock()
		defer manager.lock.Unlock()

		if prev == nil {
			return nil, false
		}

		// When should we update the firewalls in case of namespace updates?
		// 1) When previously it had some labels that were ok and now are not.
		// 2) The opposite of the 1): previously ok, now not-ok
		// Anyway, on case 1) we must remove the pods previously allowed
		// and on case 2) we must allow some new pods.
		// This simply means checking the previous state vs the current.

		previously := utils.AreLabelsContained(action.NamespaceLabels, prev.Labels)
		currently := utils.AreLabelsContained(action.NamespaceLabels, ns.Labels)

		if previously == currently {
			return nil, false
		}

		logger.Infof("%s's labels have changed", ns.Name)

		policyQuery := utils.BuildQuery(policyName, nil)
		nsQuery := utils.BuildQuery(policyNs, nil)
		policies, err := pcn_controllers.K8sPolicies().List(policyQuery, nsQuery)
		if err != nil {
			logger.Errorf("Could not load policies named %s on namespace %s. Error: %s", policyName, policyNs, err)
			return nil, false
		}
		if len(policies) == 0 {
			logger.Errorf("Policiy named %s on namespace %s has not been found.", policyName, policyNs)
			return nil, false
		}
		policy := policies[0]

		return &policy, true
	}

	// Check it
	policy, ok := needsUpdate()
	if !ok {
		return
	}

	// Remove that policy and redeploy it again
	manager.RemoveK8sPolicy(nil, policy)
	manager.DeployK8sPolicy(policy, nil)
}

// checkNewPod will perform some checks on the new pod just updated.
// Specifically, it will check if the pod needs to be protected.
func (manager *NetworkPolicyManager) checkNewPod(pod, prev *core_v1.Pod) {
	//-------------------------------------
	// Basic Checks
	//-------------------------------------

	// Is this pod from the kube-system?
	if pod.Namespace == "kube-system" {
		logger.Infof("Pod %s belongs to the kube-system namespace: no point in checking for policies. Will stop here.", pod.Name)
		return
	}

	// Get or create the firewall manager for this pod and then link it.
	// Doing it a lambda so we can use defer, and we can block the thread
	// for as short time as possible
	linkPod := func() (bool, pcn_firewall.PcnFirewall) {
		manager.lock.Lock()
		defer manager.lock.Unlock()

		fw, justCreated := manager.getOrCreateFirewallManager(pod, prev)

		// Link returns false when the pod was not linked because it was already linked,
		// 	or if the pod's firewall was not ok.
		inserted := fw.Link(pod)

		if inserted {
			manager.linkedPods[pod.UID] = fw.Name()
			manager.unflagForDeletion(fw.Name())
		}

		// If the firewall manager already existed there is no point in going on:
		// policies are already there. But if it was just created we need to parse rules
		// even if the pod was not linked correctly, because it is very probable that
		// the pod will be deployed again, so when it happens it'll have rules ready.
		// This will also prevent us from depleting resources if the pod is unstable:
		// this will be done just once.
		return justCreated, fw
	}

	shouldInit, fw := linkPod()
	if !shouldInit {
		// Firewall is already inited. You can stop here.
		return
	}

	//-------------------------------------
	// Must this pod enforce any policy?
	//-------------------------------------
	k8sPolicies, _ := pcn_controllers.K8sPolicies().List(nil, &pcn_types.ObjectQuery{By: "name", Name: pod.Namespace})
	for _, kp := range k8sPolicies {
		if manager.k8sPolicyParser.DoesPolicyAffectPod(&kp, pod) {
			manager.deployK8sPolicyToFw(&kp, fw.Name())
		}
	}
}

// getOrCreateFirewallManager gets a local firewall manager
// for this pod or creates one if not there.
// Returns the newly created/already existing firewall manager,
// its key, and TRUE if it was just created.
func (manager *NetworkPolicyManager) getOrCreateFirewallManager(pod, prev *core_v1.Pod) (pcn_firewall.PcnFirewall, bool) {
	fwKey := pod.Namespace + "|" + utils.ImplodeLabels(pod.Labels, ",", true)

	//-------------------------------------
	// Already linked?
	//-------------------------------------
	linkedFw, wasLinked := manager.linkedPods[pod.UID]
	if wasLinked && linkedFw != fwKey {
		prevFwKey := prev.Namespace + "|" + utils.ImplodeLabels(prev.Labels, ",", true)

		// This pod was already linked to a firewall manager,
		// but it's not the one we expected. This means that someone
		// (user or plugin) changed this pod's labels,
		// so we now need to unlink the pod from its current fw manager.
		prevFw, exists := manager.localFirewalls[prevFwKey]
		if exists {
			unlinked, remaining := prevFw.Unlink(pod, pcn_firewall.CleanFirewall)
			if !unlinked {
				logger.Warningf("%s was not linked in previous firewall manager!", pod.UID)
			} else {
				if remaining == 0 {
					manager.flagForDeletion(prevFw.Name())
				}
				delete(manager.linkedPods, pod.UID)
			}
		} else {
			logger.Warningf("Could not find %s previous firewall manager!", pod.UID)
		}
	}

	//-------------------------------------
	// Create and link it
	//-------------------------------------
	fw, exists := manager.localFirewalls[fwKey]
	if !exists {
		manager.localFirewalls[fwKey] = startFirewall(fwKey, pod.Namespace, pod.Labels)
		fw = manager.localFirewalls[fwKey]
		return fw, true
	}
	return fw, false
}

// flagForDeletion flags a firewall for deletion
func (manager *NetworkPolicyManager) flagForDeletion(fwKey string) {
	_, wasFlagged := manager.flaggedForDeletion[fwKey]

	// Was it flagged?
	if !wasFlagged {
		manager.flaggedForDeletion[fwKey] = time.AfterFunc(time.Minute*time.Duration(UnscheduleThreshold), func() {
			manager.deleteFirewallManager(fwKey)
		})
	}
}

// unflagForDeletion unflags a firewall manager for deletion
func (manager *NetworkPolicyManager) unflagForDeletion(fwKey string) {
	timer, wasFlagged := manager.flaggedForDeletion[fwKey]

	// Was it flagged?
	if wasFlagged {
		timer.Stop() // you're going to survive! Be happy!
		delete(manager.flaggedForDeletion, fwKey)
	}
}

// manageDeletedPod makes sure that the appropriate fw manager
// will destroy this pod's firewall
func (manager *NetworkPolicyManager) manageDeletedPod(_ *core_v1.Pod, pod *core_v1.Pod) {
	manager.lock.Lock()
	defer manager.lock.Unlock()

	if pod.Namespace == "kube-system" {
		logger.Infof("Pod %s belongs to the kube-system namespace: no point in checking its firewall manager. Will stop here.", pod.Name)
		return
	}

	//fwKey := manager.implode(pod.Labels, pod.Namespace)
	fwKey := pod.Namespace + "|" + utils.ImplodeLabels(pod.Labels, ",", true)
	defer delete(manager.linkedPods, pod.UID)

	// First get the firewall
	fw, exists := manager.localFirewalls[fwKey]
	if !exists {
		// The firewall manager for this pod does not exist.
		// Then who managed it until now? This is a very improbable case.
		logger.Warningln("Could not find a firewall manager for dying pod", pod.UID, "!")
		return
	}

	wasLinked, remaining := fw.Unlink(pod, pcn_firewall.DestroyFirewall)
	if !wasLinked {
		// This pod wasn't even linked to the firewall!
		logger.Warningln("Dying pod", pod.UID, "was not linked to its firewall manager", fwKey)
		return
	}

	if remaining == 0 {
		manager.flagForDeletion(fwKey)
	}
}

// deleteFirewallManager will delete a firewall manager.
// Usually this function is called automatically when after a certain threshold.
func (manager *NetworkPolicyManager) deleteFirewallManager(fwKey string) {
	manager.lock.Lock()
	defer manager.lock.Unlock()

	// The garbage collector will take care of destroying everything inside it
	// now that no one points to it anymore.
	// Goodbye!
	manager.localFirewalls[fwKey].Destroy()
	delete(manager.localFirewalls, fwKey)
	delete(manager.flaggedForDeletion, fwKey)
}
