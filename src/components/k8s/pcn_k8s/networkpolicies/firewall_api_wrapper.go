/*
	This file exist for the sole purpose of providing a way to communicate with the firewall APIs.
	After a lot of tests, I found that 500 code is returned if the firewall is created with just one request.
	You need to do a request for each component you need to add on the firewall: the firewall itself, the ports,
	the rules... you CAN'T do that it in a single request and I don't know why.
	Also, no matter what the error is, they always return 500: you have no way to know if the firewall does not exist or the
	operation was not permitted.
	That's why I created this file: using the APIs that are tested to work without exposing those that don't, de facto operating as a middleware.
	There's one big issue here: since there is no way to use just one query to bulk insert rules, you have to make as much request as rules
	you need to insert!!
	So, until a solution is found, this is the way the firewall APIs are going to be used.
*/
package networkpolicies

import (
	"sync"

	k8sfirewall "github.com/SunSince90/polycube/src/components/k8s/utils/k8sfirewall"
	log "github.com/sirupsen/logrus"
)

type FirewallAPI struct {
	logBy string

	fwAPI *k8sfirewall.FirewallApiService
}

func NewFirewallAPI(basePath string) *FirewallAPI {
	var fwAPI *k8sfirewall.FirewallApiService

	cfgK8firewall := k8sfirewall.Configuration{BasePath: basePath}
	srK8firewall := k8sfirewall.NewAPIClient(&cfgK8firewall)
	fwAPI = srK8firewall.FirewallApi

	return &FirewallAPI{
		fwAPI: fwAPI,
	}
}

func (f *FirewallAPI) Create(firewall k8sfirewall.Firewall) (string, error) {

	//	TODO: the name must be sha-1-ed or uuid.
	// Generate it here ....
	generatedName := firewall.Name
	var err error
	var chainWaitGroup sync.WaitGroup
	var chainsWaitLen int

	//	First create the firewall
	if err = f.createFirewall(generatedName); err != nil {
		return "", err
	}

	//	Then create the ports
	if err = f.createPorts(generatedName, firewall.Ports); err != nil {

		f.Destroy(generatedName)

		//	delete the firewall here
		return "", err
	}

	//	No chains?
	//	TODO: Check if this needs another function
	if len(firewall.Chain) < 1 {
		return generatedName, nil
	}

	//	Now for the harder part.
	//	(usually there are just two)
	chainsWaitLen = len(firewall.Chain)
	chainWaitGroup.Add(chainsWaitLen)

	for _, chain := range firewall.Chain {

		if chain.Name == "ingress" || chain.Name == "egress" {

			go func(currentChain k8sfirewall.Chain) {
				defer chainWaitGroup.Done()
				f.BulkAddRules(firewall.Name, chain.Name, chain.Rule)
				f.ApplyRules(firewall.Name, chain.Name)
			}(chain)

		}
	}

	//	Wait for ingress and egress to finish
	chainWaitGroup.Wait()

	return generatedName, nil
}

func (f *FirewallAPI) Destroy(name string) {

	var l = log.WithFields(log.Fields{
		"by":     f.logBy,
		"method": "Destroy()",
	})

	//	If the firewall does not exist, 500 is returned, not 404. So... why even bother to check the result?
	//	If you did it, ok! If not... well just log it, man....
	response, err := f.fwAPI.DeleteFirewallByID(nil, name)

	if err != nil {
		l.Errorf("Could not delete firewall with name %s! The error returned was %d.", name, response.StatusCode)
		return
	}

	l.Debugf("Successfully deleted firewall with name %s.", name)
}

func (f *FirewallAPI) BulkAddRules(name string, type_ string, rules []k8sfirewall.ChainRule) {

	var l = log.WithFields(log.Fields{
		"by":     f.logBy,
		"method": "BulkAddRules()",
	})

	var waitRules sync.WaitGroup
	waitRules.Add(len(rules))

	for _, rule := range rules {

		go func(currentRule k8sfirewall.ChainRule) {
			defer waitRules.Done()
			f.AddRule(name, type_, currentRule)
		}(rule)

	}

	//	Wait for them to finish
	waitRules.Wait()

	l.Debugf("Finished waiting for bulkAddRules(%s,%s) to finish", name, type_)
}

func (f *FirewallAPI) AddRule(name string, type_ string, rule k8sfirewall.ChainRule) {

	var l = log.WithFields(log.Fields{
		"by":     f.logBy,
		"method": "AddRule()",
	})

	//	Convert it to a Chain Append struct (nothing changes...)
	ruleToAppend := k8sfirewall.ChainAppendInput{
		Src:         rule.Src,
		Dst:         rule.Dst,
		L4proto:     rule.L4proto,
		Action:      rule.Action,
		Sport:       rule.Sport,
		Dport:       rule.Dport,
		Tcpflags:    rule.Tcpflags,
		Conntrack:   rule.Conntrack,
		Description: rule.Description,
	}

	//	Make the request now
	output, response, err := f.fwAPI.CreateFirewallChainAppendByID(nil, name, type_, ruleToAppend)

	if err != nil {
		l.Errorf("could not add rule for %s, %d", type_, response.StatusCode)
		return
	}

	l.Debugf("added rule for %s, output id is %d", type_, output.Id)

	//	TODO: does it make sense to return the rule id? Check this up
}

func (f *FirewallAPI) ApplyRules(name string, type_ string) bool {

	var l = log.WithFields(log.Fields{
		"by":     f.logBy,
		"method": "ApplyRules()",
	})

	output, response, err := f.fwAPI.CreateFirewallChainApplyRulesByID(nil, "newone", "ingress")

	if err != nil || !output.Result {
		l.Errorf("Could not apply rules for %s, in %s! %d", name, type_, response.StatusCode)
		return false
	}

	l.Debugf("Applied rules for %s, in %s", name, type_)
	return true
}

func (f *FirewallAPI) createFirewall(name string) error {

	var l = log.WithFields(log.Fields{
		"by":     f.logBy,
		"method": "createFirewall()",
	})

	//	Make the request
	//	NOTE: as I wrote above, response is always 500 in case of errors. So is there really a point in checking the response code?
	//	That's why the response is not checked and this function just returns an error
	_, err := f.fwAPI.CreateFirewallByID(nil, name, k8sfirewall.Firewall{
		Name:     name,
		Type_:    "TC",
		Loglevel: "DEBUG",

		//	This is important because this way rules are applied asynchronously!...
		//	... but it seems that the API don't even bother to consider it :( I'm leaving it here anyways...
		Interactive: false,
	})

	if err != nil {
		l.Errorf("Could not create firewall with name %s: %s", name, err)
		return err
	}

	l.Debugf("Successfully created firewall with name %s", name)
	return nil
}

func (f *FirewallAPI) createPorts(name string, ports []k8sfirewall.Ports) error {

	var l = log.WithFields(log.Fields{
		"by":     f.logBy,
		"method": "createPorts()",
	})

	//	As above, there is no point in checking the response...
	_, err := f.fwAPI.CreateFirewallPortsListByID(nil, name, ports)

	if err != nil {
		l.Errorf("Could not add ports for firewall %s", name)
		return err
	}

	l.Debugf("Created ports for firewall %s", name)
	return nil
}
