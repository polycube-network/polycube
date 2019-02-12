package networkpolicies

import (
	"errors"
	"strings"
	"sync"

	//	TODO-ON-MERGE: change these to the polycube path
	"github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/controllers"
	pcn_types "github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/types"
	k8sfirewall "github.com/SunSince90/polycube/src/components/k8s/utils/k8sfirewall"

	/*events "github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/types/events"
	podquery "github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/types/podquery"
	"github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/types/polycube_pod"*/

	log "github.com/sirupsen/logrus"
	core_v1 "k8s.io/api/core/v1"
	networking_v1 "k8s.io/api/networking/v1"
	meta_v1 "k8s.io/apimachinery/pkg/apis/meta/v1"
	k8s_types "k8s.io/apimachinery/pkg/types"
)

type NetworkPolicyManager struct {
	logBy string

	dnpc *controllers.DefaultNetworkPolicyController

	podController controllers.PodController

	fwAPI *FirewallAPI

	deployedFirewalls map[k8s_types.UID]*deployedFirewall
	deployedPolicies  map[string][]k8s_types.UID

	deployedFirewallsLock sync.Mutex
}

type parsedProtoPort struct {
	protocol string
	port     int32
}

type deployedFirewall struct {
	firewall     *k8sfirewall.Firewall
	rules        map[string]*policyRules
	ingressChain *k8sfirewall.Chain
	egressChain  *k8sfirewall.Chain

	//	Experimental
	sync.Mutex
}

type policyRules struct {
	ingress []k8sfirewall.ChainRule
	egress  []k8sfirewall.ChainRule
	sync.Mutex

	//	UPDATE: think I don't actually need these
	//	iLock sync.Mutex
	//	eLock sync.Mutex
}

/*type policyRulesIDs struct {
	ingress []int32
	egress  []int32
}*/

//	TODO: this is just temporary, maybe do this with configMaps?
const (
	//	Just for test
	supportedProtocols = "TCP,UDP"
	//supportedProtocols = "TCP,UDP,ICMP"
)

func NewNetworkPolicyManager(dnpc *controllers.DefaultNetworkPolicyController, podController controllers.PodController, basePath string) *NetworkPolicyManager {

	var l = log.WithFields(log.Fields{
		"by":     "Network Policy Manager",
		"method": "ParseDefaultRules()",
	})

	l.Infoln("Network Policy Manager starting...")

	//https://play.golang.org/p/0AaBhB1MHBc

	//	Create the resource
	manager := &NetworkPolicyManager{
		logBy:             "Network Policy Manager",
		deployedFirewalls: map[k8s_types.UID]*deployedFirewall{},
		deployedPolicies:  map[string][]k8s_types.UID{},
	}

	//	Link the default policy controller
	if dnpc != nil {
		manager.dnpc = dnpc
		//	Let me listen for default network policies deployments
		dnpc.Subscribe(pcn_types.New, manager.DeployNewPolicy)
		//dnpc.Subscribe(events.Update, manager.UpdatePolicy)
		dnpc.Subscribe(pcn_types.Delete, manager.RemovePolicy)
	} else {
		l.Warningln("Warning: default policy controller is nil, functions involving pods and namespaces may cause errors")
	}

	//	Link the pod controller
	if podController != nil {
		manager.podController = podController

		//podController.Subscribe()
	} else {
		l.Warningln("Warning: pod controller is nil, functions involving pods and namespaces may cause errors")
	}

	manager.fwAPI = NewFirewallAPI(basePath)

	return manager
}

