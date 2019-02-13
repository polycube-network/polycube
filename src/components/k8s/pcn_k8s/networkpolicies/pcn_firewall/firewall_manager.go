package pcnfirewall

import (
	"sync"

	k8sfirewall "github.com/SunSince90/polycube/src/components/k8s/utils/k8sfirewall"
	k8s_types "k8s.io/apimachinery/pkg/types"
)

type PcnFirewallManager interface {
	GetOrCreate(k8s_types.UID) PcnFirewall
	Get(k8s_types.UID) PcnFirewall
}

type FirewallManager struct {
	firewalls map[k8s_types.UID]PcnFirewall
	fwAPI     *k8sfirewall.FirewallApiService
	sync.Mutex
}

func StartFirewallManager(basePath string) PcnFirewallManager {

	fwManager := &FirewallManager{
		firewalls: map[k8s_types.UID]PcnFirewall{},
	}

	cfgK8firewall := k8sfirewall.Configuration{BasePath: basePath}
	srK8firewall := k8sfirewall.NewAPIClient(&cfgK8firewall)
	fwManager.fwAPI = srK8firewall.FirewallApi

	return fwManager
}

func (f *FirewallManager) GetOrCreate(name k8s_types.UID) PcnFirewall {

	//	Exists?
	if fw := f.Get(name); fw != nil {
		return fw
	}

	//	Create it
	fw := newFirewall(string(name), f.fwAPI)

	f.Lock()
	f.firewalls[name] = fw
	f.Unlock()

	return fw
}

func (f *FirewallManager) Get(name k8s_types.UID) PcnFirewall {

	if fw, exists := f.firewalls[name]; exists {
		return fw
	}

	return nil
}
