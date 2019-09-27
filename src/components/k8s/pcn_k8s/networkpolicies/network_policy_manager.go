package networkpolicies

import (
	"strings"
	"sync"
	"time"

	parsers "github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/networkpolicies/parsers"

	pcn_controllers "github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/controllers"
	pcn_firewall "github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/networkpolicies/pcn_firewall"
	v1beta "github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/pkg/apis/polycube.network/v1beta"
	pcn_types "github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/types"
	"github.com/polycube-network/polycube/src/components/k8s/utils"
	core_v1 "k8s.io/api/core/v1"
	networking_v1 "k8s.io/api/networking/v1"
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
	// lock is the main lock used in the manager
	lock sync.Mutex
	// localFirewalls is a map of the firewall managers inside this node.
	localFirewalls map[string]pcn_firewall.PcnFirewallManager
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
	// servPolicies contains function to be used when unsubscribing
	servPolicies map[string]*servPolicy
}

// nsUnsubscriptor contains information about namespace events that are
// interesting for some policies.
type nsUnsubscriptor struct {
	nsUnsub map[string]func()
}

// servPolicy contains information about service events that are
// interesting for some policies.
type servPolicy struct {
	new map[string]bool
	upd map[string]bool
	del map[string]bool
}

// startFirewall is a pointer to the StartFirewall method of the
// pcn_firewall package. It is both used as a shortcut and for testing purposes.
var startFirewall = pcn_firewall.StartFirewall

// StartNetworkPolicyManager will start a new network policy manager.
func StartNetworkPolicyManager(nodeName string) PcnNetworkPolicyManager {
	logger.Infoln("Starting Network Policy Manager")

	manager := NetworkPolicyManager{
		localFirewalls:       map[string]pcn_firewall.PcnFirewallManager{},
		flaggedForDeletion:   map[string]*time.Timer{},
		linkedPods:           map[k8s_types.UID]string{},
		policyUnsubscriptors: map[string]*nsUnsubscriptor{},
		servPolicies:         map[string]*servPolicy{},
	}

	//-------------------------------------
	// Subscribe to k8s policies events
	//-------------------------------------

	// Deploy a new k8s policy
	pcn_controllers.K8sPolicies().Subscribe(pcn_types.New, manager.DeployK8sPolicy)

	// Remove a default policy
	pcn_controllers.K8sPolicies().Subscribe(pcn_types.Delete, manager.RemoveK8sPolicy)

	// Update a policy
	pcn_controllers.K8sPolicies().Subscribe(pcn_types.Update, manager.UpdateK8sPolicy)

	//-------------------------------------
	// Subscribe to policy policies events
	//-------------------------------------

	// Deploy a new policy policy
	pcn_controllers.PcnPolicies().Subscribe(pcn_types.New, manager.DeployPcnPolicy)

	// Remove a default policy
	pcn_controllers.PcnPolicies().Subscribe(pcn_types.Delete, manager.RemovePcnPolicy)

	// Update a policy
	pcn_controllers.PcnPolicies().Subscribe(pcn_types.Update, manager.UpdatePcnPolicy)

	//-------------------------------------
	// Subscribe to services events
	//-------------------------------------

	// check new service
	pcn_controllers.Services().Subscribe(pcn_types.New, manager.CheckNewService)

	// check removed
	pcn_controllers.Services().Subscribe(pcn_types.Delete, manager.CheckRemovedService)

	// checkUpdate
	pcn_controllers.Services().Subscribe(pcn_types.Update, manager.CheckUpdatedService)

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

	//--------------------------------------
	// Split to mini-policies & deploy
	//--------------------------------------

	ingressPolicies := parsers.ParseK8sIngress(policy)
	egressPolicies := parsers.ParseK8sEgress(policy)

	// -- deploy all ingress policies
	for _, in := range ingressPolicies {
		manager.deployPolicyToFw(&in, "")
	}

	// -- deploy all egress policies
	for _, eg := range egressPolicies {
		manager.deployPolicyToFw(&eg, "")
	}
}