func (manager *NetworkPolicyManager) DeployNewPolicy(policy *networking_v1.NetworkPolicy) {

	var l = log.WithFields(log.Fields{
		"by":     manager.logBy,
		"method": "DeployNewPolicy()",
	})

	l.Debugln("Going to manage the default policy")

	//-------------------------------------
	//	The basics
	//-------------------------------------

	//	Get the namespace (for us, '*' means all namespaces)
	namespace := "*"
	if len(policy.ObjectMeta.Namespace) > 0 {
		namespace = policy.Namespace
	}

	nsPods, err := manager.getPodsAffectedByDefaultPolicy(&policy.Spec.PodSelector, namespace)

	if err != nil {
		l.Errorln("Error while trying to get pods affected", err)
		return
	}

	//	No pods found?
	if len(nsPods) < 1 {
		return
	}

	//	Get the spec, with the ingress & egress rules
	spec := policy.Spec
	ingress, egress := manager.GetPolicyTypes(&spec)

	//-------------------------------------
	//	Parse the rules
	//-------------------------------------

	//	Get the parsed chains
	//	TODO: goroutine for each ns as well?
	for ns, pods := range nsPods {

		//	Get the chains
		ingressChain, egressChain := manager.ParseDefaultRules(ingress, egress, ns)

		var podsWaitGroup sync.WaitGroup
		podsWaitGroup.Add(len(pods))

		for _, pod := range pods {
			go func(currentPod pcn_types.Pod) {
				//-------------------------------------
				//	Start
				//-------------------------------------

				defer podsWaitGroup.Done()

				fwUID := k8s_types.UID("fw-" + currentPod.Pod.UID)
				var fw *deployedFirewall

				//-------------------------------------
				//	Prepare the firewall on the manager struct
				//-------------------------------------

				//	NOTE: no matter if the firewall creation succeeds or fails, we create the deployedFirewall struct
				//	in order to have it ready for next policy deployments.
				//	Actual firewall creation is going to be done a little later (see below)

				manager.deployedFirewallsLock.Lock()
				fw, exists := manager.deployedFirewalls[fwUID]
				if !exists {
					fw = manager.insertNewEmptyFirewall(currentPod.Pod)
				}
				manager.deployedFirewallsLock.Unlock()

				//	Complete the rules by correctly adding the IPs
				ingressChain, egressChain := manager.fillChains(currentPod, ingressChain, egressChain)

				//	There may be two policies deploying concurrently for this pod.
				//	We need only the first one to actually create the firewall.
				//	This is why we lock here and not before injecting the rules.
				fw.Lock()

				//-------------------------------------
				//	Create the firewall
				//-------------------------------------

				//	Create the firewall first
				//	If the firewall already exists, then it's not a problem. It just means that someone did it before us
				if ok, err := manager.fwAPI.GetOrCreate(*fw.firewall); !ok {
					l.Errorln("error while trying to create firewall", err, "going to stop.")
					return
				}

				//-------------------------------------
				//	Inject the rules
				//-------------------------------------

				//	UPDATE: rules injection has been moved to a function because it is *very* useful for UpdatePolicy.
				//	Later on I decided to use a different algorithm there, which makes this not necessary anymore.
				//	But I decided to keep it in a separate function anyway to improve code readability
				injectedRules := manager.injectRules(fw.firewall.Name, ingressChain.Rule, egressChain.Rule)

				//-------------------------------------
				//	Phew... all done for this pod
				//-------------------------------------

				//	Add the newly created rules on our struct, so we can reference them at all times
				if _, exists := fw.rules[policy.Name]; !exists {
					fw.rules[policy.Name] = injectedRules
				} else {
					//	This piece of code only executes if this policy is being updated and couldn't delete rules
					fw.rules[policy.Name].ingress = append(fw.rules[policy.Name].ingress, injectedRules.ingress...)
					fw.rules[policy.Name].egress = append(fw.rules[policy.Name].egress, injectedRules.egress...)
				}
				fw.Unlock()

				//	Add this pod for this policy
				manager.deployedFirewallsLock.Lock()
				manager.deployedPolicies[policy.Name] = append(manager.deployedPolicies[policy.Name], currentPod.Pod.UID)
				manager.deployedFirewallsLock.Unlock()
			}(pod)
		}

		podsWaitGroup.Wait()
	}
}

func (manager *NetworkPolicyManager) UpdatePolicy(policy *networking_v1.NetworkPolicy) {
	var l = log.WithFields(log.Fields{
		"by":     manager.logBy,
		"method": "RemovePolicy()",
	})

	l.Debugln("Going to remove a default policy")

	/*	Our strategy for updating will be: first we remove the policy, then we redeploy it.
		But why do we deploy this again? Isn't it just enough to look for changed rules?
		Well, no:
			1) the policy might be changed in the selectors as well (earlier it applied to pods with labels A:1,
				and now it applies to pods with labels A:1 & B:2 or just B:2), thus forcing us to look for pods again.
			2) searching for changed rules, new ones and removed ones might actually bring us more
				performance penalties than advantages.
			The previous points both bring us to point 3) unfortunately, we have no way of knowing these things in advance,
			so we need to parse the policy again anyway in order to perform a correct update of the policy.
			So why complicate things when we can just reuse existing code, and break down the problem in two smaller ones
			which we already have a solution for?

		A better algorithm would be that of starting two threads: one removes the policy for a certaing pod and the other
		parses the policy, improving efficiency. In future this function might be done that way, but for now, let's not
		add additional overhead: the problem is already tricky per sè. */

	//	First, delete the policy everywhere
	manager.RemovePolicy(policy)

	//	Now deploy it again.
	manager.DeployNewPolicy(policy)
}

