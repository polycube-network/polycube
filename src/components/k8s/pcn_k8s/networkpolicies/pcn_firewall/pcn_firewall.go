package pcnfirewall

import (
	"net"
	"strings"
	"sync"
	"time"

	pcn_controllers "github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/controllers"
	pcn_types "github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/types"
	k8sfirewall "github.com/polycube-network/polycube/src/components/k8s/utils/k8sfirewall"
	log "github.com/sirupsen/logrus"
	core_v1 "k8s.io/api/core/v1"
	meta_v1 "k8s.io/apimachinery/pkg/apis/meta/v1"
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
	// The second arguments specifies if the pod's firewall should be cleaned
	// or destroyed. It returns FALSE if the pod was not among the monitored ones,
	// and the number of remaining pods linked.
	Unlink(pod *core_v1.Pod, then UnlinkOperation) (bool, int)
	// LinkedPods returns a map of pods monitored by this firewall manager.
	LinkedPods() map[k8s_types.UID]string
	// IsPolicyEnforced returns true if this firewall enforces this policy
	IsPolicyEnforced(name string) bool
	// Name returns the name of this firewall manager
	Name() string
	// Selector returns the namespace and labels of the pods
	// monitored by this firewall manager
	Selector() (map[string]string, string)
	// EnforcePolicy enforces the provided policy.
	EnforcePolicy(policyName, policyType string, policyTime meta_v1.Time, ingress, egress []k8sfirewall.ChainRule)
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
	// ingressDefaultAction is the default action for ingress
	ingressDefaultAction string
	// egressDefaultAction is the default action for egress
	egressDefaultAction string
	// ingressPoliciesCount is the count of ingress policies enforced
	ingressPoliciesCount int
	// egressPoliciesCount is the count of egress policies enforced
	egressPoliciesCount int
	// policyTypes is a map of policies types enforced.
	// Used to know how the default action should be handled.
	policyTypes map[string]string
	// selector defines what kind of pods this firewall is monitoring
	selector selector
	// priorities is the list of priorities
	priorities []policyPriority
	// vPodsRange
	vPodsRange *net.IPNet
}

