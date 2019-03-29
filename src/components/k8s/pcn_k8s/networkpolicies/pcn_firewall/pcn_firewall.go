package pcnfirewall

import (
	"errors"
	"sync"

	//	TODO-ON-MERGE: change these to the polycube path
	pcn_controllers "github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/controllers"
	pcn_types "github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/types"
	k8sfirewall "github.com/SunSince90/polycube/src/components/k8s/utils/k8sfirewall"

	log "github.com/sirupsen/logrus"
	core_v1 "k8s.io/api/core/v1"
	k8s_types "k8s.io/apimachinery/pkg/types"
)

type PcnFirewall interface {
	EnforcePolicy(string, string, []k8sfirewall.ChainRule, []k8sfirewall.ChainRule, pcn_types.FirewallActions) (error, error)
	CeasePolicy(string)
	ForPod() k8s_types.UID
	RemoveRules(string, []k8sfirewall.ChainRule) []k8sfirewall.ChainRule
	RemoveIPReferences(string)
	ImplementsPolicy(string) bool
	Destroy() error
}

type DeployedFirewall struct {
	firewall *k8sfirewall.Firewall
	rules    map[string]*rulesContainer

	podController pcn_controllers.PodController

	ingressRules map[string][]k8sfirewall.ChainRule
	egressRules  map[string][]k8sfirewall.ChainRule

	/*ingressChain *k8sfirewall.Chain
	egressChain  *k8sfirewall.Chain*/
	fwAPI  k8sfirewall.FirewallAPI
	podUID k8s_types.UID
	podIP  string

	policyTypes          map[string]string
	policyActions        map[string]*policyActions
	ingressPoliciesCount int
	egressPoliciesCount  int
	//	For caching.
	/*lastIngressID int32
	lastEgressID int32*/
	lock sync.Mutex
}

type rulesContainer struct {
	ingress []k8sfirewall.ChainRule
	egress  []k8sfirewall.ChainRule
}

type policyActions struct {
	ingress []func()
	egress  []func()
}

func newFirewall(pod core_v1.Pod, API k8sfirewall.FirewallAPI, podController pcn_controllers.PodController) *DeployedFirewall {

	//	This method is unexported by design: *only* the firewall manager is supposed to get new firewalls.

	var l = log.WithFields(log.Fields{
		"by":     "PcnFirewall",
		"method": "New()",
	})

	//-------------------------------------
	//	Init
	//-------------------------------------

	//	The name of the firewall
	name := "fw-" + pod.Status.PodIP

	deployedFw := DeployedFirewall{}

	//	The chains are here just for storing a default action (and maybe contain stats in future)
	/*deployedFw.ingressChain = &k8sfirewall.Chain{
		Name:     "ingress",
		Default_: "drop",
	}
	deployedFw.egressChain = &k8sfirewall.Chain{
		Name:     "egress",
		Default_: "drop",
	}*/

	//	Rules
	deployedFw.rules = map[string]*rulesContainer{}
	deployedFw.ingressRules = map[string][]k8sfirewall.ChainRule{}
	deployedFw.egressRules = map[string][]k8sfirewall.ChainRule{}

	//	The firewall API
	deployedFw.fwAPI = API

	//	The pod this firewall is intended for
	deployedFw.podUID = pod.UID

	//	The pod's ip
	deployedFw.podIP = pod.Status.PodIP

	deployedFw.ingressPoliciesCount = 0
	deployedFw.egressPoliciesCount = 0
	deployedFw.policyTypes = map[string]string{}
	deployedFw.policyActions = map[string]*policyActions{}

	deployedFw.podController = podController

	//-------------------------------------
	//	Get the firewall
	//-------------------------------------

	fw, response, err := deployedFw.fwAPI.ReadFirewallByID(nil, name)

	if err != nil {
		l.Errorln("Could not get firewall with name", name, ":", err, response)

		if response.StatusCode != 200 {
			l.Errorln("The firewall is nil. Will stop now.")
			return nil
		}
	}

	deployedFw.firewall = &fw

	// Since Interactive is not sent in the request, we will do it again now
	response, err = deployedFw.fwAPI.UpdateFirewallInteractiveByID(nil, name, false)

	if err != nil {
		l.Warningln("Could not set interactive to false for firewall", name, ":", err, response, ". Applying rules may take some time!")
	}

	//	TODO: read the firewall by ID in order to get the other components?

	return &deployedFw
}

func (d *DeployedFirewall) ForPod() k8s_types.UID {
	return d.podUID
}

