package networkpolicies

import (
	"net"
	"sync"

	pcn_controllers "github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/controllers"
	pcn_firewall "github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/networkpolicies/pcn_firewall"
	pcn_types "github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/types"
	log "github.com/sirupsen/logrus"
	core_v1 "k8s.io/api/core/v1"
)

// PcnNetworkPolicyManager is a network policy manager
type PcnNetworkPolicyManager interface {
}

// NetworkPolicyManager is the implementation of the policy manager
type NetworkPolicyManager struct {
	// podController is the pod controller
	podController pcn_controllers.PodController
	// nodeName is the name of the node in which we are running
	nodeName string
	// lock is the main lock used in the manager
	lock sync.Mutex
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

	manager := NetworkPolicyManager{
		podController: podController,
		nodeName:      nodeName,
		vPodsRange:    vPodsRange,
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

	l.Infof("Pod %s detected in a Running phase", pod.Name)
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

	l.Infof("Pod %s detected dead.", pod.Name)
}
