package networkpolicies

import (
	"net"
	"sync"
	"time"

	pcn_controllers "github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/controllers"
	pcn_firewall "github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/networkpolicies/pcn_firewall"
	pcn_types "github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/types"
	"github.com/polycube-network/polycube/src/components/k8s/utils"
	k8sfirewall "github.com/polycube-network/polycube/src/components/k8s/utils/k8sfirewall"
	log "github.com/sirupsen/logrus"
	core_v1 "k8s.io/api/core/v1"
	networking_v1 "k8s.io/api/networking/v1"
	meta_v1 "k8s.io/apimachinery/pkg/apis/meta/v1"
	k8s_types "k8s.io/apimachinery/pkg/types"
	"k8s.io/client-go/kubernetes"
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
	// knpc is the kubernetes policy controller
	knpc *pcn_controllers.K8sNetworkPolicyController
	// podController is the pod controller
	podController pcn_controllers.PodController
	// nsController is the namespace controller
	nsController pcn_controllers.NamespaceController
	// k8sPolicyParser is the instance of the default policy parser
	k8sPolicyParser PcnK8sPolicyParser
	// fwAPI is the firewall API
	fwAPI *k8sfirewall.FirewallApiService
	// nodeName is the name of the node in which we are running
	nodeName string
	// lock is the main lock used in the manager
	lock sync.Mutex
	// localFirewalls is a map of the firewall managers inside this node.
	localFirewalls map[string]pcn_firewall.PcnFirewall
	// unscheduleThreshold is the number of MINUTES after which a firewall manager
	// should be deleted if no pods are assigned to it.
	unscheduleThreshold int
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
	// vPodsRange is the virtual IP range of the pods
	vPodsRange *net.IPNet
}

// startFirewall is a pointer to the StartFirewall method of the
// pcn_firewall package. It is both used as a shortcut and for testing purposes.
var startFirewall = pcn_firewall.StartFirewall

// StartNetworkPolicyManager will start a new network policy manager.
func StartNetworkPolicyManager(clientset kubernetes.Interface, vPodsRange *net.IPNet, basePath, nodeName string, knpc *pcn_controllers.K8sNetworkPolicyController, podController pcn_controllers.PodController, nsController pcn_controllers.NamespaceController) PcnNetworkPolicyManager {
	l := log.New().WithFields(log.Fields{"by": PM, "method": "StartNetworkPolicyManager()"})
	l.Debugln("Starting Network Policy Manager")

	cfgK8firewall := k8sfirewall.Configuration{BasePath: basePath}
	srK8firewall := k8sfirewall.NewAPIClient(&cfgK8firewall)
	fwAPI := srK8firewall.FirewallApi

	manager := NetworkPolicyManager{
		knpc:                knpc,
		podController:       podController,
		nsController:        nsController,
		nodeName:            nodeName,
		localFirewalls:      map[string]pcn_firewall.PcnFirewall{},
		unscheduleThreshold: UnscheduleThreshold,
		flaggedForDeletion:  map[string]*time.Timer{},
		linkedPods:          map[k8s_types.UID]string{},
		fwAPI:               fwAPI,
		vPodsRange:          vPodsRange,
	}

	//-------------------------------------
	// Subscribe to k8s policies events
	//-------------------------------------

	manager.k8sPolicyParser = newK8sPolicyParser(clientset, podController, nsController, vPodsRange)

	// Deploy a new k8s policy
	knpc.Subscribe(pcn_types.New, manager.DeployK8sPolicy)

	// Remove a default policy
	knpc.Subscribe(pcn_types.Delete, manager.RemoveK8sPolicy)

	// Update a policy
	knpc.Subscribe(pcn_types.Update, manager.UpdateK8sPolicy)

	//-------------------------------------
	// Subscribe to pod events
	//-------------------------------------

	podController.Subscribe(pcn_types.Update, nil, nil, &pcn_types.ObjectQuery{Name: nodeName}, pcn_types.PodRunning, manager.checkNewPod)
	podController.Subscribe(pcn_types.Delete, nil, nil, &pcn_types.ObjectQuery{Name: nodeName}, pcn_types.PodAnyPhase, manager.manageDeletedPod)

	return &manager
}

// DeployK8sPolicy deploys a kubernetes policy in the appropriate fw managers
func (manager *NetworkPolicyManager) DeployK8sPolicy(policy, _ *networking_v1.NetworkPolicy) {
	manager.lock.Lock()
	defer manager.lock.Unlock()

	l := log.New().WithFields(log.Fields{"by": PM, "method": "DeployK8sPolicy(" + policy.Name + ")"})
	l.Infof("K8s policy %s has been deployed", policy.Name)

	manager.deployK8sPolicyToFw(policy, "")
}

