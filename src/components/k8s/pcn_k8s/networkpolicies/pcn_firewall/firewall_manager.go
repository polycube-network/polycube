package pcnfirewall

import (
	"sync"

	//	TODO-ON-MERGE: change these to the polycube path
	pcn_controllers "github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/controllers"
	pcn_types "github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/types"
	k8sfirewall "github.com/SunSince90/polycube/src/components/k8s/utils/k8sfirewall"

	log "github.com/sirupsen/logrus"
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
	podController pcn_controllers.PodController
	node          string
	fwAPI         k8sfirewall.FirewallAPI
	lock          sync.Mutex
}

func StartFirewallManager(basePath, nodeName string, podController pcn_controllers.PodController) Manager {

	fwManager := &FirewallManager{
		//firewalls: map[k8s_types.UID]PcnFirewall{},
		firewalls:     map[string]PcnFirewall{},
		node:          nodeName,
		podController: podController,
	}

	cfgK8firewall := k8sfirewall.Configuration{BasePath: basePath}
	srK8firewall := k8sfirewall.NewAPIClient(&cfgK8firewall)
	fwManager.fwAPI = srK8firewall.FirewallApi

	//	Subscribe to events
	podController.Subscribe(pcn_types.Event{Type: pcn_types.Update}, fwManager.manageFirewallForPod)

	return fwManager
}

func (f *FirewallManager) manageFirewallForPod(pod *core_v1.Pod) {
	var l = log.WithFields(log.Fields{
		"by":     "FirewallManager",
		"method": "manageFirewallForPod(" + pod.Name + ")",
	})

	if pod == nil {
		l.Errorln("Called with a nil pod!")
		return
	}

	//	Is this pod from the kube-system?
	if pod.Namespace == "kube-system" {
		//l.Infoln("Pod", pod.Name, "belongs to the kube-system namespaces: no point in getting its firewall. Will stop here.")
		return
	}

	//	Is this pod running in my same node?
	if pod.Spec.NodeName != f.node {
		//l.Infoln("Pod", pod.Name, "is not running in my node: no point in getting its firewall. Will stop here.")
		return
	}

	//	Is it running? (if not, it wouldn't have a valid ip yet)
	if pod.Status.Phase != core_v1.PodRunning {
		//l.Infoln("Pod", pod.Name, "is in", pod.Status.Phase, "so I'm not going to check it.")
		return
	}

	f.lock.Lock()
	defer f.lock.Unlock()

	ID := "fw-" + pod.Status.PodIP

	//	A hack way to determine if the pod is terminating. The APIs don't have a way to do this :(
	if pod.ObjectMeta.DeletionTimestamp != nil {
		//l.Debugln("Pod", pod.Name, "is terminating. Will remove it.")

		if fw, ok := f.firewalls[ID]; ok {
			fw.Destroy()
			delete(f.firewalls, ID)
		}
		return
	}

	//	Already there?
	if _, ok := f.firewalls[ID]; ok {
		//l.Debugln("Already have the firewall for pod", pod.Name, "in my list.")
		return
	}

	//	Ok add this.
	fw := newFirewall(*pod, f.fwAPI)
	if fw == nil {
		l.Errorln("Could not get the firewall for pod", pod.Name)
		return
	}

	f.firewalls[ID] = fw
}

func (f *FirewallManager) GetOrCreate(pod core_v1.Pod) PcnFirewall {

	//	Firewall name
	//ID := k8s_types.UID("fw-" + pod.UID)
	ID := "fw-" + pod.Status.PodIP

	f.lock.Lock()
	defer f.lock.Unlock()

	//	Exists?
	if fw := f.Get(pod); fw != nil {
		return fw
	}

	//	Create it
	fw := newFirewall(pod, f.fwAPI)
	if fw == nil {
		return nil
	}

	f.firewalls[ID] = fw

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