func (d *DeployedFirewall) EnforcePolicy(policyName, policyType string, ingress, egress []k8sfirewall.ChainRule, actions pcn_types.FirewallActions) (error, error) {

	//-------------------------------------
	//	Init
	//-------------------------------------

	var l = log.WithFields(log.Fields{
		"by":     d.firewall.Name,
		"method": "InjectRules()",
	})

	var iError error
	var eError error
	var applyWait sync.WaitGroup
	waitChains := 0

	//	Default values (empty)
	injectedRules := rulesContainer{
		ingress: []k8sfirewall.ChainRule{},
		egress:  []k8sfirewall.ChainRule{},
	}

	//	How many threads should we wait for?
	if len(ingress) > 0 {
		waitChains++
	}
	if len(egress) > 0 {
		waitChains++
	}
	applyWait.Add(waitChains)

	//-------------------------------------
	//	Inject rules concurrently
	//-------------------------------------

	//	Only one policy at a time, please
	d.lock.Lock()
	defer d.lock.Unlock()

	if !d.isFirewallOk() {
		l.Errorln("Firewall seems not to be ok! Will not inject rules.")
		return errors.New("Firewall is not ok"), errors.New("Firewall is not ok")
	}

	//	Ingress
	if len(ingress) > 0 {
		go func() {
			defer applyWait.Done()

			if rulesWithIds, err := d.injectRules("ingress", ingress); err == nil {
				injectedRules.ingress = rulesWithIds
			} else {
				iError = err
			}
		}()
	}

	//	Egress
	if len(egress) > 0 {
		go func() {
			defer applyWait.Done()

			if rulesWithIds, err := d.injectRules("egress", egress); err == nil {
				injectedRules.egress = rulesWithIds
			} else {
				eError = err
			}
		}()
	}

	applyWait.Wait()

	//-------------------------------------
	//	Update rules struct
	//-------------------------------------

	//	If at least something succeded, then we can specify that this firewall implements this policy
	if iError == nil || eError == nil {
		//	Add the newly created rules on our struct, so we can reference them at all times

		//	Ingress
		if len(injectedRules.ingress) > 0 {
			if _, exists := d.ingressRules[policyName]; !exists {
				d.ingressRules[policyName] = []k8sfirewall.ChainRule{}
			}
			d.ingressRules[policyName] = append(d.ingressRules[policyName], injectedRules.ingress...)
		}

		//	Egress
		if len(injectedRules.egress) > 0 {
			if _, exists := d.egressRules[policyName]; !exists {
				d.egressRules[policyName] = []k8sfirewall.ChainRule{}
			}
			d.egressRules[policyName] = append(d.egressRules[policyName], injectedRules.egress...)
		}
	}

	//-------------------------------------
	//	Update Reactions & default action
	//-------------------------------------

	//	So we just enforced a new policy. The final step is to change actions (if needed: that's what increaseCount does)
	//	But only if we did not do that already!
	if _, exists := d.policyTypes[policyName]; !exists {

		//	---	Update default actions
		d.policyTypes[policyName] = policyType
		switch policyType {
		case "ingress":
			d.increaseCount("ingress")
		case "egress":
			d.increaseCount("egress")
		case "*":
			d.increaseCount("ingress")
			d.increaseCount("egress")
		}

		//	---	React to pod events
		if len(actions.Ingress) > 0 || len(actions.Egress) > 0 {
			d.definePolicyActions(policyName, actions.Ingress, actions.Egress)
		}
	}

	return iError, eError
}

func (d *DeployedFirewall) definePolicyActions(policyName string, ingress, egress []pcn_types.FirewallAction) {

	d.policyActions[policyName] = &policyActions{}

	//-------------------------------------
	//	Ingress
	//-------------------------------------
	for _, i := range ingress {

		//	Subscribe to the pod controller for this specific object
		subscription, err := d.podController.Subscribe(pcn_types.Update, pcn_types.ObjectQuery{
			Labels: i.PodLabels,
		}, pcn_types.ObjectQuery{
			Name:   i.NamespaceName,
			Labels: i.NamespaceLabels,
		}, pcn_types.PodRunning, func(pod *core_v1.Pod) {
			d.reactToPod(pod, policyName, i.Actions)
		})

		if err == nil {
			d.policyActions[policyName].ingress = append(d.policyActions[policyName].ingress, subscription)
		}
	}

	//-------------------------------------
	//	Egress
	//-------------------------------------
	for _, e := range egress {

		subscription, err := d.podController.Subscribe(pcn_types.Update, pcn_types.ObjectQuery{
			Labels: e.PodLabels,
		}, pcn_types.ObjectQuery{
			Name:   e.NamespaceName,
			Labels: e.NamespaceLabels,
		}, pcn_types.PodRunning, func(pod *core_v1.Pod) {
			d.reactToPod(pod, policyName, e.Actions)
		})

		if err == nil {
			d.policyActions[policyName].egress = append(d.policyActions[policyName].egress, subscription)
		}
	}

}

