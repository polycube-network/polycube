package networkpolicies

import (

	//	TODO-ON-MERGE: change these two to the polycube path

	pcn_controllers "github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/controllers"
	pcn_firewall "github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/networkpolicies/pcn_firewall"
	pcn_types "github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/types"
	k8sfirewall "github.com/SunSince90/polycube/src/components/k8s/utils/k8sfirewall"
	networking_v1 "k8s.io/api/networking/v1"

	log "github.com/sirupsen/logrus"
)

type PcnNetworkPolicyManager interface {
}

type NetworkPolicyManager struct {
	dnpc            *pcn_controllers.DefaultNetworkPolicyController
	podController   pcn_controllers.PodController
	firewallManager pcn_firewall.Manager

	defaultPolicyParser PcnDefaultPolicyParser
}

type ParsedPolicy struct {
	ingressChain k8sfirewall.Chain
	egressChain  k8sfirewall.Chain
}

func StartNetworkPolicyManager(dnpc *pcn_controllers.DefaultNetworkPolicyController, podController pcn_controllers.PodController, firewallManager pcn_firewall.Manager) PcnNetworkPolicyManager {
	var l = log.WithFields(log.Fields{
		"by":     PM,
		"method": "StartNetworkPolicyManager()",
	})
	l.Infoln("Starting Network Policy Manager")

	manager := NetworkPolicyManager{
		dnpc:            dnpc,
		podController:   podController,
		firewallManager: firewallManager,
	}

	manager.defaultPolicyParser = newDefaultPolicyParser(podController, firewallManager, &manager)

	//	Subscribe

	dnpc.Subscribe(pcn_types.New, func(policy *networking_v1.NetworkPolicy) {
		manager.defaultPolicyParser.Parse(policy, true)
	})

	dnpc.Subscribe(pcn_types.Delete, func(policy *networking_v1.NetworkPolicy) {
		podsAffected, err := manager.defaultPolicyParser.GetPodsAffected(policy)

		if err != nil {
			//	err
			return
		}

		for _, pods := range podsAffected {
			for _, pod := range pods {
				fw := firewallManager.Get(pod.Pod)

				if fw != nil {
					fw.CeasePolicy(policy.Name)

					//	Remove this pod from the deployed policies
				}
			}
		}
	})
	/*dnpc.Subscribe(pcn_types.Update, func(policy *networking_v1){
		//	delete
		//	Deploy
	})*/

	return &manager
}
