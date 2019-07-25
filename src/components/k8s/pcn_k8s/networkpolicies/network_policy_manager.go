package networkpolicies

import (
	"net"
	"sync"

	pcn_controllers "github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/controllers"
	pcn_firewall "github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/networkpolicies/pcn_firewall"
	pcn_types "github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/types"
	"github.com/polycube-network/polycube/src/components/k8s/utils"
	k8sfirewall "github.com/polycube-network/polycube/src/components/k8s/utils/k8sfirewall"
	log "github.com/sirupsen/logrus"
	core_v1 "k8s.io/api/core/v1"
	k8s_types "k8s.io/apimachinery/pkg/types"
)

// PcnNetworkPolicyManager is a network policy manager
type PcnNetworkPolicyManager interface {
}

// NetworkPolicyManager is the implementation of the policy manager
type NetworkPolicyManager struct {
	// podController is the pod controller
	podController pcn_controllers.PodController
	// fwAPI is the firewall API
	fwAPI *k8sfirewall.FirewallApiService
	// nodeName is the name of the node in which we are running
	nodeName string
	// lock is the main lock used in the manager
	lock sync.Mutex
	// localFirewalls is a map of the firewall managers inside this node.
	localFirewalls map[string]pcn_firewall.PcnFirewall
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
func StartNetworkPolicyManager(vPodsRange *net.IPNet, basePath, nodeName string, podController pcn_controllers.PodController) PcnNetworkPolicyManager {
	l := log.New().WithFields(log.Fields{"by": PM, "method": "StartNetworkPolicyManager()"})
	l.Debugln("Starting Network Policy Manager")

	cfgK8firewall := k8sfirewall.Configuration{BasePath: basePath}
	srK8firewall := k8sfirewall.NewAPIClient(&cfgK8firewall)
	fwAPI := srK8firewall.FirewallApi

	manager := NetworkPolicyManager{
		podController:  podController,
		nodeName:       nodeName,
		localFirewalls: map[string]pcn_firewall.PcnFirewall{},
		linkedPods:     map[k8s_types.UID]string{},
		fwAPI:          fwAPI,
		vPodsRange:     vPodsRange,
	}

	//-------------------------------------
	// Subscribe to pod events
	//-------------------------------------

	podController.Subscribe(pcn_types.Update, nil, nil, &pcn_types.ObjectQuery{Name: nodeName}, pcn_types.PodRunning, manager.checkNewPod)
	podController.Subscribe(pcn_types.Delete, nil, nil, &pcn_types.ObjectQuery{Name: nodeName}, pcn_types.PodAnyPhase, manager.manageDeletedPod)

	return &manager
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

	l.Infof("Pod %s has been linked and its firewall manager %s has been created.", pod.Name, fw.Name())
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
					l.Infof("Firewall Manager %s is now empty", prevFw.Name())
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
		l.Infof("Firewall manager %s is now empty", fwKey)
	}
}