func (manager *NetworkPolicyManager) RemovePolicy(policy *networking_v1.NetworkPolicy) {
	var l = log.WithFields(log.Fields{
		"by":     manager.logBy,
		"method": "RemovePolicy()",
	})

	l.Debugln("Going to remove a default policy")

	//-------------------------------------
	//	The basics
	//-------------------------------------

	policyName := policy.Name
	fwIDs, exists := manager.deployedPolicies[policyName]

	if !exists {
		l.Errorln("Called to remove policy", policyName, ", but it has not been found in the deployed policies list!")
		return
	}

	if len(fwIDs) < 1 {
		l.Errorln("Called to remove policy", policyName, ", but the firewall IDs containing it is empty!")
		return
	}

	//-------------------------------------
	//	Actually remove it
	//-------------------------------------

	/*	We just got all the firewalls that implement this policy. For each of them, we have to get all the rules
		that have been generated by this policy, in order to know which ones to delete.
		Next, we delete them. Then remove that entry on the policy rules. */

	for _, fwID := range fwIDs {
		//	We're going to reuse the "exists" variable a lot. It seems dumb to use synonyms :P
		deployedFw, exists := manager.deployedFirewalls[fwID]

		if exists {
			//fwName := deployedFw.firewall.Name
			var deleteWait sync.WaitGroup
			deleteNumber := 0
			//	No need for a lock because no one else acts on this policy
			rules, exists := deployedFw.rules[policyName]

			failedRules := policyRules{
				ingress: []k8sfirewall.ChainRule{},
				egress:  []k8sfirewall.ChainRule{},
			}

			if !exists {
				l.Infoln("fw", fwID, "has no", policyName, "in its list of implemented policies rules")
			}

			if len(rules.ingress) < 1 && len(rules.egress) < 1 {
				l.Infoln("fw", fwID, "has no ingress nor egress rules for policy", policyName)
			}

			if len(rules.ingress) > 0 {
				deleteNumber++
			}
			if len(rules.egress) > 0 {
				deleteNumber++
			}

			deleteWait.Add(deleteNumber)

			if len(rules.ingress) > 0 {
				go func(rules []k8sfirewall.ChainRule) {
					defer deleteWait.Done()
					//iFailedRules := manager.fwAPI.BulkRemoveRules(fwName, "ingress", rules)
					iFailedRules := []k8sfirewall.ChainRule{}

					if len(iFailedRules) > 0 {
						failedRules.ingress = iFailedRules
					}
				}(rules.ingress)
			}

			if len(rules.egress) > 0 {
				go func(rules []k8sfirewall.ChainRule) {
					defer deleteWait.Done()
					//eFailedRules := manager.fwAPI.BulkRemoveRules(fwName, "ingress", rules)
					eFailedRules := []k8sfirewall.ChainRule{}

					if len(eFailedRules) > 0 {
						failedRules.egress = eFailedRules
					}
				}(rules.egress)
			}

			deleteWait.Wait()

			deployedFw.Lock()
			if len(failedRules.ingress) < 1 && len(failedRules.egress) < 1 {
				//	All rules were delete successfully: we may delete the entry
				delete(deployedFw.rules, policyName)

			} else {
				//	Some rules were not delete. We can't delete the entry: we need to change it with the still active rules.
				deployedFw.rules[policyName] = &failedRules
			}
			deployedFw.Unlock()
		}
	}

	//	LOCK
	delete(manager.deployedPolicies, policyName)
	//	UNLOCK
}

func (manager *NetworkPolicyManager) GetPolicyTypes(policySpec *networking_v1.NetworkPolicySpec) ([]networking_v1.NetworkPolicyIngressRule, []networking_v1.NetworkPolicyEgressRule) {
	var ingress []networking_v1.NetworkPolicyIngressRule
	var egress []networking_v1.NetworkPolicyEgressRule

	//	What it spec is not even there?
	if policySpec == nil {
		return ingress, egress
	}

	//	Documentation is not very specific about the possibility of PolicyTypes being [], so I made this dumb piece of code just in case
	if policySpec.PolicyTypes == nil {
		ingress = policySpec.Ingress
	} else {
		if len(policySpec.PolicyTypes) < 1 {
			ingress = policySpec.Ingress
		} else {
			policyTypes := policySpec.PolicyTypes

			//	https://godoc.org/k8s.io/api/networking/v1#NetworkPolicySpec =\
			for _, val := range policyTypes {
				switch val {
				case "Ingress":
					ingress = policySpec.Ingress
				case "Egress":
					egress = policySpec.Egress
				case "Ingress,Egress":
					ingress = policySpec.Ingress
					egress = policySpec.Egress
				}
			}
		}
	}

	return ingress, egress
}

func (manager *NetworkPolicyManager) injectRules(fwName string, ingress, egress []k8sfirewall.ChainRule) *policyRules {
	var l = log.WithFields(log.Fields{
		"by":     manager.logBy,
		"method": "injectRules()",
	})

	var applyWait sync.WaitGroup
	waitChains := 0

	//	Default values
	//	NOTE: these may remain empty, example: when blocking/allowing everything regardless of port and protocol.
	//	But they also remain empty if an error occurred while deploy rules. While this may seem like an
	//	incorrect behaviour, it is actually correct: for kubernetes this policy is actually *deployed*,
	//	it doesn't care if errors occurred while injecting rules.
	newRules := policyRules{
		ingress: []k8sfirewall.ChainRule{},
		egress:  []k8sfirewall.ChainRule{},
	}

	if len(ingress) > 0 {
		waitChains++
	}
	if len(egress) > 0 {
		waitChains++
	}
	applyWait.Add(waitChains)

	//	Ingress
	if len(ingress) > 0 {
		go func() {
			defer applyWait.Done()
			rulesWithIDs, err := manager.fwAPI.BulkAddRules(fwName, "ingress", ingress)

			if err != nil {
				l.Errorln("error while trying to inject ingress rules!", err)
				return
			}

			newRules.ingress = rulesWithIDs
		}()
	}

	//	Egress
	if len(egress) > 0 {
		go func() {
			defer applyWait.Done()
			rulesWithIDs, err := manager.fwAPI.BulkAddRules(fwName, "egress", egress)

			if err != nil {
				l.Errorln("error while trying to inject egress rules!", err)
				return
			}

			newRules.egress = rulesWithIDs
		}()
	}

	applyWait.Wait()

	return &newRules
}