// DeployPcnPolicy deploys a kubernetes policy in the appropriate fw managers
func (manager *NetworkPolicyManager) DeployPcnPolicy(policy, _ *v1beta.PolycubeNetworkPolicy) {
	manager.lock.Lock()
	defer manager.lock.Unlock()

	logger.Infof("Pcn policy %s has been deployed", policy.Name)

	//--------------------------------------
	// Has a service?
	//--------------------------------------
	var serv *core_v1.Service
	if policy.ApplyTo.Target == v1beta.ServiceTarget {
		if policy.ApplyTo.Any != nil && *policy.ApplyTo.Any {
			logger.Infof("Cannot set any when targeting services. Stopping now.")
			return
		}

		// Get it
		sQuery := utils.BuildQuery(policy.ApplyTo.WithName, nil)
		nQuery := utils.BuildQuery(policy.Namespace, nil)

		// Let's first create the service entry for this service.
		// So, when the service is udated, the policy is updated too.
		key := utils.BuildObjectKey(nQuery, "ns") + "|" + utils.BuildObjectKey(sQuery, "serv")
		if _, exists := manager.servPolicies[key]; !exists {
			manager.servPolicies[key] = &servPolicy{
				new: map[string]bool{},
				upd: map[string]bool{},
				del: map[string]bool{},
			}
		}

		servList, err := pcn_controllers.Services().List(sQuery, nQuery)

		if err != nil {
			logger.Errorf("Error occurred while trying to get service with name %s. Stopping now.", policy.ApplyTo.WithName)
			return
		}
		if len(servList) == 0 {
			logger.Infof("Service with name %s was not found. The policy will be parsed when it is deployed.", policy.ApplyTo.WithName)

			// Let's deploy this policy when the service is created.
			// It is useless to do it now
			manager.servPolicies[key].new[policy.Name] = true
			return
		}
		if len(servList[0].Spec.Selector) == 0 {
			logger.Infof("Service with name %s has no selectors. Stopping now.", policy.ApplyTo.WithName)
			return
		}

		serv = &servList[0]

		// We need to modify this policy when the service is updated
		manager.servPolicies[key].upd[policy.Name] = true
		manager.servPolicies[key].del[policy.Name] = true
	}

	//--------------------------------------
	// Split to mini-policies & deploy
	//--------------------------------------

	ingressPolicies := parsers.ParsePcnIngress(policy, serv)
	egressPolicies := parsers.ParsePcnEgress(policy, serv)

	// -- deploy all ingress policies
	for _, in := range ingressPolicies {
		manager.deployPolicyToFw(&in, "")
	}

	// -- deploy all egress policies
	for _, eg := range egressPolicies {
		manager.deployPolicyToFw(&eg, "")
	}
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

	//--------------------------------------
	// Split to mini-policies & deploy
	//--------------------------------------

	ingressPolicies := parsers.ParseK8sIngress(policy)
	egressPolicies := parsers.ParseK8sEgress(policy)

	//-------------------------------------
	// Unsubscribe from namespace changes
	//-------------------------------------

	// This is the function that will do it
	unsubFunc := func(name string) {
		if unsubs, exists := manager.policyUnsubscriptors[policy.Name]; exists {
			for _, unsub := range unsubs.nsUnsub {
				unsub()
			}
			delete(manager.policyUnsubscriptors, policy.Name)
		}
	}

	//-------------------------------------
	// Cease this policy
	//-------------------------------------

	for _, fwManager := range manager.localFirewalls {
		// -- Ingress
		for _, in := range ingressPolicies {
			if fwManager.IsPolicyEnforced(in.Name) {
				unsubFunc(in.Name)
				fwManager.CeasePolicy(in.Name)
			}
		}

		// -- Egress
		for _, eg := range egressPolicies {
			if fwManager.IsPolicyEnforced(eg.Name) {
				unsubFunc(eg.Name)
				fwManager.CeasePolicy(eg.Name)
			}
		}
	}
}