// policyPriority is the priority of this policy.
// Most recently deployed policies take precedence over the older ones.
type policyPriority struct {
	policyName string
	timestamp  time.Time
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
		// The counts
		ingressPoliciesCount: 0,
		egressPoliciesCount:  0,
		// Policy types
		policyTypes: map[string]string{},
		// Linked pods
		linkedPods: map[k8s_types.UID]string{},
		// The default actions
		ingressDefaultAction: pcn_types.ActionForward,
		egressDefaultAction:  pcn_types.ActionForward,
		// The priorities
		priorities: []policyPriority{},
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
	// Extract the rules
	//-------------------------------------
	// We are going to get all rules regardless of the policy they belong to
	ingressRules := []k8sfirewall.ChainRule{}
	egressRules := []k8sfirewall.ChainRule{}

	if len(d.ingressRules) > 0 || len(d.egressRules) > 0 {
		// -- Ingress
		for _, rules := range d.ingressRules {
			ingressRules = append(ingressRules, rules...)
		}

		// -- Egress
		for _, rules := range d.egressRules {
			egressRules = append(egressRules, rules...)
		}
	}

	//-------------------------------------
	// Inject rules and change default actions
	//-------------------------------------
	if len(ingressRules) > 0 || len(egressRules) > 0 {
		if err := d.injecter(name, ingressRules, egressRules, nil, 0, 0); err != nil {
			// injecter fails only if pod's firewall is not ok (it is dying
			// or crashed or not found) so there's no point in going on.
			l.Warningf("Injecter encountered an error upon linking the pod: %s. Will stop here.", err)
			return false
		}
	}

	// -- ingress
	err := d.updateDefaultAction(name, "ingress", d.ingressDefaultAction)
	if err != nil {
		l.Errorf("Could not update the default ingress action: %s", err)
	} else {
		_, err := d.applyRules(name, "ingress")
		if err != nil {
			l.Errorf("Could not apply ingress rules: %s", err)
		}
	}

	// -- egress
	err = d.updateDefaultAction(name, "egress", d.egressDefaultAction)
	if err != nil {
		l.Errorf("Could not update the default egress action: %s", err)
	} else {
		_, err := d.applyRules(name, "egress")
		if err != nil {
			l.Errorf("Could not apply egress rules: %s", err)
		}
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
// The second arguments specifies if the pod's firewall should be cleaned
// or destroyed. It returns FALSE if the pod was not among the monitored ones,
// and the number of remaining pods linked.
func (d *FirewallManager) Unlink(pod *core_v1.Pod, then UnlinkOperation) (bool, int) {
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
	name := "fw-" + podIP

	// Should I also destroy its firewall?
	switch then {
	case CleanFirewall:
		if i, e := d.cleanFw(name); i != nil || e != nil {
			l.Warningln("Could not properly clean firewall for the provided pod.")
		} else {
			d.updateDefaultAction(name, "ingress", pcn_types.ActionForward)
			d.applyRules(name, "ingress")
			d.updateDefaultAction(name, "egress", pcn_types.ActionForward)
			d.applyRules(name, "egress")
		}
	case DestroyFirewall:
		if err := d.destroyFw(name); err != nil {
			l.Warningln("Could not delete firewall for the provided pod:", err)
		}
	}

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
func (d *FirewallManager) EnforcePolicy(policyName, policyType string, policyTime meta_v1.Time, ingress, egress []k8sfirewall.ChainRule) {
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
	// Update default actions
	//-------------------------------------

	// update the policy type, so that later - if this policy is removed -
	// we can enforce isolation mode correctly
	d.policyTypes[policyName] = policyType
	d.updateCounts("increase", policyType)

	//-------------------------------------
	// Set its priority
	//-------------------------------------
	// By setting its priority, we know where to start injecting rules from
	iStartFrom, eStartFrom := d.setPolicyPriority(policyName, policyTime)

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
		go d.injecter(name, ingressIDs, egressIDs, &injectWaiter, iStartFrom, eStartFrom)
	}
	injectWaiter.Wait()
}

// setPolicyPriority sets the priority of the policy in the rules list
func (d *FirewallManager) setPolicyPriority(policyName string, policyTime meta_v1.Time) (int32, int32) {
	// Ingress and egress first useful ids
	iStartFrom := 0
	eStartFrom := 0

	// NOTE: we can't use a map for this operation,
	// because we need to know how many rules to skip.
	// calculateInsertionIDs has an example of this.

	// Loop through all policies
	i := 0
	newPriorities := make([]policyPriority, len(d.priorities)+1)
	found := false
	for _, currentPolicy := range d.priorities {
		// If the policy we're going to enforce has been deployed AFTER the one
		// we're checking, then it tkes precedence
		if policyTime.After(currentPolicy.timestamp) && !found {
			newPriorities[i] = policyPriority{
				policyName: policyName,
				timestamp:  policyTime.Time,
			}
			i++
			found = true
		}

		// This is done whether the policy is inserted or not in i-th place
		newPriorities[i] = currentPolicy
		i++

		// jump the rules
		if !found {
			iStartFrom += len(d.ingressRules[currentPolicy.policyName])
			eStartFrom += len(d.egressRules[currentPolicy.policyName])
		}
	}

	// If you looped through all of them and still not inserted this policy,
	// it means it must be the last
	if !found {
		newPriorities[i] = policyPriority{
			policyName: policyName,
			timestamp:  policyTime.Time,
		}
	}

	// Update the priorities
	d.priorities = newPriorities

	return int32(iStartFrom), int32(eStartFrom)
}

// removePolicyPriority removes the policy from the list of the priorities
// and reformats it.
func (d *FirewallManager) removePolicyPriority(policyName string) {
	l := log.New().WithFields(log.Fields{"by": "FirewallManager-" + d.name, "method": "removePolicyPriority"})

	// Loop through all policies to find it
	i := 0
	newPriorities := make([]policyPriority, len(d.priorities)-1)
	for _, currentPolicy := range d.priorities {
		if currentPolicy.policyName != policyName {
			newPriorities[i] = currentPolicy
			l.Infof("%s removed from policy priorities", policyName)
		}
		i++
	}

	// Update the priorities
	d.priorities = newPriorities
}

// updateCounts updates the internal counts of policies types enforced,
// making sure default actions are respected. This is just a convenient method
// used to keep core methods (EnforcePolicy and CeasePolicy) as clean and
// as readable as possible. When possible, this function is used in place of
// increaseCount or decreaseCount, as it is preferrable to do it like this.
func (d *FirewallManager) updateCounts(operation, policyType string) {
	l := log.New().WithFields(log.Fields{"by": "FirewallManager-" + d.name, "method": "updateCounts(" + operation + "," + policyType + ")"})

	// BRIEF: read increaseCounts and decreaseCounts for an explanation
	// of when and why these functions are called.

	//-------------------------------------
	// Increase
	//-------------------------------------

	increase := func() {
		directions := []string{}

		// -- Increase the counts and append the directions to update accordingly.
		if (policyType == "ingress" || policyType == "*") && d.increaseCount("ingress") {
			directions = append(directions, "ingress")
		}
		if (policyType == "egress" || policyType == "*") && d.increaseCount("egress") {
			directions = append(directions, "egress")
		}

		if len(directions) == 0 {
			return
		}

		// -- Let's now update the default actions.
		for _, ip := range d.linkedPods {
			name := "fw-" + ip
			for _, direction := range directions {
				err := d.updateDefaultAction(name, direction, pcn_types.ActionDrop)
				if err != nil {
					l.Errorf("Could not update default action for firewall %s: %s", name, direction)
				} else {
					if _, err := d.applyRules(name, direction); err != nil {
						l.Errorf("Could not apply rules for firewall %s: %s", name, direction)
					}
				}
			}
		}
	}

	//-------------------------------------
	// Decrease
	//-------------------------------------

	decrease := func() {
		directions := []string{}

		// -- Decrease the counts and append the directions to update accordingly.
		if (policyType == "ingress" || policyType == "*") && d.decreaseCount("ingress") {
			directions = append(directions, "ingress")
		}
		if (policyType == "egress" || policyType == "*") && d.decreaseCount("egress") {
			directions = append(directions, "egress")
		}

		if len(directions) == 0 {
			return
		}

		// -- Let's now update the default actions.
		for _, ip := range d.linkedPods {
			name := "fw-" + ip
			for _, direction := range directions {
				err := d.updateDefaultAction(name, direction, pcn_types.ActionForward)
				if err != nil {
					l.Errorf("Could not update default action for firewall %s: %s", name, direction)
				} else {
					if _, err := d.applyRules(name, direction); err != nil {
						l.Errorf("Could not apply rules for firewall %s: %s", name, direction)
					}
				}
			}
		}
	}

	switch operation {
	case "increase":
		increase()
	case "decrease":
		decrease()
	}
}

// increaseCount increases the count of policies enforced
// and changes the default action for the provided direction, if needed.
// It returns TRUE if the corresponding action should be updated
func (d *FirewallManager) increaseCount(which string) bool {
	// BRIEF: this function is called when a new policy is deployed
	// with the appropriate direction.
	// If there are no policies, the default action is FORWARD.
	// If there is at least one, then the default action should be updated to DROP
	// because only what is allowed is forwarded. This function returns true when
	// there is only one policy, because that's when we should actually switch
	// to DROP (we were in FORWARD)

	// Ingress
	if which == "ingress" {
		d.ingressPoliciesCount++

		if d.ingressPoliciesCount > 0 {
			d.ingressDefaultAction = pcn_types.ActionDrop
			// If this is the *first* ingress policy, then switch to drop,
			// otherwise no need to do that (it's already DROP)
			if d.ingressPoliciesCount == 1 {
				return true
			}
		}
	}

	// Egress
	if which == "egress" {
		d.egressPoliciesCount++

		if d.egressPoliciesCount > 0 {
			d.egressDefaultAction = pcn_types.ActionDrop

			if d.egressPoliciesCount == 1 {
				return true
			}
		}
	}

	return false
}

// decreaseCount decreases the count of policies enforced and changes
// the default action for the provided direction, if needed.
// It returns TRUE if the corresponding action should be updated
func (d *FirewallManager) decreaseCount(which string) bool {
	// BRIEF: this function is called when a policy must be ceased.
	// If - after ceasing it - we have no policies enforced,
	// then the default action must be FORWARD.
	// If there is at least one, then the default action should remain DROP
	// This function returns true when there are no policies enforced,
	// because that's when we should actually switch to FORWARD (we were in DROP)

	if which == "ingress" {
		d.ingressPoliciesCount--
		// Return to default=FORWARD only if there are no policies anymore
		// after removing this
		if d.ingressPoliciesCount == 0 {
			d.ingressDefaultAction = pcn_types.ActionForward
			return true
		}
	}

	if which == "egress" {
		if d.egressPoliciesCount--; d.egressPoliciesCount == 0 {
			d.egressDefaultAction = pcn_types.ActionForward
			return true
		}
	}

	return false
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

// IsPolicyEnforced returns true if this firewall enforces this policy
func (d *FirewallManager) IsPolicyEnforced(name string) bool {
	d.lock.Lock()
	defer d.lock.Unlock()

	_, exists := d.policyTypes[name]
	return exists
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

// destroyFw destroy a firewall linked by this firewall manager
func (d *FirewallManager) destroyFw(name string) error {
	_, err := d.fwAPI.DeleteFirewallByID(nil, name)
	return err
}

// cleanFw cleans the firewall linked by this firewall manager
func (d *FirewallManager) cleanFw(name string) (error, error) {
	var iErr error
	var eErr error

	if _, err := d.fwAPI.DeleteFirewallChainRuleListByID(nil, name, "ingress"); err != nil {
		iErr = err
	}
	if _, err := d.fwAPI.DeleteFirewallChainRuleListByID(nil, name, "egress"); err != nil {
		eErr = err
	}

	return iErr, eErr
}

// applyRules is a wrapper for CreateFirewallChainApplyRulesByID method.
func (d *FirewallManager) applyRules(firewall, direction string) (bool, error) {
	out, _, err := d.fwAPI.CreateFirewallChainApplyRulesByID(nil, firewall, direction)
	return out.Result, err
}