func (manager *NetworkPolicyManager) deleteRules(ingressChain, egressChain k8sfirewall.Chain) {
	/*
		waitgroup.add(2)
		go func(ingressRulesToDelete){
			ingressDeletedRules = fwAPI.BulkDeleteIds(ingressRulesToDelete, "ingress")
			ingressRulesToKeep := manager.removeRules(deployedFw.ingressChain.Rule, ruleIDs.ingress)
			lock ingress
			deployedFw.ingressChain.Rule = ingressRulesToKeep
			unlock
		}(ruleIDs.ingress)
		go func(egressRulesToDelete){
			egressDeletedRules = fwAPI.BulkDeleteIds(egressRulesToDelete, "egress")
			egressRulesToKeep := manager.removeRules(deployedFw.egressChain.Rule, ruleIDs.egress)
			lock egress
			deployedFw.egressChain.Rule = egressRulesToKeep
			unlock
		}(ruleIDs.egress)
		wait()
	*/
}

func (manager *NetworkPolicyManager) fillChains(pod pcn_types.Pod, ingressChain, egressChain k8sfirewall.Chain) (k8sfirewall.Chain, k8sfirewall.Chain) {

	//	This is just a convenience method to make things parallel...

	var parseWait sync.WaitGroup
	var parseLen = 0

	if len(ingressChain.Rule) > 0 {
		parseLen++
	}
	if len(egressChain.Rule) > 0 {
		parseLen++
	}
	parseWait.Add(parseLen)

	if len(ingressChain.Rule) > 0 {
		go func() {
			defer parseWait.Done()
			completedRules := manager.putIPs(pod, ingressChain)
			ingressChain.Rule = completedRules
		}()
	}

	if len(egressChain.Rule) > 0 {
		go func() {
			defer parseWait.Done()
			completedRules := manager.putIPs(pod, egressChain)
			egressChain.Rule = completedRules
		}()
	}

	parseWait.Wait()

	return ingressChain, egressChain
}

func (manager *NetworkPolicyManager) putIPs(pod pcn_types.Pod, chain k8sfirewall.Chain) []k8sfirewall.ChainRule {

	generatedRules := []k8sfirewall.ChainRule{}

	for i := 0; i < len(chain.Rule); i++ {

		if chain.Name == "ingress" {
			//	Make sure not to target myself
			if chain.Rule[i].Src != pod.Pod.Status.PodIP {
				chain.Rule[i].Dst = pod.Pod.Status.PodIP
				generatedRules = append(generatedRules, chain.Rule[i])
			}
		}

		if chain.Name == "egress" {
			if chain.Rule[i].Dst != pod.Pod.Status.PodIP {
				chain.Rule[i].Src = pod.Pod.Status.PodIP
				generatedRules = append(generatedRules, chain.Rule[i])
			}
		}
	}

	return generatedRules
}

func (manager *NetworkPolicyManager) insertNewEmptyFirewall(pod core_v1.Pod) *deployedFirewall {

	fwName := string("fw-" + pod.UID)
	uid := k8s_types.UID(fwName)
	deployedFw := deployedFirewall{}

	//	TODO: some of these parts should be done with a config
	deployedFw.firewall = &k8sfirewall.Firewall{
		Name:  fwName,
		Type_: "TC",
		Ports: []k8sfirewall.Ports{},
	}
	//	UPDATE: I have moved all rules to the map below (rules) in order to improve policy->rules binding and indexing,
	//	so having ingressChain and egressChain is now useless. But I'm keeping them just for clarity.
	//	They now contain just the default action, which must be taken from configMap
	/*deployedFw.ingressChain = nil
	deployedFw.egressChain = nil*/
	deployedFw.ingressChain = &k8sfirewall.Chain{
		Name:     "ingress",
		Default_: "drop",
	}
	deployedFw.egressChain = &k8sfirewall.Chain{
		Name:     "egress",
		Default_: "drop",
	}
	deployedFw.rules = map[string]*policyRules{}
	manager.deployedFirewalls[uid] = &deployedFw

	return &deployedFw
}

