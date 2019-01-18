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

	//	First create the firewall
	if err := f.createFirewall("c"); err == nil {
		return "", nil
	}

	//	Then create the ports

	return generatedName, nil
}

func (f *FirewallAPI) createFirewall(name string) error {
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

	return err
}
