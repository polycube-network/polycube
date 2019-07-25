package pcnfirewall

import (
	"net"
	"strings"
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
	// EnforcePolicy enforces the provided policy.
	EnforcePolicy(policyName, policyType string, ingress, egress []k8sfirewall.ChainRule)
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

// EnforcePolicy enforces the provided policy.
func (d *FirewallManager) EnforcePolicy(policyName, policyType string, ingress, egress []k8sfirewall.ChainRule) {
	// Only one policy at a time, please
	d.lock.Lock()
	defer d.lock.Unlock()

	l := log.New().WithFields(log.Fields{"by": "FirewallManager-" + d.name, "method": "EnforcePolicy"})
	l.Infof("Enforcing policy %s...", policyName)

	//-------------------------------------
	// Store the rules
	//-------------------------------------

	ingressIDs, egressIDs := d.storeRules(policyName, "", ingress, egress)

	//-------------------------------------
	// Inject the rules on each firewall
	//-------------------------------------

	if len(d.linkedPods) == 0 {
		l.Infoln("There are no linked pods. Stopping here.")
		return
	}

	var injectWaiter sync.WaitGroup
	injectWaiter.Add(len(d.linkedPods))

	for _, ip := range d.linkedPods {
		name := "fw-" + ip
		go d.injecter(name, ingressIDs, egressIDs, &injectWaiter, 0, 0)
	}
	injectWaiter.Wait()
}

// storeRules stores rules in memory according to their policy
func (d *FirewallManager) storeRules(policyName, target string, ingress, egress []k8sfirewall.ChainRule) ([]k8sfirewall.ChainRule, []k8sfirewall.ChainRule) {
	if _, exists := d.ingressRules[policyName]; !exists {
		d.ingressRules[policyName] = []k8sfirewall.ChainRule{}
	}
	if _, exists := d.egressRules[policyName]; !exists {
		d.egressRules[policyName] = []k8sfirewall.ChainRule{}
	}

	description := "policy=" + policyName
	newIngress := make([]k8sfirewall.ChainRule, len(ingress))
	newEgress := make([]k8sfirewall.ChainRule, len(egress))

	// --- ingress
	for i, rule := range ingress {
		newIngress[i] = rule
		newIngress[i].Description = description
		if len(target) > 0 {
			newIngress[i].Dst = target
		}

		d.ingressRules[policyName] = append(d.ingressRules[policyName], newIngress[i])
	}

	// -- Egress
	for i, rule := range egress {
		newEgress[i] = rule
		newEgress[i].Description = description
		if len(target) > 0 {
			newEgress[i].Src = target
		}

		d.egressRules[policyName] = append(d.egressRules[policyName], newEgress[i])
	}
	return newIngress, newEgress
}

// injecter is a convenient method for injecting rules for a single firewall
// for both directions
func (d *FirewallManager) injecter(firewall string, ingressRules, egressRules []k8sfirewall.ChainRule, waiter *sync.WaitGroup, iStartFrom, eStartFrom int32) error {
	l := log.New().WithFields(log.Fields{"by": "FirewallManager-" + d.name, "method": "Injecter(" + firewall + ", ...)"})

	// Should I notify caller when I'm done?
	if waiter != nil {
		defer waiter.Done()
	}

	// Is firewall ok?
	if ok, err := d.isFirewallOk(firewall); !ok {
		l.Errorf("Could not inject rules. Firewall is not ok: %s", err)
		return err
	}

	//-------------------------------------
	// Inject rules direction concurrently
	//-------------------------------------
	var injectWaiter sync.WaitGroup
	injectWaiter.Add(2)
	defer injectWaiter.Wait()

	go d.injectRules(firewall, "ingress", ingressRules, &injectWaiter, iStartFrom)
	go d.injectRules(firewall, "egress", egressRules, &injectWaiter, eStartFrom)

	return nil
}

// injectRules is a wrapper for firewall's CreateFirewallChainRuleListByID
// and CreateFirewallChainApplyRulesByID methods.
func (d *FirewallManager) injectRules(firewall, direction string, rules []k8sfirewall.ChainRule, waiter *sync.WaitGroup, startFrom int32) error {
	l := log.New().WithFields(log.Fields{"by": "FirewallManager-" + d.name, "method": "injectRules(" + firewall + "," + direction + ",...)"})

	// Should I notify caller when I'm done?
	if waiter != nil {
		defer waiter.Done()
	}

	//-------------------------------------
	// Inject & apply
	//-------------------------------------
	// The ip of the pod we are protecting. Used for the SRC or the DST
	me := strings.Split(firewall, "-")[1]

	// We are using the insert call here, which adds the rule on the startFrom id
	// and pushes the other rules downwards. In order to preserve original order,
	// we're going to start injecting from the last to the first.

	len := len(rules)
	for i := len - 1; i > -1; i-- {
		ruleToInsert := k8sfirewall.ChainInsertInput(rules[i])
		ruleToInsert.Id = startFrom

		// There is only the pod on the other side of the link, so all packets
		// travelling there are obviously either going to or from the pod.
		// So there is no need to include the pod as a destination or source
		// in the rules. But it helps to keep them clearer and precise.
		if direction == "ingress" {
			ruleToInsert.Src = me
		} else {
			ruleToInsert.Dst = me
		}

		_, response, err := d.fwAPI.CreateFirewallChainInsertByID(nil, firewall, direction, ruleToInsert)
		if err != nil {
			l.Errorf("Error while trying to inject rule: %s, %+v", err, response)
			// This rule had an error, but we still gotta push the other ones dude...
			//return err
		}
	}

	// Now apply the changes
	if response, err := d.applyRules(firewall, direction); err != nil {
		l.Errorf("Error while trying to apply rules: %s, %+v", err, response)
		return err
	}

	return nil
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

// updateDefaultAction is a wrapper for UpdateFirewallChainDefaultByID method.
func (d *FirewallManager) updateDefaultAction(firewall, direction, to string) error {
	// To is enclosed between \" because otherwise the swagger-generated API
	// would wrongly send forward instead of "forward".
	actualTo := "\"" + to + "\""
	_, err := d.fwAPI.UpdateFirewallChainDefaultByID(nil, firewall, direction, actualTo)
	return err
}

// applyRules is a wrapper for CreateFirewallChainApplyRulesByID method.
func (d *FirewallManager) applyRules(firewall, direction string) (bool, error) {
	out, _, err := d.fwAPI.CreateFirewallChainApplyRulesByID(nil, firewall, direction)
	return out.Result, err
}