func (manager *NetworkPolicyManager) ParseDefaultRules(ingress []networking_v1.NetworkPolicyIngressRule, egress []networking_v1.NetworkPolicyEgressRule, currentNamespace string) (k8sfirewall.Chain, k8sfirewall.Chain) {
	//-------------------------------------
	//	Init
	//-------------------------------------

	var l = log.WithFields(log.Fields{
		"by":     manager.logBy,
		"method": "ParseDefaultRules()",
	})

	//	TODO: the default_ action must come from ConfigMaps
	ingressChain := k8sfirewall.Chain{
		Name:     "ingress",
		Default_: "drop",
		Rule:     []k8sfirewall.ChainRule{},
	}
	egressChain := k8sfirewall.Chain{
		Name:     "egress",
		Default_: "drop",
		Rule:     []k8sfirewall.ChainRule{},
	}

	var parseWait sync.WaitGroup
	var parseLen = 0

	l.Infof("Network Policy Manager is going to parse the default policy")

	//-------------------------------------
	//	Parse the basics
	//-------------------------------------

	//	What to do when both are nil? Documentation is not clear about this...
	//	Let's just return the default action (when everything is empty, k8sfirewall generates a default)
	if ingress == nil && egress == nil {

		ingressChain = k8sfirewall.Chain{
			Name: "ingress",

			//	TODO: do this with ConfigMaps
			Default_: "drop",
		}

		egressChain = k8sfirewall.Chain{
			Name: "egress",

			//	TODO: do this with ConfigMaps
			Default_: "drop",
		}

		return ingressChain, egressChain
	}

	//-------------------------------------
	//	Group pods by their namespace
	//-------------------------------------

	//	To speed things up, we're going to parse Ingress and Egress concurrently.
	//	But how many routines do I have to wait for? (we may only have one and not both...)
	if ingress != nil {
		parseLen++
	}
	if egress != nil {
		parseLen++
	}
	parseWait.Add(parseLen)

	//-------------------------------------
	//	Parse the ingress rules
	//-------------------------------------

	if ingress != nil {
		go func() {
			defer parseWait.Done()
			ingressChain = manager.parseDefaultIngressRules(ingress, currentNamespace)
		}()
	}

	//-------------------------------------
	//	Parse the egress rules
	//-------------------------------------

	if egress != nil {
		go func() {
			defer parseWait.Done()
			egressChain = manager.parseDefaultEgressRules(egress, currentNamespace)
		}()
	}

	//	Wait for them to finish before doing the rest
	parseWait.Wait()

	return ingressChain, egressChain
}

func (manager *NetworkPolicyManager) parseDefaultIngressRules(rules []networking_v1.NetworkPolicyIngressRule, namespace string) k8sfirewall.Chain {

	//-------------------------------------
	//	Init
	//-------------------------------------

	var l = log.WithFields(log.Fields{
		"by":     manager.logBy,
		"method": "parseDefaultIngressRules()",
	})
	var ingressChain = k8sfirewall.Chain{
		Name:     "ingress",
		Default_: "drop",
		Rule:     []k8sfirewall.ChainRule{},
	}

	l.Debugln("Network Policy Manager is going to parse the ingress rules")

	//-------------------------------------
	//	Preliminary checks
	//-------------------------------------

	//	Rules is nil?
	if rules == nil {
		l.Debugln("rules is nil: nothing is accepted")

		ingressChain.Default_ = "drop"
		return ingressChain
	}

	//	No rules?
	//	Same as above, but I kept it separate to improve readability
	if len(rules) < 1 {
		l.Debugln("rules is empty: nothing is accepted")

		ingressChain.Default_ = "drop"
		return ingressChain
	}

	//-------------------------------------
	//	Actual parsing
	//-------------------------------------

	for _, rule := range rules {

		//	The rules generated in this iteration.
		var incompleteRules []k8sfirewall.ChainRule

		//	The ports generated in this iteration
		var generatedPorts = []parsedProtoPort{}

		//	Tells if we can go on parsing rules
		var proceed = true

		//-------------------------------------
		//	Protocol & Port
		//-------------------------------------

		//	First, parse the protocol: so that if an unsupported protocol is listed, we silently ignore it.
		//	By doing it this way we don't have to remove rules later on
		generatedPorts = manager.generatePorts(rule.Ports)

		//	If this rule consists of only unsupported protocols, then we can't go on!
		//	If we did, we would be creating wrong rules!
		//	We just need to ignore the rules, for now.
		//	But if there is at least one supported protocol, then we can proceed
		if len(rule.Ports) > 0 && len(generatedPorts) == 0 {
			proceed = false
		}

		//-------------------------------------
		//	Peers
		//-------------------------------------

		//	From is {} ?
		if rule.From == nil {
			l.Debugln("From is nil: ALL resources are allowed")

			//	TODO: is this correct?
			//	If we have at least one port...
			if len(generatedPorts) > 0 {
				incompleteRules = append(incompleteRules, k8sfirewall.ChainRule{
					Action: "forward",
					//	TODO: check this.
					//Src: "0",
				})
			} else {
				//	...	otherwise we need to modify the default behaviour
				ingressChain.Default_ = "forward"
			}
		}

		//	From is [] ?
		/* UPDATE: quoting from official documentation:
		"If this field is empty or missing, this rule matches all sources (traffic not restricted by
		source). If this field is present and contains at least on item, this rule
		allows traffic only if the traffic matches at least one item in the from list."
		I disabled this because this never verifies, even if you set it as nil, kubernetes instantiate it to an empty array by default. */
		/*if len(rule.From) < 1 {
			l.Debugln("From has 0 len: NO resources are allowed")

			//	TODO: is this correct?
			//	If we have at least one port...
			if len(generatedPorts) > 0 {
				incompleteRules = append(incompleteRules, k8sfirewall.ChainRule{
					Action: "drop",
					//	TODO: check this.

					Src: "0",
				})
			} else {
				//	...	otherwise we need to modify the default behaviour
				ingressChain.Default_ = "drop"
			}
		}*/

		for i := 0; i < len(rule.From) && proceed; i++ {
			from := rule.From[i]

			//	IPBlock?
			if from.IPBlock != nil {
				incompleteRules = append(incompleteRules, manager.parseDefaultIPBlock(from.IPBlock, "ingress")...)
			}

			//	PodSelector Or NamespaceSelector?
			if from.PodSelector != nil || from.NamespaceSelector != nil {

				rulesGot, err := manager.parseDefaultSelectors(from.PodSelector, from.NamespaceSelector, namespace, "ingress")

				if err == nil && len(rulesGot) > 0 {
					incompleteRules = append(incompleteRules, rulesGot...)
				}
			}
		}

		l.Debugf("generated rules: %d", len(incompleteRules))

		//-------------------------------------
		//	Finalize
		//-------------------------------------

		//	Finally, for each parsed rule, apply the ports that have been found
		//	But only if you have at least one port
		for i := 0; i < len(incompleteRules) && len(generatedPorts) > 0; i++ {
			rule := incompleteRules[i]
			for _, generatedPort := range generatedPorts {

				ingressChain.Rule = append(ingressChain.Rule, k8sfirewall.ChainRule{
					Src:     rule.Src,
					L4proto: generatedPort.protocol,
					Sport:   generatedPort.port,
					Action:  rule.Action,
				})
			}
		}

		//	No ports in this? Then just append the rules with ports 0
		if len(generatedPorts) < 1 {
			ingressChain.Rule = append(ingressChain.Rule, incompleteRules...)
		}
	}

	return ingressChain
}

