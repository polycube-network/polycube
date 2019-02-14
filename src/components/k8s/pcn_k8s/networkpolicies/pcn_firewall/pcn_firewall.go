package pcnfirewall

import (
	"errors"
	"sync"

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
	RemoveRules(string, []k8sfirewall.ChainRule) []k8sfirewall.ChainRule
	ImplementsPolicy(string) bool
	Destroy() error
}

type DeployedFirewall struct {
	firewall     *k8sfirewall.Firewall
	rules        map[string]*rulesContainer
	ingressChain *k8sfirewall.Chain
	egressChain  *k8sfirewall.Chain

	//fwAPI *k8sfirewall.FirewallApiService
	fwAPI k8sfirewall.FirewallAPI
	//	For caching.
	//	UPDATE: this sounds like a good idea, but I need to discover why injection fails sometimes
	//	before being able to do this, otherwise IDs can be very wrong!
	/*lastIngressID int32
	lastEgressID int32*/

	sync.Mutex
}

type rulesContainer struct {
	ingress []k8sfirewall.ChainRule
	egress  []k8sfirewall.ChainRule
	sync.Mutex

	//	UPDATE: think I don't actually need these
	//	iLock sync.Mutex
	//	eLock sync.Mutex
}

type jsonFirewall struct {
}

func newFirewall(pod core_v1.Pod, API k8sfirewall.FirewallAPI) *DeployedFirewall {

	//	This method is unexported by design: *only* the firewall manager is supposed to create new firewalls,
	//	no one else should be able to do that.

	var l = log.WithFields(log.Fields{
		"by":     "PcnFirewall",
		"method": "New()",
	})
	//	The name of the firewall
	name := string("fw-" + pod.UID)
	l.Infoln("Starting a new firewall with name", name)

	//-------------------------------------
	//	Init
	//-------------------------------------

	deployedFw := DeployedFirewall{}

	//	TODO: some of these parts should be done with a config
	deployedFw.firewall = &k8sfirewall.Firewall{
		Name:  name,
		Type_: "TC",
		Ports: []k8sfirewall.Ports{},
		//	This is important because this way rules are applied asynchronously!...
		//	... but it seems that the API don't even bother to consider it :( I'm leaving it here anyways...
		//	This value is sent later with a new request
		Interactive: false,
	}

	//	TODO: now create the ports

	//	The chains are here just for storing a default action (and maybe contain stats in future)
	//	TODO: some of these parts should be done with a config
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

	//-------------------------------------
	//	Create the firewall
	//-------------------------------------

	//	Make the request
	//	NOTE: response is always 500 in case of errors. So is there really a point in checking the response code?
	//	That's why the response is not checked and this function just returns an error.
	response, err := deployedFw.fwAPI.CreateFirewallByID(nil, name, k8sfirewall.Firewall{
		Name:     name,
		Type_:    "TC",
		Loglevel: "DEBUG",
	})

	if err != nil {
		l.Errorln("Could not create firewall with name", name, ":", err, response)
		return nil
	}

	//	Set the Ports
	//response, err := deployedFw.fwAPI.CreateFirewallPortsListByID(nil)

	// Since Interactive is not sent in the request, we will do it again now
	response, err = deployedFw.fwAPI.UpdateFirewallInteractiveByID(nil, name, false)

	if err != nil {
		l.Warningln("Could not set interactive to false for firewall", name, ":", err, response, ". Applying rules may take some time!")
	}

	//	TODO: read the firewall by ID in order to get the other components?

	return &deployedFw
}

func (d *DeployedFirewall) EnforcePolicy(policyName string, ingress, egress []k8sfirewall.ChainRule) (error, error) {

	//-------------------------------------
	//	Init
	//-------------------------------------

	var l = log.WithFields(log.Fields{
		"by":     d.firewall.Name,
		"method": "InjectRules()",
	})

	l.Debugln("firewall", d.firewall.Name, "is going to enforce policy", policyName)

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

	//-------------------------------------
	//	Inject rules concurrently
	//-------------------------------------

	//	Locking here, becase unfortunately we need to build IDs in order to inject multiple rules at once.
	//	We may even not do this, but we would be forced to make an HTTP request for every single rule!
	d.Lock()

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
			d.rules[policyName].ingress = append(d.rules[policyName].ingress, injectedRules.ingress...)
			d.rules[policyName].egress = append(d.rules[policyName].egress, injectedRules.egress...)
		}
	}

	d.Unlock()

	return iError, eError
}

func (d *DeployedFirewall) CeasePolicy(policyName string) {
	var l = log.WithFields(log.Fields{
		"by":     d.firewall.Name,
		"method": "RemoveRules(" + policyName + ")",
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

	rules.Lock()
	if len(failedRules.ingress) < 1 && len(failedRules.egress) < 1 {
		//	All rules were delete successfully: we may delete the entry
		delete(d.rules, policyName)

	} else {
		//	Some rules were not delete. We can't delete the entry: we need to change it with the still active rules.
		d.rules[policyName] = &failedRules
	}
	rules.Unlock()
}

func (d *DeployedFirewall) injectRules(direction string, rules []k8sfirewall.ChainRule) ([]k8sfirewall.ChainRule, error) {

	var l = log.WithFields(log.Fields{
		"by":     d.firewall.Name,
		"method": "injectChain()",
	})

	var ID int32 = 1

	//	Just in case...
	if rules == nil {
		l.Errorln("Rules is nil")
		return nil, errors.New("Rules is nil")
	}

	if len(rules) < 1 {
		l.Errorln("No rules to inject")
		return nil, errors.New("No rules to inject")
	}

	//-------------------------------------
	//	Build the IDs
	//-------------------------------------

	//	In order to be able to inject multiple IDs at once, we need to create the IDs, and to do that we need to check
	//	the last one.
	chain, response, err := d.fwAPI.ReadFirewallChainByID(nil, d.firewall.Name, direction)
	if err != nil {
		l.Errorln("Error while trying to get chain for firewall", d.firewall.Name, "in", direction, ":", err, response)
		return nil, err
	}

	if len(chain.Rule) > 0 {
		//	TODO: are rules always sorted by ID? check this.
		ID = chain.Rule[len(chain.Rule)-1].Id + 1
	}
	for i := 0; i < len(rules); i++ {
		//rules[i].Id = ID + i
		rules[i].Id = ID
		ID++
	}

	//-------------------------------------
	//	Actually Inject
	//-------------------------------------

	//	TODO: UPDATE firewall chain rule list?
	response, err = d.fwAPI.CreateFirewallChainRuleListByID(nil, d.firewall.Name, direction, rules)
	if err != nil {
		l.Errorln("Error while trying to inject rules for firewall", d.firewall.Name, "in", direction, ":", err, response)
		return []k8sfirewall.ChainRule{}, err
	}

	return rules, nil
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

	//	Hopefully this is empty
	return failedRules
}

func (d *DeployedFirewall) ImplementsPolicy(name string) bool {
	_, exists := d.rules[name]
	return exists
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