// RemoveK8sPolicy removes (ceases) a kubernetes policy
// from the appropriate firewall managers
func (manager *NetworkPolicyManager) RemoveK8sPolicy(_, policy *networking_v1.NetworkPolicy) {
	manager.lock.Lock()
	defer manager.lock.Unlock()

	l := log.New().WithFields(log.Fields{"by": PM, "method": "RemoveK8sPolicy(" + policy.Name + ")"})
	l.Infof("K8s policy %s is going to be removed", policy.Name)

	if len(manager.localFirewalls) == 0 {
		l.Infoln("There are no active firewall managers in this node. Will stop here.")
		return
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
		l.Infof("Policy %s was not enforced by anyone in this node. Will stop here.", policy.Name)
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

	l := log.New().WithFields(log.Fields{"by": PM, "method": "UpdateK8sPolicy(" + policy.Name + ")"})
	l.Infof("K8s policy %s has been updated. Going to parse it again", policy.Name)

	// Updating a policy is no trivial task.
	// We don't know what changed from its previous state:
	// we are forced to re-parse it to know it.
	// Instead of parsing and checking what's changed and what's stayed,
	// we're going to remove the policy and
	// redeploy it.

	if len(manager.localFirewalls) == 0 {
		l.Infoln("There are no active firewall managers in this node. Will stop here.")
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
		l.Infof("Policy %s was not enforced by anyone in this node. Will stop here.", policy.Name)
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
	l := log.New().WithFields(log.Fields{"by": PM, "method": "deployK8sPolicyToFw(" + policy.Name + ", " + fwName + ")"})

	// NOTE: This function must be called while holding a lock.
	// That's also one of the reasons why it is un-exported.

	if len(manager.localFirewalls) == 0 {
		l.Infoln("There are no active firewall managers in this node. Will stop here.")
		return
	}

	// Does it exist?
	if len(fwName) > 0 {
		if _, exists := manager.localFirewalls[fwName]; !exists {
			l.Errorf("Firewall manager with name %s does not exists in this node. Will stop here.", fwName)
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

	parsed = manager.k8sPolicyParser.ParseRules(ingress, egress, policy.Namespace)

	// Firewall is transparent: we need to reverse the directions
	oppositePolicyType := policyType
	if oppositePolicyType != "*" {
		if policyType == "ingress" {
			oppositePolicyType = "egress"
		} else {
			oppositePolicyType = "ingress"
		}
	}

	//-------------------------------------
	// Enforce the policy
	//-------------------------------------

	// -- To a single firewall manager
	if len(fwName) > 0 {
		fw := manager.localFirewalls[fwName]
		fw.EnforcePolicy(policy.Name, oppositePolicyType, policy.CreationTimestamp, parsed.Ingress, parsed.Egress)
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
		l.Infof("Policy %s does not affect anyone in this node. Will stop here.", policy.Name)
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
			fwManager.EnforcePolicy(policy.Name, oppositePolicyType, policy.CreationTimestamp, parsed.Ingress, parsed.Egress)
		}(currentFwName)
	}

	waiter.Wait()
}

// checkNewPod will perform some checks on the new pod just updated.
// Specifically, it will check if the pod needs to be protected.
func (manager *NetworkPolicyManager) checkNewPod(pod, prev *core_v1.Pod) {
	l := log.New().WithFields(log.Fields{"by": PM, "method": "checkNewPod(" + pod.Name + ")"})

	//-------------------------------------
	// Basic Checks
	//-------------------------------------

	// Is this pod from the kube-system?
	if pod.Namespace == "kube-system" {
		l.Infof("Pod %s belongs to the kube-system namespace: no point in checking for policies. Will stop here.", pod.Name)
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
	k8sPolicies, _ := manager.knpc.GetPolicies(nil, &pcn_types.ObjectQuery{By: "name", Name: pod.Namespace})
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
	l := log.New().WithFields(log.Fields{"by": PM, "method": "getOrCreateFirewallManager(" + pod.Name + ")"})
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
				l.Warningf("%s was not linked in previous firewall manager!", pod.UID)
			} else {
				if remaining == 0 {
					manager.flagForDeletion(prevFw.Name())
				}
				delete(manager.linkedPods, pod.UID)
			}
		} else {
			l.Warningf("Could not find %s previous firewall manager!", pod.UID)
		}
	}

	//-------------------------------------
	// Create and link it
	//-------------------------------------
	fw, exists := manager.localFirewalls[fwKey]
	if !exists {
		manager.localFirewalls[fwKey] = startFirewall(manager.fwAPI, manager.podController, manager.vPodsRange, fwKey, pod.Namespace, pod.Labels)
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
		manager.flaggedForDeletion[fwKey] = time.AfterFunc(time.Minute*time.Duration(manager.unscheduleThreshold), func() {
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

	l := log.New().WithFields(log.Fields{"by": PM, "method": "manageDeletedPod(" + pod.Name + ")"})

	if pod.Namespace == "kube-system" {
		l.Infof("Pod %s belongs to the kube-system namespace: no point in checking its firewall manager. Will stop here.", pod.Name)
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
		l.Warningln("Could not find a firewall manager for dying pod", pod.UID, "!")
		return
	}

	wasLinked, remaining := fw.Unlink(pod, pcn_firewall.DestroyFirewall)
	if !wasLinked {
		// This pod wasn't even linked to the firewall!
		l.Warningln("Dying pod", pod.UID, "was not linked to its firewall manager", fwKey)
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