func (manager *NetworkPolicyManager) parseDefaultEgressRules(rules []networking_v1.NetworkPolicyEgressRule, namespace string) k8sfirewall.Chain {

	//-------------------------------------
	//	Init
	//-------------------------------------

	var l = log.WithFields(log.Fields{
		"by":     manager.logBy,
		"method": "parseDefaultEgressRules()",
	})
	var egressChain = k8sfirewall.Chain{
		Name:     "egress",
		Default_: "drop",
		Rule:     []k8sfirewall.ChainRule{},
	}

	l.Debugln("Network Policy Manager is going to parse the egress rules")

	//-------------------------------------
	//	Preliminary checks
	//-------------------------------------

	//	Rules is nil?
	if rules == nil {
		l.Debugln("rules is nil: nothing is accepted")

		egressChain.Default_ = "drop"
		return egressChain
	}

	//	No rules?
	//	Same as above, but I kept it separate to improve readability
	if len(rules) < 1 {
		l.Debugln("rules is empty: nothing is accepted")

		egressChain.Default_ = "drop"
		return egressChain
	}

	//-------------------------------------
	//	Actual parsing
	//-------------------------------------

	for _, rule := range rules {

		//	The rules generated in this iteration.
		var incompleteRules []k8sfirewall.ChainRule

		//	The ports generated in this iteration
		var generatedPorts = []parsedProtoPort{}

		//	Tells if we can go on parsing rules
		var proceed = true

		//-------------------------------------
		//	Protocol & Port
		//-------------------------------------

		//	First, parse the protocol: so that if an unsupported protocol is listed, we silently ignore it.
		//	By doing it this way we don't have to remove rules later on
		if len(rule.Ports) > 0 {
			generatedPorts = manager.generatePorts(rule.Ports)
		}

		l.Debugf("generated ports: %d", len(generatedPorts))

		//	If this rule consists of only unsupported protocols, then we can't go on!
		//	If we did, we would be creating wrong rules!
		//	We just need to ignore the rules, for now.
		//	But if there is at least one supported protocol, then we can proceed
		if len(rule.Ports) > 0 && len(generatedPorts) == 0 {
			proceed = false
		}

		//-------------------------------------
		//	Peers
		//-------------------------------------

		//	To is {} ?
		if rule.To == nil {
			l.Debugln("To is nil: ALL resources are allowed")

			//	TODO: is this correct?
			//	If we have at least one port...
			if len(generatedPorts) > 0 {
				incompleteRules = append(incompleteRules, k8sfirewall.ChainRule{
					Action: "forward",
					//	TODO: check this.
					//Dst: "0",
				})
			} else {
				//	...	otherwise we need to modify the default behaviour
				egressChain.Default_ = "forward"
			}
		}

		//	To is [] ?
		//	UPDATE: commented this. Read what I wrote about FROM
		/*if len(rule.To) < 1 {
			l.Infoln("To has 0 len: NO resources are allowed")

			//	TODO: is this correct?
			//	If we have at least one port...
			if len(generatedPorts) > 0 {
				incompleteRules = append(incompleteRules, k8sfirewall.ChainRule{
					Action: "drop",
					//	TODO: check this.

					Dst: "0",
				})
			} else {
				//	...	otherwise we need to modify the default behaviour
				egressChain.Default_ = "drop"
			}
		}*/

		for i := 0; i < len(rule.To) && proceed; i++ {
			to := rule.To[i]

			//	IPBlock?
			if to.IPBlock != nil {
				incompleteRules = append(incompleteRules, manager.parseDefaultIPBlock(to.IPBlock, "egress")...)
			}

			//	PodSelector Or NamespaceSelector?
			if to.PodSelector != nil || to.NamespaceSelector != nil {

				rulesGot, err := manager.parseDefaultSelectors(to.PodSelector, to.NamespaceSelector, namespace, "egress")

				if err == nil && len(rulesGot) > 0 {
					incompleteRules = append(incompleteRules, rulesGot...)
				}
			}
		}

		l.Debugf("generated rules: %d", len(incompleteRules))

		//-------------------------------------
		//	Finalize
		//-------------------------------------

		//	Finally, for each parsed rule, apply the ports that have been found
		//	But only if you have at least one port
		for i := 0; i < len(incompleteRules) && len(generatedPorts) > 0; i++ {
			rule := incompleteRules[i]
			for _, generatedPort := range generatedPorts {
				egressChain.Rule = append(egressChain.Rule, k8sfirewall.ChainRule{
					Dst:     rule.Dst,
					L4proto: generatedPort.protocol,
					Dport:   generatedPort.port,
					Action:  rule.Action,
				})
			}
		}

		//	No ports in this? Then just append the rules with ports 0
		if len(generatedPorts) < 1 {
			egressChain.Rule = append(egressChain.Rule, incompleteRules...)
		}
	}

	return egressChain
}

