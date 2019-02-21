package pcnfirewall

import (
	"sync"

	k8sfirewall "github.com/SunSince90/polycube/src/components/k8s/utils/k8sfirewall"
	core_v1 "k8s.io/api/core/v1"
)

type Manager interface {
	GetOrCreate(core_v1.Pod) PcnFirewall
	Get(core_v1.Pod) PcnFirewall
	GetAll() []PcnFirewall
}

type FirewallManager struct {
	//firewalls map[k8s_types.UID]PcnFirewall
	firewalls map[string]PcnFirewall
	//fwAPI     *k8sfirewall.FirewallApiService
	fwAPI k8sfirewall.FirewallAPI
	lock  sync.Mutex
}

func StartFirewallManager(basePath string) Manager {

	fwManager := &FirewallManager{
		//firewalls: map[k8s_types.UID]PcnFirewall{},
		firewalls: map[string]PcnFirewall{},
	}

	cfgK8firewall := k8sfirewall.Configuration{BasePath: basePath}
	srK8firewall := k8sfirewall.NewAPIClient(&cfgK8firewall)
	fwManager.fwAPI = srK8firewall.FirewallApi

	return fwManager
}

func (f *FirewallManager) GetOrCreate(pod core_v1.Pod) PcnFirewall {

	//	Firewall name
	//ID := k8s_types.UID("fw-" + pod.UID)
	ID := "fw-" + pod.Status.PodIP

	//	Exists?
	if fw := f.Get(pod); fw != nil {
		return fw
	}

	//	Create it
	fw := newFirewall(pod, f.fwAPI)

	f.lock.Lock()
	f.firewalls[ID] = fw
	f.lock.Unlock()

	return fw
}

func (f *FirewallManager) Get(pod core_v1.Pod) PcnFirewall {

	//	Firewall name
	//ID := k8s_types.UID("fw-" + pod.UID)
	ID := "fw-" + pod.Status.PodIP

	if fw, exists := f.firewalls[ID]; exists {
		return fw
	}

	return nil
}

func (f *FirewallManager) GetAll() []PcnFirewall {
	fws := []PcnFirewall{}

	f.lock.Lock()
	for _, fw := range f.firewalls {
		fws = append(fws, fw)
	}
	f.lock.Unlock()

	return fws
}
