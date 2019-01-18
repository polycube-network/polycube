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
	k8sfirewall "github.com/SunSince90/polycube/src/components/k8s/utils/k8sfirewall"
	log "github.com/sirupsen/logrus"
)

type FirewallAPI struct {
	logBy string

	fwAPI *k8sfirewall.FirewallApiService
}

func NewFirewallApi(basePath string) *FirewallAPI {
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

	//	First create the firewall
	if err = f.createFirewall(generatedName); err == nil {
		return "", nil
	}

	//	Then create the ports
	if err = f.createPorts(generatedName, firewall.Ports); err == nil {

		//	delete the firewall here
		return "", nil
	}

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
	}

	l.Debugf("Successfully deleted firewall with name %s.", name)

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
	}

	l.Infof("Successfully created firewall with name %s", name)

	return err
}

func (f *FirewallAPI) createPorts(name string, ports []k8sfirewall.Ports) error {

	//	As above, there is no point in checking the response...
	_, err := f.fwAPI.CreateFirewallPortsListByID(nil, name, ports)

	return err
}