func (manager *NetworkPolicyManager) parseDefaultIPBlock(block *networking_v1.IPBlock, direction string) []k8sfirewall.ChainRule {

	var l = log.WithFields(log.Fields{
		"by":     manager.logBy,
		"method": "parseDefaultIPBlock()",
	})
	var rules []k8sfirewall.ChainRule

	l.Debugln("Parsing IPBlock...")

	//	Add the default one
	mainBlock := k8sfirewall.ChainRule{
		Action: "forward",
	}

	//	Set the direction
	if direction == "ingress" {
		mainBlock.Src = block.CIDR
	} else {
		mainBlock.Dst = block.CIDR
	}
	rules = append(rules, mainBlock)

	//	Loop through all exceptions
	for _, exception := range block.Except {
		exceptionRule := k8sfirewall.ChainRule{
			Action: "drop",
		}

		if direction == "ingress" {
			exceptionRule.Src = exception
		} else {
			exceptionRule.Dst = exception
		}

		rules = append(rules, exceptionRule)
	}

	return rules
}

func (manager *NetworkPolicyManager) parseDefaultProtocolAndPort(pp networking_v1.NetworkPolicyPort) (bool, string, int32) {

	//	Not sure if port can be nil, but it doesn't harm to do a simple reset
	var port int32
	if pp.Port != nil {
		port = int32(pp.Port.IntValue())
	}

	//	TCP?
	if *pp.Protocol == core_v1.ProtocolTCP {
		return true, "TCP", port
	}

	//	UDP?
	if *pp.Protocol == core_v1.ProtocolUDP {
		return true, "UDP", port
	}

	//	Not supported ¯\_(ツ)_/¯
	return false, "", 0
}

func (manager *NetworkPolicyManager) generatePorts(ports []networking_v1.NetworkPolicyPort) []parsedProtoPort {

	//	Init
	generatedPorts := []parsedProtoPort{}

	for _, port := range ports {

		//	If protocol is nil, then we have to get all protocols
		if port.Protocol == nil {

			//	If the port is not nil, default port is not 0
			var defaultPort int32
			if port.Port != nil {
				defaultPort = int32(port.Port.IntValue())
			}

			for _, protocol := range strings.Split(supportedProtocols, ",") {
				generatedPorts = append(generatedPorts, parsedProtoPort{protocol, defaultPort})
			}

		} else {
			//	else parse the protocol
			supported, proto, port := manager.parseDefaultProtocolAndPort(port)

			//	Our firewall does not support SCTP, so we check if protocol is supported
			if supported {
				generatedPorts = append(generatedPorts, parsedProtoPort{
					protocol: proto,
					port:     port,
				})
			}
		}
	}

	return generatedPorts
}

func (manager *NetworkPolicyManager) parseDefaultSelectors(podSelector, namespaceSelector *meta_v1.LabelSelector, namespace, direction string) ([]k8sfirewall.ChainRule, error) {
	var l = log.WithFields(log.Fields{
		"by":     manager.logBy,
		"method": "parseDefaultSelectors()",
	})
	rules := []k8sfirewall.ChainRule{}
	action := "forward"

	l.Debugln("Going to parse the default selectors....")

	if podSelector != nil && podSelector.MatchLabels == nil {
		action = "drop"
	}

	podsFound, err := manager.getPodsFromDefaultSelectors(podSelector, namespaceSelector, namespace)

	if err != nil {
		return nil, err
	}

	// No need to check for len of podsFound.
	for _, pod := range podsFound {
		rule := k8sfirewall.ChainRule{
			Action: action,
		}

		if direction == "ingress" {
			rule.Src = pod.Pod.Status.PodIP
		} else {
			rule.Dst = pod.Pod.Status.PodIP
		}

		rules = append(rules, rule)
	}

	return rules, nil
}