// RemovePcnPolicy removes (ceases) a kubernetes policy
// from the appropriate firewall managers
func (manager *NetworkPolicyManager) RemovePcnPolicy(_, policy *v1beta.PolycubeNetworkPolicy) {
	manager.lock.Lock()
	defer manager.lock.Unlock()

	logger.Infof("Pcn policy %s is going to be removed", policy.Name)

	if len(manager.localFirewalls) == 0 {
		logger.Infoln("There are no active firewall managers in this node. Will stop here.")
		return
	}

	//--------------------------------------
	// Split to mini-policies & deploy
	//--------------------------------------

	ingressPolicies := parsers.ParsePcnIngress(policy, nil)
	egressPolicies := parsers.ParsePcnEgress(policy, nil)

	//-------------------------------------
	// Unsubscribe from namespace changes
	//-------------------------------------

	// This is the function that will do it
	unsubFunc := func(name string) {
		if unsubs, exists := manager.policyUnsubscriptors[policy.Name]; exists {
			for _, unsub := range unsubs.nsUnsub {
				unsub()
			}
			delete(manager.policyUnsubscriptors, policy.Name)
		}
	}

	//-------------------------------------
	// Delete serv
	//-------------------------------------

	if policy.ApplyTo.Target == v1beta.ServiceTarget {
		sQuery := utils.BuildQuery(policy.ApplyTo.WithName, nil)
		nQuery := utils.BuildQuery(policy.Namespace, nil)
		key := utils.BuildObjectKey(nQuery, "ns") + "|" + utils.BuildObjectKey(sQuery, "serv")

		if pols, exists := manager.servPolicies[key]; exists {
			delete(pols.upd, policy.Name)
			delete(pols.new, policy.Name)
			delete(pols.del, policy.Name)
		}
	}

	//-------------------------------------
	// Cease this policy
	//-------------------------------------

	for _, fwManager := range manager.localFirewalls {
		// -- Ingress
		for _, in := range ingressPolicies {
			if fwManager.IsPolicyEnforced(in.Name) {
				unsubFunc(in.Name)
				fwManager.CeasePolicy(in.Name)
			}
		}

		// -- Egress
		for _, eg := range egressPolicies {
			if fwManager.IsPolicyEnforced(eg.Name) {
				unsubFunc(eg.Name)
				fwManager.CeasePolicy(eg.Name)
			}
		}
	}
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

	//--------------------------------------
	// Split to mini-policies & deploy
	//--------------------------------------

	ingressPolicies := parsers.ParseK8sIngress(policy)
	egressPolicies := parsers.ParseK8sEgress(policy)

	//-------------------------------------
	// Remove and redeploy
	//-------------------------------------

	for _, fwManager := range manager.localFirewalls {
		// -- Ingress
		for _, in := range ingressPolicies {
			if fwManager.IsPolicyEnforced(in.Name) {
				fwManager.CeasePolicy(in.Name)
			}
			manager.deployPolicyToFw(&in, fwManager.Name())
		}

		// -- Egress
		for _, eg := range egressPolicies {
			if fwManager.IsPolicyEnforced(eg.Name) {
				fwManager.CeasePolicy(eg.Name)
			}
			manager.deployPolicyToFw(&eg, fwManager.Name())
		}
	}
}