func (d *DeployedFirewall) reactToPod(pod *core_v1.Pod, policyName string, action pcn_types.ParsedRules) {
	ingress := make([]k8sfirewall.ChainRule, len(action.Ingress))
	egress := make([]k8sfirewall.ChainRule, len(action.Egress))

	//	Ingress
	for i := 0; i < len(action.Ingress); i++ {
		ingress[i] = action.Ingress[i]
		ingress[i].Src = pod.Status.PodIP
	}

	//	Egress
	for i := 0; i < len(action.Ingress); i++ {
		egress[i] = action.Egress[i]
		egress[i].Dst = pod.Status.PodIP
	}

	//	No need to specify a policy type
	d.EnforcePolicy(policyName, "", ingress, egress, pcn_types.FirewallActions{})
}

func (d *DeployedFirewall) CeasePolicy(policyName string) {
	var l = log.WithFields(log.Fields{
		"by":     d.firewall.Name,
		"method": "CeasePolicy(" + policyName + ")",
	})

	var deleteWait sync.WaitGroup
	deleteNumber := 0

	failedRules := rulesContainer{
		ingress: []k8sfirewall.ChainRule{},
		egress:  []k8sfirewall.ChainRule{},
	}

	d.lock.Lock()
	defer d.lock.Unlock()

	//	Do they exist?
	policyIngress, iexists := d.ingressRules[policyName]
	policyEgress, eexists := d.ingressRules[policyName]
	if !iexists && !eexists {
		l.Infoln("fw", d.firewall.Name, "has no", policyName, "in its list of implemented policies rules")
		return
	}

	//	What type was this policy?
	policyType := d.policyTypes[policyName]

	//-------------------------------------
	//	Are there any rules on this policy?
	//-------------------------------------
	//	NOTE: check on exists and len are actually useless (read EnforcePolicy). But since I'm paranoid, I'll do it anyway.

	//	Check for ingress rules
	if iexists && len(policyIngress) < 1 {
		delete(d.ingressRules, policyName)
	}
	//	Check for egress rules
	if eexists && len(policyEgress) < 1 {
		delete(d.ingressRules, policyName)
	}

	//	After the above, policy is not even listed anymore? (which means: both ingress and egress were actually empty?)
	policyIngress, iexists = d.ingressRules[policyName]
	policyEgress, eexists = d.ingressRules[policyName]
	if !iexists && !eexists {

		//	They were empty: no point in going on. Let's now decrease counts.
		switch policyType {
		case "ingress":
			d.decreaseCount("ingress")
		case "egress":
			d.decreaseCount("egress")
		case "*":
			d.decreaseCount("ingress")
			d.decreaseCount("egress")
		}

		return
	}

	//-------------------------------------
	//	Remove the rules
	//-------------------------------------

	if !d.isFirewallOk() {
		l.Errorln("Firewall seems not to be ok! Will not remove rules.")
		return
	}

	if iexists && len(policyIngress) > 0 {
		deleteNumber++
	}
	if eexists && len(policyEgress) > 0 {
		deleteNumber++
	}
	deleteWait.Add(deleteNumber)

	//	Ingress
	if iexists && len(policyIngress) > 0 {
		go func(rules []k8sfirewall.ChainRule) {
			defer deleteWait.Done()

			iFailedRules := d.RemoveRules("ingress", rules)
			if len(iFailedRules) > 0 {
				failedRules.ingress = iFailedRules
			}
		}(policyIngress)
	}

	//	Egress
	if eexists && len(policyEgress) > 0 {
		go func(rules []k8sfirewall.ChainRule) {
			defer deleteWait.Done()

			eFailedRules := d.RemoveRules("egress", rules)
			if len(eFailedRules) > 0 {
				failedRules.egress = eFailedRules
			}
		}(policyEgress)
	}

	deleteWait.Wait()

	//-------------------------------------
	//	Update the enforced policies
	//-------------------------------------

	if len(failedRules.ingress) < 1 && len(failedRules.egress) < 1 {
		//	All rules were delete successfully: we may delete the entry
		delete(d.ingressRules, policyName)
		delete(d.egressRules, policyName)

		delete(d.policyTypes, policyName)

	} else {
		//	Some rules were not deleted. We can't delete the entry: we need to change it with the still active rules.
		d.ingressRules[policyName] = failedRules.ingress
		d.egressRules[policyName] = failedRules.egress
	}

	//-------------------------------------
	//	Update the actions
	//-------------------------------------

	//	We just removed a policy. We must change the actions (if needed: that's what decreaseCount does)
	switch policyType {
	case "ingress":
		d.decreaseCount("ingress")
	case "egress":
		d.decreaseCount("egress")
	case "*":
		d.decreaseCount("ingress")
		d.decreaseCount("egress")
	}

	delete(d.policyActions, policyName)
}

