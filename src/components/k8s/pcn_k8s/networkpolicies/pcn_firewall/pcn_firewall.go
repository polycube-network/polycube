package pcnfirewall

import (
	"net"
	"sync"

	pcn_controllers "github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/controllers"
	k8sfirewall "github.com/polycube-network/polycube/src/components/k8s/utils/k8sfirewall"
	log "github.com/sirupsen/logrus"
	core_v1 "k8s.io/api/core/v1"
	k8s_types "k8s.io/apimachinery/pkg/types"
)

// PcnFirewall is the interface of the firewall manager.
type PcnFirewall interface {
	// Link adds a new pod to the list of pods that must be managed by this manager.
	// Best practice is to only link similar pods
	// (i.e.: same labels, same namespace, same node).
	// It returns TRUE if the pod was inserted,
	// FALSE if it already existed or an error occurred
	Link(pod *core_v1.Pod) bool
	// Unlink removes the  pod from the list of monitored ones by this manager.
	// It returns FALSE if the pod was not among the monitored ones,
	// and the number of remaining pods linked.
	Unlink(pod *core_v1.Pod) (bool, int)
	// LinkedPods returns a map of pods monitored by this firewall manager.
	LinkedPods() map[k8s_types.UID]string
	// Name returns the name of this firewall manager
	Name() string
	// Selector returns the namespace and labels of the pods
	// monitored by this firewall manager
	Selector() (map[string]string, string)
}

// FirewallManager is the implementation of the firewall manager.
type FirewallManager struct {
	// podController is the pod controller
	podController pcn_controllers.PodController
	// fwAPI is the low level firewall api
	fwAPI *k8sfirewall.FirewallApiService
	// ingressRules contains the ingress rules divided by policy
	ingressRules map[string][]k8sfirewall.ChainRule
	// egressRules contains the egress rules divided by policy
	egressRules map[string][]k8sfirewall.ChainRule
	// linkedPods is a map of pods monitored by this firewall manager
	linkedPods map[k8s_types.UID]string
	// Name is the name of this firewall manager
	name string
	// lock is firewall manager's main lock
	lock sync.Mutex
	// selector defines what kind of pods this firewall is monitoring
	selector selector
	// vPodsRange
	vPodsRange *net.IPNet
}

// selector is the selector for the pods this firewall manager is managing
type selector struct {
	namespace string
	labels    map[string]string
}

// StartFirewall will start a new firewall manager
func StartFirewall(API *k8sfirewall.FirewallApiService, podController pcn_controllers.PodController, vPodsRange *net.IPNet, name, namespace string, labels map[string]string) PcnFirewall {
	l := log.New().WithFields(log.Fields{"by": FWM, "method": "StartFirewall()"})
	l.Infoln("Starting Firewall Manager, with name", name)

	manager := &FirewallManager{
		// Rules
		ingressRules: map[string][]k8sfirewall.ChainRule{},
		egressRules:  map[string][]k8sfirewall.ChainRule{},
		// External APIs
		fwAPI:         API,
		podController: podController,
		// The name (its key)
		name: name,
		// Selector
		selector: selector{
			namespace: namespace,
			labels:    labels,
		},
		// Linked pods
		linkedPods: map[k8s_types.UID]string{},
		// vPodsRange
		vPodsRange: vPodsRange,
	}

	return manager
}

// Link adds a new pod to the list of pods that must be managed by this manager.
// Best practice is to only link similar pods
// (i.e.: same labels, same namespace, same node).
// It returns TRUE if the pod was inserted,
// FALSE if it already existed or an error occurred
func (d *FirewallManager) Link(pod *core_v1.Pod) bool {
	d.lock.Lock()
	defer d.lock.Unlock()

	l := log.New().WithFields(log.Fields{"by": "FirewallManager-" + d.name, "method": "Link(" + pod.Name + ")"})

	podIP := pod.Status.PodIP
	podUID := pod.UID
	name := "fw-" + podIP

	//-------------------------------------
	// Check firewall health and pod presence
	//-------------------------------------
	if ok, err := d.isFirewallOk(name); !ok {
		l.Errorf("Could not link firewall for pod %s: %s", name, err.Error())
		return false
	}
	_, alreadyLinked := d.linkedPods[podUID]
	if alreadyLinked {
		return false
	}

	//-------------------------------------
	// Finally, link it
	//-------------------------------------
	// From now on, when this firewall manager will react to events,
	// this pod's firewall will be updated as well.
	d.linkedPods[podUID] = podIP
	l.Infof("Pod %s linked.", podIP)
	return true
}

// Unlink removes the  pod from the list of monitored ones by this manager.
// It returns FALSE if the pod was not among the monitored ones,
// and the number of remaining pods linked.
func (d *FirewallManager) Unlink(pod *core_v1.Pod) (bool, int) {
	d.lock.Lock()
	defer d.lock.Unlock()

	l := log.New().WithFields(log.Fields{"by": "FirewallManager-" + d.name, "method": "Unlink(" + pod.Name + ")"})

	podUID := pod.UID

	_, ok := d.linkedPods[podUID]
	if !ok {
		// This pod was not even linked
		return false, len(d.linkedPods)
	}

	podIP := d.linkedPods[pod.UID]

	delete(d.linkedPods, podUID)
	l.Infof("Pod %s unlinked.", podIP)
	return true, len(d.linkedPods)
}

// LinkedPods returns a map of pods monitored by this firewall manager.
func (d *FirewallManager) LinkedPods() map[k8s_types.UID]string {
	d.lock.Lock()
	defer d.lock.Unlock()

	return d.linkedPods
}

// Name returns the name of this firewall manager
func (d *FirewallManager) Name() string {
	return d.name
}

// Selector returns the namespace and labels of the pods
// monitored by this firewall manager
func (d *FirewallManager) Selector() (map[string]string, string) {
	return d.selector.labels, d.selector.namespace
}

// isFirewallOk checks if the firewall is ok.
// Used to check if firewall exists and is healthy.
func (d *FirewallManager) isFirewallOk(firewall string) (bool, error) {
	// We are going to do that by reading its uuid
	if _, _, err := d.fwAPI.ReadFirewallUuidByID(nil, firewall); err != nil {
		return false, err
	}
	return true, nil
}