// UpdatePcnPolicy updates a kubernetes policy in the appropriate fw managers
func (manager *NetworkPolicyManager) UpdatePcnPolicy(policy, _ *v1beta.PolycubeNetworkPolicy) {
	manager.lock.Lock()
	defer manager.lock.Unlock()

	logger.Infof("Pcn policy %s has been updated. Going to parse it again", policy.Name)

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

	//--------------------------------------
	// Has a service?
	//--------------------------------------
	var serv *core_v1.Service
	if policy.ApplyTo.Target == v1beta.ServiceTarget {
		if policy.ApplyTo.Any != nil && *policy.ApplyTo.Any {
			logger.Infof("Cannot set any when targeting services. Stopping now.")
			return
		}

		// Get it
		sQuery := utils.BuildQuery(policy.ApplyTo.WithName, nil)
		nQuery := utils.BuildQuery(policy.Namespace, nil)
		servList, err := pcn_controllers.Services().List(sQuery, nQuery)

		if err != nil {
			logger.Errorf("Error occurred while trying to get service with name %s. Stopping now.", policy.ApplyTo.WithName)
			return
		}
		if len(servList) == 0 {
			logger.Infof("Service with name %s was not found. Stopping now.", policy.ApplyTo.WithName)
			return
		}
		if len(servList[0].Spec.Selector) == 0 {
			logger.Infof("Service with name %s has no selectors. Stopping now.", policy.ApplyTo.WithName)
			return
		}

		serv = &servList[0]
	}

	//--------------------------------------
	// Split to mini-policies & deploy
	//--------------------------------------

	ingressPolicies := parsers.ParsePcnIngress(policy, serv)
	egressPolicies := parsers.ParsePcnEgress(policy, serv)

	//-------------------------------------
	// Remove and redeploy
	//-------------------------------------

	for _, fwManager := range manager.localFirewalls {
		// -- Ingress
		for _, in := range ingressPolicies {
			if fwManager.IsPolicyEnforced(in.Name) {
				fwManager.CeasePolicy(in.Name)
			}
			manager.deployPolicyToFw(&in, fwManager.Name())
		}

		// -- Egress
		for _, eg := range egressPolicies {
			if fwManager.IsPolicyEnforced(eg.Name) {
				fwManager.CeasePolicy(eg.Name)
			}
			manager.deployPolicyToFw(&eg, fwManager.Name())
		}
	}
}

func (manager *NetworkPolicyManager) checkService(serv *core_v1.Service, eventType string) {
	// First, build the key
	sQuery := utils.BuildQuery(serv.Name, nil)
	nQuery := utils.BuildQuery(serv.Namespace, nil)
	key := utils.BuildObjectKey(nQuery, "ns") + "|" + utils.BuildObjectKey(sQuery, "serv")

	// is it there?
	polNames := func() map[string]bool {
		manager.lock.Lock()
		defer manager.lock.Unlock()

		events, exists := manager.servPolicies[key]
		if !exists {
			return nil
		}

		names := map[string]bool{}
		switch eventType {
		case "new":
			names = events.new
		case "update":
			names = events.upd
		case "delete":
			names = events.del
		}

		return names
	}()

	for polName := range polNames {
		pQuery := utils.BuildQuery(polName, nil)

		// Find them
		found, err := pcn_controllers.PcnPolicies().List(pQuery, nQuery)
		if err != nil {
			logger.Errorf("Could not get policy %s on namespace %s.", polName, serv.Namespace)
			continue
		}
		if len(found) == 0 {
			logger.Infof("No policies found with name %s namespaces %s.", polName, serv.Namespace)
			continue
		}

		pol := found[0]

		switch eventType {
		case "new":
			manager.DeployPcnPolicy(&pol, nil)
			func() {
				manager.lock.Lock()
				defer manager.lock.Unlock()
				delete(manager.servPolicies[key].new, polName)
				manager.servPolicies[key].upd[polName] = true
				manager.servPolicies[key].del[polName] = true
			}()
		case "delete":
			manager.RemovePcnPolicy(nil, &pol)
			func() {
				manager.lock.Lock()
				defer manager.lock.Unlock()
				delete(manager.servPolicies[key].upd, polName)
				delete(manager.servPolicies[key].del, polName)
				manager.servPolicies[key].new[polName] = true
			}()
		case "update":
			manager.RemovePcnPolicy(nil, &pol)
			manager.DeployPcnPolicy(&pol, nil)
		}
	}
}

// CheckNewService checks if this service needs to be protected
func (manager *NetworkPolicyManager) CheckNewService(serv, _ *core_v1.Service) {
	logger.Infof("Service %s has been deployed. Going to check for policies...", serv.Name)
	manager.checkService(serv, "new")
}

// CheckRemovedService checks if this service needs to be protected
func (manager *NetworkPolicyManager) CheckRemovedService(_, serv *core_v1.Service) {
	logger.Infof("Service %s has been removed. Going to check for policies...", serv.Name)
	manager.checkService(serv, "delete")
}

// CheckUpdatedService checks if this service needs to be protected
func (manager *NetworkPolicyManager) CheckUpdatedService(serv, prev *core_v1.Service) {
	logger.Infof("Service %s has been updated. Going to check for policies...", serv.Name)
	manager.checkService(serv, "update")
}

