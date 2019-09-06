package pcnfirewall

import (
	k8sfirewall "github.com/polycube-network/polycube/src/components/k8s/utils/k8sfirewall"
	log "github.com/sirupsen/logrus"
)

var (
	fwAPI  *k8sfirewall.FirewallApiService
	logger *log.Logger
)

func init() {
	logger = log.New()

	// Only log the debug severity or above.
	logger.SetLevel(log.DebugLevel)

	// All logs will specify who the function that logged it
	logger.SetReportCaller(true)
}

// SetFwAPI sets the firewall API on the package
func SetFwAPI(basePath string) {
	cfgK8firewall := k8sfirewall.Configuration{BasePath: basePath}
	srK8firewall := k8sfirewall.NewAPIClient(&cfgK8firewall)
	fwAPI = srK8firewall.FirewallApi
}