func (d *DeployedFirewall) injectRules(direction string, rules []k8sfirewall.ChainRule) ([]k8sfirewall.ChainRule, error) {

	//	UPDATE: it's better to always inject with a policy instead of doing it anonymously.
	//	That's why this function is un-exported now

	var l = log.WithFields(log.Fields{
		"by":     d.firewall.Name,
		"method": "injectRules()",
	})

	//	Just in case...
	if rules == nil {
		//l.Errorln("Rules is nil")
		return nil, errors.New("Rules is nil")
	}

	if len(rules) < 1 {
		//l.Errorln("No rules to inject")
		return nil, errors.New("No rules to inject")
	}

	var ID int32 = 1
	rulesToInject := []k8sfirewall.ChainRule{}

	//-------------------------------------
	//	Build the IDs & put my pod's IP
	//-------------------------------------

	chain, response, err := d.fwAPI.ReadFirewallChainByID(nil, d.firewall.Name, direction)
	if err != nil {
		l.Errorln("Error while trying to get chain for firewall", d.firewall.Name, "in", direction, ":", err, response)
		return nil, err
	}

	//	The last ID is always the default one's, which always increments by one as we push rules.
	if len(chain.Rule) > 1 {
		ID = chain.Rule[len(chain.Rule)-2].Id + 1
	}

	//	Modify the rules
	for _, rule := range rules {
		workedRule := k8sfirewall.ChainRule{}
		if direction == "ingress" {
			//	Make sure not to target myself
			if rule.Src != d.podIP {
				workedRule = rule
				workedRule.Dst = d.podIP
			}
		}
		if direction == "egress" {
			//	Make sure not to target myself
			if rule.Dst != d.podIP {
				workedRule = rule
				workedRule.Src = d.podIP
			}
		}

		if len(workedRule.Action) > 0 {
			workedRule.Id = ID
			ID++
			rulesToInject = append(rulesToInject, workedRule)
		}
	}

	if len(rulesToInject) < 1 {
		return rulesToInject, nil
	}

	//-------------------------------------
	//	Actually Inject
	//-------------------------------------

	response, err = d.fwAPI.CreateFirewallChainRuleListByID(nil, d.firewall.Name, direction, rulesToInject)
	if err != nil {
		l.Errorln("Error while trying to inject rules for firewall", d.firewall.Name, "in", direction, ":", err, response)
		return []k8sfirewall.ChainRule{}, err
	}

	if response, err := d.applyRules(direction); err != nil {
		l.Errorln("Error while trying to apply rules", d.firewall.Name, "in", direction, ":", err, response)
	}

	return rulesToInject, nil
}

func (d *DeployedFirewall) RemoveRules(direction string, rules []k8sfirewall.ChainRule) []k8sfirewall.ChainRule {
	var l = log.WithFields(log.Fields{
		"by":     d.firewall.Name,
		"method": "RemoveRules(" + direction + ")",
	})

	//	Just in case...
	if rules == nil {
		err := errors.New("Rules is nil")
		l.Errorln(err.Error())
		return nil
	}

	//	Make sure to call this function after checking for rules
	if len(rules) < 1 {
		l.Warningln("There are no rules to remove.")
		return []k8sfirewall.ChainRule{}
	}

	//	1) delete the rule
	//	2) if successful, do nothing
	//	3) if not, add this rule to the failed ones
	failedRules := []k8sfirewall.ChainRule{}

	//	No need to do this with separate threads...
	for _, rule := range rules {
		response, err := d.fwAPI.DeleteFirewallChainRuleByID(nil, d.firewall.Name, direction, rule.Id)
		if err != nil {
			l.Errorln("Error while trying to delete rule", rule.Id, "in", direction, "for firewall", d.firewall.Name, err, response)
			failedRules = append(failedRules, rule)
		}
	}

	if _, err := d.applyRules(direction); err != nil {
		l.Errorln("Error while trying to apply rules", d.firewall.Name, "in", direction, ":", err)
	}

	//	Hopefully this is empty
	return failedRules
}