func (manager *NetworkPolicyManager) getPodsFromDefaultSelectors(podSelector, namespaceSelector *meta_v1.LabelSelector, namespace string) ([]pcn_types.Pod, error) {
	var l = log.WithFields(log.Fields{
		"by":     manager.logBy,
		"method": "getPodsFromDefaultSelectors()",
	})
	podsFound := []pcn_types.Pod{}
	var query pcn_types.Query

	l.Debugln("Parsing selectors...")

	//	If podSelector is nil we must not select anything (meaning we have to block everything)
	//	If podSelector is empty (len = 0) we must select everything
	//	NOTE: blocking everything is not the same as setting a default rule to block anything!
	//	Because that way we would also be preventing external connections from accessing our pods.
	//	Instead, we need to block all pods individually, so we can't solve it by just creating a default rule:
	//	we cannot know if user will deploy a policy to allow ipblocks in advance.

	//	Build the query
	if podSelector != nil {

		//	This is not supported yet...
		if podSelector.MatchExpressions != nil {
			return nil, errors.New("MatchExpressions on pod selector is not supported yet")
		}

		//	Empty labels means "select everything"
		//	Nil labels means do not select anything. Which, for us, means deny access to those pods (see below)
		if len(podSelector.MatchLabels) < 1 || podSelector.MatchLabels == nil {
			query.Pod = []pcn_types.QueryObject{
				pcn_types.QueryObject{
					By:   "name",
					Name: "*",
				},
			}
		} else {
			query.Pod = []pcn_types.QueryObject{
				pcn_types.QueryObject{
					By:     "labels",
					Labels: podSelector.MatchLabels,
				},
			}
		}
	} else {
		query.Pod = []pcn_types.QueryObject{
			pcn_types.QueryObject{
				By:   "name",
				Name: "*",
			},
		}
	}

	//	Namespace selector
	if namespaceSelector != nil {

		//	Match expressions?
		if namespaceSelector.MatchExpressions != nil {
			//	This is not supported yet...
			return nil, errors.New("MatchExpressions on namespace selector is not supported yet")
		}

		if len(namespaceSelector.MatchLabels) > 0 {
			//	Parse the match labels (like for the pod)
			query.Namespace = []pcn_types.QueryObject{
				pcn_types.QueryObject{
					By:     "labels",
					Labels: namespaceSelector.MatchLabels,
				},
			}
		} else {
			//	No labels: as per documentation, this means ALL namespaces
			query.Namespace = []pcn_types.QueryObject{
				pcn_types.QueryObject{
					By:   "name",
					Name: "*",
				},
			}
		}
	} else {
		//	If namespace selector is nil, we're going to use the one we found on the policy
		query.Namespace = []pcn_types.QueryObject{
			pcn_types.QueryObject{
				By:   "name",
				Name: namespace,
			},
		}
	}

	//	Now get the pods
	podsFound, err := manager.podController.GetPods(query)

	if err != nil {
		l.Errorln("error when trying to get pods:", err)
		return nil, errors.New("error when trying to get pods")
	}

	return podsFound, nil
}

func (manager *NetworkPolicyManager) getPodsAffectedByDefaultPolicy(podSelector *meta_v1.LabelSelector, namespace string) (map[string][]pcn_types.Pod, error) {
	var l = log.WithFields(log.Fields{
		"by":     manager.logBy,
		"method": "getPodsAffectedByDefaultPolicy()",
	})

	var namespacesGroup map[string][]pcn_types.Pod

	//	Get the pods this policy must be applied to
	podsAffected, err := manager.getPodsFromDefaultSelectors(podSelector, nil, namespace)

	//	Something happened?
	if err != nil {
		return nil, err
	}

	//	No pods found?
	if len(podsAffected) < 1 {
		l.Infoln("No pods found.")
		return namespacesGroup, nil
	}

	//-------------------------------------
	//	Group pods by their namespace
	//-------------------------------------

	namespacesGroup = make(map[string][]pcn_types.Pod)

	//	This is important! Some rules specify that the restriction must be applied to the namespace they BELONG, not every single one.
	//	Example: IPBlock rules don't consider namespaces, but PodSelector (and/or NamespaceSelector) may restrict access based on the
	//	namespace that particular pod is in (i.e.: only allowing pods from the same namespace they are found).
	//	But unfortunately, we don't know in advance if the policy only consists of IPBlock-s without parsing it first.
	//	So, we need to group our found pods by their namespace, in order to do a correct parsing.
	for _, pod := range podsAffected {

		if _, ok := namespacesGroup[pod.Pod.Namespace]; !ok {
			namespacesGroup[pod.Pod.Namespace] = []pcn_types.Pod{}
		}

		namespacesGroup[pod.Pod.Namespace] = append(namespacesGroup[pod.Pod.Namespace], pod)
	}

	return namespacesGroup, nil
}