func (manager *NetworkPolicyManager) deployPolicyToFw(policy *pcn_types.ParsedPolicy, fwName string) {
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
	// Find the firewall managers
	//-------------------------------------

	// Let's now find the firewall managers that must enforce this policy
	appliers := []string{}

	if len(fwName) > 0 {
		fw := manager.localFirewalls[fwName]
		if checkFwManger(policy, fw) {
			appliers = append(appliers, fw.Name())
		}
	} else {
		for _, fw := range manager.localFirewalls {
			if checkFwManger(policy, fw) {
				appliers = append(appliers, fw.Name())
			}
		}
	}

	// Put this here to improve readability
	if len(appliers) == 0 {
		return
	}

	//-------------------------------------
	// Create the rules
	//-------------------------------------

	// Temporarily commented to make the warnings shut up
	rules := buildRules(policy)

	//-------------------------------------
	// Enforce this on each fw manager
	//-------------------------------------

	for _, applier := range appliers {
		manager.localFirewalls[applier].EnforcePolicy(*policy, rules)
	}

	//-------------------------------------
	// Subscribe to namespace changes
	//-------------------------------------

	if len(policy.Peer.Key) == 0 {
		return
	}

	if policy.Peer.Namespace != nil && len(policy.Peer.Namespace.Labels) > 0 {
		if _, exists := manager.policyUnsubscriptors[policy.Name]; !exists {
			manager.policyUnsubscriptors[policy.Name] = &nsUnsubscriptor{
				nsUnsub: map[string]func(){},
			}
		}
		// already there?
		nsKey := utils.ImplodeLabels(policy.Peer.Namespace.Labels, ",", false)
		if _, exists := manager.policyUnsubscriptors[policy.Name].nsUnsub[nsKey]; exists {
			return
		}

		// Subscribe
		unsub, err := pcn_controllers.Namespaces().Subscribe(pcn_types.Update, func(ns, prev *core_v1.Namespace) {
			manager.handleNamespaceUpdate(ns, prev, policy.Peer.Namespace.Labels, policy.Name, policy.Subject.Namespace, policy.ParentPolicy.Provider)
		})
		if err == nil {
			manager.policyUnsubscriptors[policy.Name].nsUnsub[nsKey] = unsub
		}
	}
}