func (d *DeployedFirewall) RemoveIPReferences(ip string) {

	ingressIDs := []k8sfirewall.ChainRule{}
	egressIDs := []k8sfirewall.ChainRule{}

	d.lock.Lock()
	defer d.lock.Unlock()

	var directionsWait sync.WaitGroup
	directionsWait.Add(2)

	go func() {
		defer directionsWait.Done()
		for _, v := range d.ingressRules {
			for _, i := range v {
				if i.Src == ip {
					ingressIDs = append(ingressIDs, i)
				}
			}
		}
	}()

	go func() {
		defer directionsWait.Done()
		for _, v := range d.egressRules {
			for _, i := range v {
				if i.Dst == ip {
					egressIDs = append(egressIDs, i)
				}
			}
		}
	}()

	directionsWait.Wait()

	if len(ingressIDs) < 1 && len(egressIDs) < 1 {
		return
	}

	if len(ingressIDs) > 0 {
		d.RemoveRules("ingress", ingressIDs)
	}
	if len(egressIDs) > 0 {
		d.RemoveRules("egress", egressIDs)
	}
}

func (d *DeployedFirewall) increaseCount(which string) {

	//	NOTE: this function must be called while holding a lock!
	//	If there is at least one policy, then we must switch the default action to DROP for that type
	//	E.g.: if the policy had only INGRESS in its spec, then the ingress chain must be set to drop

	if which != "ingress" && which != "egress" {
		return
	}

	//	Ingress
	if which == "ingress" {
		d.ingressPoliciesCount++

		if d.ingressPoliciesCount == 1 {
			d.updateDefaultAction("ingress", pcn_types.ActionDrop)
			d.applyRules("ingress")
		}

		return
	}

	//	Egress
	d.egressPoliciesCount++

	if d.egressPoliciesCount == 1 {
		d.updateDefaultAction("egress", pcn_types.ActionDrop)
	}

}

func (d *DeployedFirewall) decreaseCount(which string) {

	//	NOTE: this function must be called while holding a lock!
	//	If there are no policies enforced, then we must switch the default action to FORWARD for that type
	//	E.g.: if the policy had only INGRESS in its spec, then the ingress chain must be set to FORWARD

	if which != "ingress" && which != "egress" {
		return
	}

	//	Ingress
	if which == "ingress" {
		d.ingressPoliciesCount--

		if d.ingressPoliciesCount == 0 {
			d.updateDefaultAction("ingress", pcn_types.ActionForward)
			d.applyRules("ingress")
		}

		return
	}

	//	Egress
	d.egressPoliciesCount--

	if d.egressPoliciesCount == 0 {
		d.updateDefaultAction("egress", pcn_types.ActionForward)
		d.applyRules("egress")
	}

}

func (d *DeployedFirewall) ImplementsPolicy(name string) bool {

	d.lock.Lock()
	defer d.lock.Unlock()

	//_, exists := d.rules[name]
	//return exists
	_, iexists := d.ingressRules[name]
	_, eexists := d.egressRules[name]

	return iexists || eexists
}

func (d *DeployedFirewall) isFirewallOk() bool {
	//	This should be done *before* injecting/remove rules:
	//	polycube crashes if you try to delete a rule on a firewall that does not exist.
	//	(this will happen when you remove a policy *while* a pod is in terminating phase)
	//	NOTE: we are not going to Read the whole firewall just for this. Let's make this fast.
	if _, _, err := d.fwAPI.ReadFirewallUuidByID(nil, d.firewall.Name); err != nil {
		return false
	}

	return true
}

func (d *DeployedFirewall) updateDefaultAction(direction, to string) error {
	var l = log.WithFields(log.Fields{
		"by":     d.firewall.Name,
		"method": "updateDefaultAction(" + direction + "," + to + ")",
	})

	response, err := d.fwAPI.UpdateFirewallChainDefaultByID(nil, d.firewall.Name, direction, to)

	if err != nil {
		l.Errorln("Could not set default", direction, "action to forward for firewall", d.firewall.Name, ":", err, response)
	}

	return err
}

func (d *DeployedFirewall) applyRules(direction string) (bool, error) {
	out, _, err := d.fwAPI.CreateFirewallChainApplyRulesByID(nil, d.firewall.Name, direction)
	return out.Result, err
}

func (d *DeployedFirewall) Destroy() error {
	var l = log.WithFields(log.Fields{
		"by":     d.firewall.Name,
		"method": "Destroy()",
	})

	if response, err := d.fwAPI.DeleteFirewallByID(nil, d.firewall.Name); err != nil {
		l.Errorln("Failed to destroy firewall,", d.firewall.Name, ":", err, response)
		return err
	}

	for _, action := range d.policyActions {
		for _, unsubscribe := range action.ingress {
			//	unsubscribe
			unsubscribe()
		}
	}

	return nil
}
