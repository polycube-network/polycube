package pcnfirewall

import (
	"errors"
	"sync"

	k8s_types "k8s.io/apimachinery/pkg/types"

	pcn_types "github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/types"
	k8sfirewall "github.com/SunSince90/polycube/src/components/k8s/utils/k8sfirewall"
	log "github.com/sirupsen/logrus"
	core_v1 "k8s.io/api/core/v1"
)

type PcnFirewall interface {
	EnforcePolicy(string, []k8sfirewall.ChainRule, []k8sfirewall.ChainRule) (error, error)
	CeasePolicy(string)
	//	UPDATE: it's better to always inject with a policy instead of doing it anonymously.
	//	That's why it is now unexported
	//InjectRules(string, []k8sfirewall.ChainRule) ([]k8sfirewall.ChainRule, error)
	ForPod() k8s_types.UID
	RemoveRules(string, []k8sfirewall.ChainRule) []k8sfirewall.ChainRule
	RemoveIPReferences(string)
	ImplementsPolicy(string) bool
	Destroy() error
}

type DeployedFirewall struct {
	firewall *k8sfirewall.Firewall
	rules    map[string]*rulesContainer

	//	TODO: Change with the following:
	/*
		ingressRules map[string]*rulesContainer
		egressRules map[string]*rulesContainer
	*/
	ingressChain *k8sfirewall.Chain
	egressChain  *k8sfirewall.Chain
	fwAPI        k8sfirewall.FirewallAPI
	podUID       k8s_types.UID
	podIP        string
	//	For caching.
	/*lastIngressID int32
	lastEgressID int32*/
	lock sync.Mutex
}

type rulesContainer struct {
	ingress []k8sfirewall.ChainRule
	egress  []k8sfirewall.ChainRule
	//	UPDATE: think I don't actually need these
	//	sync.Mutex
	//	iLock sync.Mutex
	//	eLock sync.Mutex
}