// handleNamespaceUpdate checks if a namespace has changed its labels.
// If so, the appropriate firewall should update the policies as well.
func (manager *NetworkPolicyManager) handleNamespaceUpdate(ns, prev *core_v1.Namespace, nsLabels map[string]string, policyName, policyNs, policyProvider string) {
	logger.Infof("Namespace %s has been updated", ns.Name)

	// Function that checks if an update of the policy is needed.
	// Done like this so we can use defer and keep the code more
	// organized
	needsUpdate := func() bool {
		manager.lock.Lock()
		defer manager.lock.Unlock()

		if prev == nil {
			return false
		}

		// When should we update the firewalls in case of namespace updates?
		// 1) When previously it had some labels that were ok and now are not.
		// 2) The opposite of the 1): previously ok, now not-ok
		// Anyway, on case 1) we must remove the pods previously allowed
		// and on case 2) we must allow some new pods.
		// This simply means checking the previous state vs the current.

		previously := utils.AreLabelsContained(nsLabels, prev.Labels)
		currently := utils.AreLabelsContained(nsLabels, ns.Labels)

		if previously == currently {
			return false
		}

		logger.Infof("%s's labels have changed", ns.Name)
		return true
	}

	// Check it
	if !needsUpdate() {
		return
	}

	parentPolicyName := strings.Split(policyName, "#")[0]
	policyQuery := utils.BuildQuery(parentPolicyName, nil)
	nsQuery := utils.BuildQuery(policyNs, nil)

	// Function that handles kubernetes policies
	k8s := func() {
		policies, err := pcn_controllers.K8sPolicies().List(policyQuery, nsQuery)
		if err != nil {
			logger.Errorf("Could not load policies named %s on namespace %s. Error: %s", parentPolicyName, policyNs, err)
			return
		}
		if len(policies) == 0 {
			logger.Errorf("Policiy named %s on namespace %s has not been found.", parentPolicyName, policyNs)
			return
		}
		policy := policies[0]
		// Remove that policy and redeploy it again
		// It's a lot easier to do it like this than see what's to change :(
		manager.RemoveK8sPolicy(nil, &policy)
		manager.DeployK8sPolicy(&policy, nil)
	}

	// Function that handles pcn policies
	pcn := func() {
		policies, err := pcn_controllers.PcnPolicies().List(policyQuery, nsQuery)
		if err != nil {
			logger.Errorf("Could not load policies named %s on namespace %s. Error: %s", parentPolicyName, policyNs, err)
			return
		}
		if len(policies) == 0 {
			logger.Errorf("Policiy named %s on namespace %s has not been found.", parentPolicyName, policyNs)
			return
		}
		policy := policies[0]
		// Remove that policy and redeploy it again
		manager.RemovePcnPolicy(nil, &policy)
		manager.DeployPcnPolicy(&policy, nil)
	}

	// What to update
	if policyProvider == pcn_types.K8sProvider {
		k8s()
	} else {
		pcn()
	}
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
	linkPod := func() (bool, pcn_firewall.PcnFirewallManager) {
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

	manager.checkIfPodNeedsProtection(pod, fw)
}

func (manager *NetworkPolicyManager) checkIfPodNeedsProtection(pod *core_v1.Pod, fw pcn_firewall.PcnFirewallManager) {
	nsQuery := &pcn_types.ObjectQuery{By: "name", Name: pod.Namespace}

	//-------------------------------------
	// Kubernetes network policies
	//-------------------------------------
	k8sPolicies, _ := pcn_controllers.K8sPolicies().List(nil, nsQuery)
	for _, kp := range k8sPolicies {
		if parsers.DoesK8sPolicyAffectPod(&kp, pod) {
			ingressPolicies := parsers.ParseK8sIngress(&kp)
			egressPolicies := parsers.ParseK8sEgress(&kp)

			// -- deploy all ingress policies
			for _, in := range ingressPolicies {
				manager.deployPolicyToFw(&in, fw.Name())
			}

			// -- deploy all egress policies
			for _, eg := range egressPolicies {
				manager.deployPolicyToFw(&eg, fw.Name())
			}
		}
	}

	//-------------------------------------
	// Polycube network policies
	//-------------------------------------
	pcnPolicies, _ := pcn_controllers.PcnPolicies().List(nil, nsQuery)
	podServices, _ := pcn_controllers.Services().List(&pcn_types.ObjectQuery{By: "labels", Labels: pod.Labels}, nsQuery)

	for _, pol := range pcnPolicies {
		deploy := false
		var polService *core_v1.Service

		// Does this policy apply to the pod's service?
		if pol.ApplyTo.Target == v1beta.ServiceTarget {
			for _, serv := range podServices {
				if serv.Name == pol.ApplyTo.WithName {
					deploy = true
					polService = &serv
				}
			}
		} else {
			// this policy does not target a service, but a pod.
			// Does it apply to this pod, though?
			if (pol.ApplyTo.Any != nil && *pol.ApplyTo.Any) || utils.AreLabelsContained(pol.ApplyTo.WithLabels, pod.Labels) {
				deploy = true
			}
		}

		if !deploy {
			continue
		}

		ingressPolicies := parsers.ParsePcnIngress(&pol, polService)
		egressPolicies := parsers.ParsePcnEgress(&pol, polService)

		// -- deploy all ingress policies
		for _, in := range ingressPolicies {
			manager.deployPolicyToFw(&in, fw.Name())
		}

		// -- deploy all egress policies
		for _, eg := range egressPolicies {
			manager.deployPolicyToFw(&eg, fw.Name())
		}
	}
}

// getOrCreateFirewallManager gets a local firewall manager
// for this pod or creates one if not there.
// Returns the newly created/already existing firewall manager,
// its key, and TRUE if it was just created.
func (manager *NetworkPolicyManager) getOrCreateFirewallManager(pod, prev *core_v1.Pod) (pcn_firewall.PcnFirewallManager, bool) {
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