func newFirewall(pod core_v1.Pod, API k8sfirewall.FirewallAPI) *DeployedFirewall {

	//	This method is unexported by design: *only* the firewall manager is supposed to get new firewalls,
	//	no one else should be able to do that.

	var l = log.WithFields(log.Fields{
		"by":     "PcnFirewall",
		"method": "New()",
	})

	//	The name of the firewall
	name := "fw-" + pod.Status.PodIP
	//l.Infoln("Starting a new firewall with name", name)

	//-------------------------------------
	//	Init
	//-------------------------------------

	deployedFw := DeployedFirewall{}

	//	The chains are here just for storing a default action (and maybe contain stats in future)
	deployedFw.ingressChain = &k8sfirewall.Chain{
		Name:     "ingress",
		Default_: "drop",
	}
	deployedFw.egressChain = &k8sfirewall.Chain{
		Name:     "egress",
		Default_: "drop",
	}

	//	Rules
	deployedFw.rules = map[string]*rulesContainer{}

	//	The firewall API
	deployedFw.fwAPI = API

	//	The pod this firewall is intended for
	deployedFw.podUID = pod.UID

	//	The pod's ip
	deployedFw.podIP = pod.Status.PodIP

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

	//log.Debugf("### got the firewall, is: %+v\n", fw)

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

func (d *DeployedFirewall) EnforcePolicy(policyName string, ingress, egress []k8sfirewall.ChainRule) (error, error) {

	//-------------------------------------
	//	Init
	//-------------------------------------

	var l = log.WithFields(log.Fields{
		"by":     d.firewall.Name,
		"method": "InjectRules()",
	})

	//l.Debugln("firewall", d.firewall.Name, "is going to enforce policy", policyName)

	//	Ingress and egress can be empty (e.g.: when having default rules), but cannot be nil
	if ingress == nil && egress == nil {
		err := errors.New("Both ingress and egress are nil")
		l.Errorln(err.Error())
		return err, err
	}

	var iError error
	var eError error
	var applyWait sync.WaitGroup
	waitChains := 0

	//	Default values
	//	NOTE: these may remain empty, example: when blocking/allowing everything regardless of port and protocol.
	//	But they also remain empty if an error occurred while deploy rules. While this may seem like an
	//	incorrect behaviour, it is actually correct: for kubernetes this policy is actually *deployed*,
	//	it doesn't care if errors occurred while injecting rules.
	injectedRules := rulesContainer{
		ingress: []k8sfirewall.ChainRule{},
		egress:  []k8sfirewall.ChainRule{},
	}

	if ingress != nil && len(ingress) > 0 {
		waitChains++
	}
	if egress != nil && len(egress) > 0 {
		waitChains++
	}
	applyWait.Add(waitChains)
	//l.Debugln("firewall", d.firewall.Name, "finished waiting chains", policyName)
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
	if ingress != nil && len(ingress) > 0 {
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
	if egress != nil && len(egress) > 0 {
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
	//l.Debugln("firewall", d.firewall.Name, "finished waiting applywait", policyName)
	//-------------------------------------
	//	Update rules struct
	//-------------------------------------

	//	If at least something succeded, then we can specify that this firewall implements this policy
	if iError == nil || eError == nil {
		//	Add the newly created rules on our struct, so we can reference them at all times
		if _, exists := d.rules[policyName]; !exists {
			d.rules[policyName] = &injectedRules
		} else {
			//	This piece of code only executes if this policy is being updated and couldn't delete rules

			/*
				TODO: to be changed with the following
				d.ingressRules[policyName] = append(d.ingressRules[policyName], injectedRules.ingress...)
				d.egressRules[policyName] = append(d.egressRules[policyName], injectedRules.egress...)
			*/
			d.rules[policyName].ingress = append(d.rules[policyName].ingress, injectedRules.ingress...)
			d.rules[policyName].egress = append(d.rules[policyName].egress, injectedRules.egress...)
		}
	}

	/*_fw, response, err := d.fwAPI.ReadFirewallByID(nil, d.firewall.Name)

	if err != nil {
		l.Errorln("Could not get firewall with name after enforce", d.firewall.Name, ":", err, response)

		if response.StatusCode != 200 {
			l.Errorln("The firewall is nil. Will stop now. after enforce")
			return nil, nil
		}
	}

	l.Debugf("### after enforcing policy, fw is %+v\n", _fw)*/

	//	If these are the first rules, it means that a policy exists for this pod.
	//	Which also means that now we have to change the default action
	if len(d.rules) == 1 {
		d.updateDefaultActions(pcn_types.ActionDrop)
	}

	return iError, eError
}

func (d *DeployedFirewall) CeasePolicy(policyName string) {
	var l = log.WithFields(log.Fields{
		"by":     d.firewall.Name,
		"method": "CeasePolicy(" + policyName + ")",
	})

	var deleteWait sync.WaitGroup
	deleteNumber := 0
	//	Read RemoveRules to know why this exists
	failedRules := rulesContainer{
		ingress: []k8sfirewall.ChainRule{},
		egress:  []k8sfirewall.ChainRule{},
	}

	if !d.ImplementsPolicy(policyName) {
		l.Infoln("fw", d.firewall.Name, "has no", policyName, "in its list of implemented policies rules")
		return
	}

	rules := d.rules[policyName]
	if len(rules.ingress) < 1 && len(rules.egress) < 1 {
		l.Infoln("fw", d.firewall.Name, "has no ingress nor egress rules for policy", policyName)
		d.lock.Lock()
		defer d.lock.Unlock()

		delete(d.rules, policyName)
		if len(d.rules) < 1 {
			d.updateDefaultActions(pcn_types.ActionForward)
		}
		return
	}

	if len(rules.ingress) > 0 {
		deleteNumber++
	}
	if len(rules.egress) > 0 {
		deleteNumber++
	}
	deleteWait.Add(deleteNumber)

	//-------------------------------------
	//	Remove the rules
	//-------------------------------------

	d.lock.Lock()
	defer d.lock.Unlock()

	if !d.isFirewallOk() {
		l.Errorln("Firewall seems not to be ok! Will not remove rules.")
		return
	}

	//	Ingress
	if len(rules.ingress) > 0 {
		go func(rules []k8sfirewall.ChainRule) {
			defer deleteWait.Done()

			iFailedRules := d.RemoveRules("ingress", rules)
			if len(iFailedRules) > 0 {
				failedRules.ingress = iFailedRules
			}
		}(rules.ingress)
	}

	//	Egress
	if len(rules.egress) > 0 {
		go func(rules []k8sfirewall.ChainRule) {
			defer deleteWait.Done()

			eFailedRules := d.RemoveRules("egress", rules)
			if len(eFailedRules) > 0 {
				failedRules.egress = eFailedRules
			}
		}(rules.egress)
	}

	deleteWait.Wait()

	//-------------------------------------
	//	Update the implemented policies
	//-------------------------------------

	if len(failedRules.ingress) < 1 && len(failedRules.egress) < 1 {
		//	All rules were delete successfully: we may delete the entry
		delete(d.rules, policyName)

	} else {
		//	Some rules were not deleted. We can't delete the entry: we need to change it with the still active rules.
		d.rules[policyName] = &failedRules
	}

	//	If this pod doesn't enfoce any policy, we must change the default action to forward.
	if len(d.rules) < 1 {
		d.updateDefaultActions(pcn_types.ActionForward)
	}
}

func (d *DeployedFirewall) updateDefaultActions(to string) (error, error) {
	var l = log.WithFields(log.Fields{
		"by":     d.firewall.Name,
		"method": "updateDefaultAction(" + to + ")",
	})
	//l.Debugln("###Going to set", to, "default")
	var iErr error
	var eErr error
	if response, err := d.fwAPI.UpdateFirewallChainDefaultByID(nil, d.firewall.Name, "ingress", to); err != nil {
		l.Errorln("Could not set default ingress action to forward for firewall", d.firewall.Name, ":", err, response)
		iErr = err
	}

	//	Apply the changes in ingress
	out, response, err := d.fwAPI.CreateFirewallChainApplyRulesByID(nil, d.firewall.Name, "ingress")
	if err != nil {
		l.Errorln("Error while trying to apply rules", d.firewall.Name, "in", "ingress", ":", err, response)
	} else {
		l.Debugln("applying changes to default action in ingress: result", out)
	}
	//l.Debugln("Successfully set default ingress action to forward for firewall", d.firewall.Name)

	if response, err := d.fwAPI.UpdateFirewallChainDefaultByID(nil, d.firewall.Name, "egress", to); err != nil {
		l.Errorln("Could not set default egress action to forward for firewall", d.firewall.Name, ":", err, response)
		eErr = err
	}

	//	Apply the changes in egress
	out, response, err = d.fwAPI.CreateFirewallChainApplyRulesByID(nil, d.firewall.Name, "egress")
	if err != nil {
		l.Errorln("Error while trying to apply rules", d.firewall.Name, "in", "egress", ":", err, response)
	} else {
		l.Debugln("applying changes to default action in egress: result", out)
	}
	//l.Debugln("Successfully set default egress action to forward for firewall", d.firewall.Name)

	return iErr, eErr
}

func (d *DeployedFirewall) injectRules(direction string, rules []k8sfirewall.ChainRule) ([]k8sfirewall.ChainRule, error) {

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
	//	Build the IDs
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
				workedRule := rule
				workedRule.Dst = d.podIP
			}
		}
		if direction == "egress" {
			//	Make sure not to target myself
			if rule.Dst != d.podIP {
				workedRule := rule
				workedRule.Src = d.podIP
			}
		}

		workedRule.Id = ID
		ID++
		rulesToInject = append(rulesToInject, workedRule)
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

	out, response, err := d.fwAPI.CreateFirewallChainApplyRulesByID(nil, d.firewall.Name, direction)
	if err != nil {
		l.Errorln("Error while trying to apply rules", d.firewall.Name, "in", direction, ":", err, response)
	} else {
		l.Debugln("applying injection result", out)
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

	//	After some thorough testing, I saw that sometimes rule deletion fails and I don't know why.
	//	So I am forced to do this like this:
	//	1) delete the rule
	//	2) if successful, do nothing
	//	3) if not, add this rule to the failed ones
	failedRules := []k8sfirewall.ChainRule{}
	var waitDelete sync.WaitGroup
	var mutex sync.Mutex

	waitDelete.Add(len(rules))

	for _, rule := range rules {
		go func(currentRule k8sfirewall.ChainRule) {
			defer waitDelete.Done()

			response, err := d.fwAPI.DeleteFirewallChainRuleByID(nil, d.firewall.Name, direction, currentRule.Id)
			if err != nil {
				l.Errorln("Error while trying to delete rule", currentRule.Id, "in", direction, "for firewall", d.firewall.Name, err, response)

				mutex.Lock()
				failedRules = append(failedRules, currentRule)
				mutex.Unlock()
			}
		}(rule)
	}

	waitDelete.Wait()

	out, response, err := d.fwAPI.CreateFirewallChainApplyRulesByID(nil, d.firewall.Name, direction)
	if err != nil {
		l.Errorln("Error while trying to apply rules deletion", d.firewall.Name, "in", direction, ":", err, response)
	} else {
		l.Debugln("applying deletion result:", out)
	}

	//	Hopefully this is empty
	return failedRules
}

func (d *DeployedFirewall) RemoveIPReferences(ip string) {

	log.Debugln("###remove-ip, on fw-", d.podIP)
	ingressIDs := []k8sfirewall.ChainRule{}
	egressIDs := []k8sfirewall.ChainRule{}

	d.lock.Lock()
	defer d.lock.Unlock()

	for _, policy := range d.rules {
		var directionsWait sync.WaitGroup
		directionsWait.Add(2)

		go func(ingress []k8sfirewall.ChainRule) {
			defer directionsWait.Done()
			for _, i := range ingress {
				log.Debugln("###delete-ingress, checking", i.Src, "with", ip)
				if i.Src == ip {
					log.Debugln("###delete-ingress,match!", i.Src, "with", ip)
					ingressIDs = append(ingressIDs, i)
				}
			}
		}(policy.ingress)

		go func(egress []k8sfirewall.ChainRule) {
			defer directionsWait.Done()
			for _, i := range egress {
				log.Debugln("###delete-egress, checking", i.Dst, "with", ip)
				if i.Dst == ip {
					log.Debugln("###delete-egress, match!", i.Dst, "with", ip)
					egressIDs = append(egressIDs, i)
				}
			}
		}(policy.egress)
		directionsWait.Wait()
	}

	if len(ingressIDs) < 1 && len(egressIDs) < 1 {
		return
	}
	/*var removeWait sync.WaitGroup
	removeWait.Add(2)*/

	log.Debugf("###ingress-rules-to-remove %+v\n", ingressIDs)
	log.Debugf("###egress-rules-to-remove %+v\n", egressIDs)

	if len(ingressIDs) > 0 {
		d.RemoveRules("ingress", ingressIDs)
	}
	if len(egressIDs) > 0 {
		d.RemoveRules("egress", egressIDs)
	}
	/*go func() {
	defer removeWait.Done()
	if len(ingressIDs) < 1 {
		return
	}
	d.RemoveRules("ingress", ingressIDs)
	}()
	go func() {
		defer removeWait.Done()
	if len(egressIDs) < 1 {
		return
	}
	d.RemoveRules("egress", egressIDs)
	}()
	removeWait.Wait()*/
}

//	TODO: to be changed (read inside)
func (d *DeployedFirewall) ImplementsPolicy(name string) bool {

	d.lock.Lock()
	defer d.lock.Unlock()

	_, exists := d.rules[name]
	return exists

	/*	TODO: change with the following
		ImplementsPolicy(name, direction string) (direction: ingress, egress, any)
		=>
		d.lock.Lock()
		defer d.lock.Unlock()

		if direction == "ingress" || direction == "any" {
			_, exists := d.ingressRules[name]
			return exists
		}

		_, exists := d.egressRules[name]
		return exists
	*/
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

func (d *DeployedFirewall) Destroy() error {
	var l = log.WithFields(log.Fields{
		"by":     d.firewall.Name,
		"method": "Destroy()",
	})

	if response, err := d.fwAPI.DeleteFirewallByID(nil, d.firewall.Name); err != nil {
		l.Errorln("Failed to destroy firewall,", d.firewall.Name, ":", err, response)
		return err
	}

	return nil
}
