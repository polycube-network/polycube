package networkpolicies

import (
	"errors"
	"strings"
	"sync"

	pcn_controllers "github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/controllers"
	pcn_firewall "github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/networkpolicies/pcn_firewall"
	pcn_types "github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/types"
	k8sfirewall "github.com/SunSince90/polycube/src/components/k8s/utils/k8sfirewall"

	log "github.com/sirupsen/logrus"
	core_v1 "k8s.io/api/core/v1"
	networking_v1 "k8s.io/api/networking/v1"
	meta_v1 "k8s.io/apimachinery/pkg/apis/meta/v1"
	k8s_types "k8s.io/apimachinery/pkg/types"
)

type PcnDefaultPolicyParser interface {
	Parse(*networking_v1.NetworkPolicy, bool) map[string]map[k8s_types.UID]*ParsedPolicy
	ParsePolicyTypes(*networking_v1.NetworkPolicySpec) ([]networking_v1.NetworkPolicyIngressRule, []networking_v1.NetworkPolicyEgressRule)
	//ParseAndDeploy(*networking_v1.NetworkPolicy)
	ParseRules([]networking_v1.NetworkPolicyIngressRule, []networking_v1.NetworkPolicyEgressRule, string) (k8sfirewall.Chain, k8sfirewall.Chain)
	ParseIngress([]networking_v1.NetworkPolicyIngressRule, string) k8sfirewall.Chain
	ParseEgress([]networking_v1.NetworkPolicyEgressRule, string) k8sfirewall.Chain
	ParseIPBlock(*networking_v1.IPBlock, string) []k8sfirewall.ChainRule
	ParsePorts([]networking_v1.NetworkPolicyPort) []parsedProtoPort
	ParseSelectors(*meta_v1.LabelSelector, *meta_v1.LabelSelector, string, string) ([]k8sfirewall.ChainRule, error)
	FillChains(pcn_types.Pod, k8sfirewall.Chain, k8sfirewall.Chain) (k8sfirewall.Chain, k8sfirewall.Chain)
	GetPodsAffected(*networking_v1.NetworkPolicy) (map[string][]pcn_types.Pod, error)
	DoesPolicyAffectPod(*networking_v1.NetworkPolicy, *core_v1.Pod) bool
	DoesPolicyTargetPod(*networking_v1.NetworkPolicy, *core_v1.Pod) ([]k8sfirewall.ChainRule, []k8sfirewall.ChainRule)
}

type DefaultPolicyParser struct {
	firewallManager      pcn_firewall.Manager
	podController        pcn_controllers.PodController
	networkPolicyManager PcnNetworkPolicyManager
	supportedProtocols   string
}

type parsedProtoPort struct {
	protocol string
	port     int32
}

func newDefaultPolicyParser(podController pcn_controllers.PodController, firewallManager pcn_firewall.Manager, networkPolicyManager PcnNetworkPolicyManager) *DefaultPolicyParser {
	return &DefaultPolicyParser{
		networkPolicyManager: networkPolicyManager,
		firewallManager:      firewallManager,
		podController:        podController,
		supportedProtocols:   "TCP,UDP",
	}
}

func (d *DefaultPolicyParser) Parse(policy *networking_v1.NetworkPolicy, deploy bool) map[string]map[k8s_types.UID]*ParsedPolicy {

	var l = log.WithFields(log.Fields{
		"by":     DPS,
		"method": "Parse() [" + policy.Name + "]",
	})

	l.Debugln("Going to parse a default policy")

	//-------------------------------------
	//	The basics
	//-------------------------------------

	nsPods, err := d.GetPodsAffected(policy)

	if err != nil {
		l.Errorln("Error while trying to get pods affected", err)
		return nil
	}

	//	No pods found?
	if len(nsPods) < 1 {
		return nil
	}

	//	Get the spec, with the ingress & egress rules
	spec := policy.Spec
	ingress, egress := d.ParsePolicyTypes(&spec)

	parsed := map[string]map[k8s_types.UID]*ParsedPolicy{}

	//-------------------------------------
	//	Parse the rules
	//-------------------------------------

	//	Get the parsed chains
	//	TODO: goroutine for each ns as well?
	for ns, pods := range nsPods {

		var nsLock sync.Mutex
		parsed[ns] = map[k8s_types.UID]*ParsedPolicy{}

		//	Get the chains
		ingressChain, egressChain := d.ParseRules(ingress, egress, ns)

		var podsWaitGroup sync.WaitGroup
		podsWaitGroup.Add(len(pods))

		for _, pod := range pods {
			go func(currentPod pcn_types.Pod) {

				defer podsWaitGroup.Done()

				//	Complete the rules by correctly adding the IPs
				ingressChain, egressChain := d.FillChains(currentPod, ingressChain, egressChain)

				if deploy {
					//	Create the firewall (or get it if already exists)
					fw := d.firewallManager.GetOrCreate(currentPod.Pod)
					if fw == nil {
						l.Panicln("Could not create firewall fw-", currentPod.Pod, ". Will stop here.")
						return
					}

					//-------------------------------------
					//	Inject the rules
					//-------------------------------------

					log.Debugf("--ingress: %+v\n --egress: %+v\n")
					iErr, eErr := fw.EnforcePolicy(policy.Name, ingressChain.Rule, egressChain.Rule)

					//-------------------------------------
					//	Phew... all done for this pod
					//-------------------------------------

					//
					if iErr == nil || eErr == nil {
						nsLock.Lock()
						parsed[ns][currentPod.Pod.UID] = &ParsedPolicy{
							ingressChain: ingressChain,
							egressChain:  egressChain,
						}
						nsLock.Unlock()
					}
				} else {
					nsLock.Lock()
					parsed[ns][currentPod.Pod.UID] = &ParsedPolicy{
						ingressChain: ingressChain,
						egressChain:  egressChain,
					}
					nsLock.Unlock()
				}

			}(pod)
		}

		podsWaitGroup.Wait()
	}

	return parsed
}

func (d *DefaultPolicyParser) ParseRules(ingress []networking_v1.NetworkPolicyIngressRule, egress []networking_v1.NetworkPolicyEgressRule, currentNamespace string) (k8sfirewall.Chain, k8sfirewall.Chain) {
	//-------------------------------------
	//	Init
	//-------------------------------------

	var l = log.WithFields(log.Fields{
		"by":     DPS,
		"method": "ParseRules()",
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
			ingressChain = d.ParseIngress(ingress, currentNamespace)
		}()
	}

	//-------------------------------------
	//	Parse the egress rules
	//-------------------------------------

	if egress != nil {
		go func() {
			defer parseWait.Done()
			egressChain = d.ParseEgress(egress, currentNamespace)
		}()
	}

	//	Wait for them to finish before doing the rest
	parseWait.Wait()

	return ingressChain, egressChain
}

func (d *DefaultPolicyParser) ParseIngress(rules []networking_v1.NetworkPolicyIngressRule, namespace string) k8sfirewall.Chain {

	//-------------------------------------
	//	Init
	//-------------------------------------

	var l = log.WithFields(log.Fields{
		"by":     DPS,
		"method": "ParseIngress()",
	})
	var ingressChain = k8sfirewall.Chain{
		Name:     "ingress",
		Default_: "drop",
		Rule:     []k8sfirewall.ChainRule{},
	}

	l.Debugln("Going to parse the ingress rules")

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
		generatedPorts = d.ParsePorts(rule.Ports)

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

		for i := 0; rule.From != nil && i < len(rule.From) && proceed; i++ {
			from := rule.From[i]

			//	IPBlock?
			if from.IPBlock != nil {
				incompleteRules = append(incompleteRules, d.ParseIPBlock(from.IPBlock, "ingress")...)
			}

			//	PodSelector Or NamespaceSelector?
			if from.PodSelector != nil || from.NamespaceSelector != nil {

				rulesGot, err := d.ParseSelectors(from.PodSelector, from.NamespaceSelector, namespace, "ingress")

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

func (d *DefaultPolicyParser) ParseEgress(rules []networking_v1.NetworkPolicyEgressRule, namespace string) k8sfirewall.Chain {

	//-------------------------------------
	//	Init
	//-------------------------------------

	var l = log.WithFields(log.Fields{
		"by":     DPS,
		"method": "ParseEgress()",
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
			generatedPorts = d.ParsePorts(rule.Ports)
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

		for i := 0; rule.To != nil && i < len(rule.To) && proceed; i++ {
			to := rule.To[i]

			//	IPBlock?
			if to.IPBlock != nil {
				incompleteRules = append(incompleteRules, d.ParseIPBlock(to.IPBlock, "egress")...)
			}

			//	PodSelector Or NamespaceSelector?
			if to.PodSelector != nil || to.NamespaceSelector != nil {

				rulesGot, err := d.ParseSelectors(to.PodSelector, to.NamespaceSelector, namespace, "egress")

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

func (d *DefaultPolicyParser) ParsePolicyTypes(policySpec *networking_v1.NetworkPolicySpec) ([]networking_v1.NetworkPolicyIngressRule, []networking_v1.NetworkPolicyEgressRule) {
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

func (d *DefaultPolicyParser) FillChains(pod pcn_types.Pod, ingressChain, egressChain k8sfirewall.Chain) (k8sfirewall.Chain, k8sfirewall.Chain) {

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
			completedRules := d.putIPs(pod, ingressChain)
			ingressChain.Rule = completedRules
		}()
	}

	if len(egressChain.Rule) > 0 {
		go func() {
			defer parseWait.Done()
			completedRules := d.putIPs(pod, egressChain)
			egressChain.Rule = completedRules
		}()
	}

	parseWait.Wait()

	return ingressChain, egressChain
}

func (d *DefaultPolicyParser) putIPs(pod pcn_types.Pod, chain k8sfirewall.Chain) []k8sfirewall.ChainRule {

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

func (d *DefaultPolicyParser) ParseIPBlock(block *networking_v1.IPBlock, direction string) []k8sfirewall.ChainRule {

	var l = log.WithFields(log.Fields{
		"by":     DPS,
		"method": "ParseIPBlock()",
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

func (d *DefaultPolicyParser) ParsePorts(ports []networking_v1.NetworkPolicyPort) []parsedProtoPort {

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

			for _, protocol := range strings.Split(d.supportedProtocols, ",") {
				generatedPorts = append(generatedPorts, parsedProtoPort{protocol, defaultPort})
			}

		} else {
			//	else parse the protocol
			supported, proto, port := d.parseProtocolAndPort(port)

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

func (d *DefaultPolicyParser) parseProtocolAndPort(pp networking_v1.NetworkPolicyPort) (bool, string, int32) {

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

func (d *DefaultPolicyParser) ParseSelectors(podSelector, namespaceSelector *meta_v1.LabelSelector, namespace, direction string) ([]k8sfirewall.ChainRule, error) {
	var l = log.WithFields(log.Fields{
		"by":     DPS,
		"method": "ParseSelectors()",
	})
	rules := []k8sfirewall.ChainRule{}
	action := "forward"

	l.Debugln("Going to parse the default selectors....")

	if podSelector != nil && podSelector.MatchLabels == nil {
		action = "drop"
	}

	podsFound, err := d.getPodsFromSelectors(podSelector, namespaceSelector, namespace)

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

func (d *DefaultPolicyParser) getPodsFromSelectors(podSelector, namespaceSelector *meta_v1.LabelSelector, namespace string) ([]pcn_types.Pod, error) {
	var l = log.WithFields(log.Fields{
		"by":     DPS,
		"method": "getPodsFromSelectors()",
	})
	podsFound := []pcn_types.Pod{}
	var query pcn_types.PodQuery

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
			query.Pod = pcn_types.PodQueryObject{
				By:   "name",
				Name: "*",
			}
		} else {
			query.Pod = pcn_types.PodQueryObject{
				By:     "labels",
				Labels: podSelector.MatchLabels,
			}
		}
	} else {
		query.Pod = pcn_types.PodQueryObject{
			By:   "name",
			Name: "*",
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
			query.Namespace = pcn_types.PodQueryObject{
				By:     "labels",
				Labels: namespaceSelector.MatchLabels,
			}
		} else {
			//	No labels: as per documentation, this means ALL namespaces
			query.Namespace = pcn_types.PodQueryObject{
				By:   "name",
				Name: "*",
			}
		}
	} else {
		//	If namespace selector is nil, we're going to use the one we found on the policy
		query.Namespace = pcn_types.PodQueryObject{
			By:   "name",
			Name: namespace,
		}
	}

	//	Now get the pods
	podsFound, err := d.podController.GetPods(query)

	if err != nil {
		l.Errorln("error when trying to get pods:", err)
		return nil, errors.New("error when trying to get pods")
	}

	return podsFound, nil
}

func (d *DefaultPolicyParser) GetPodsAffected(policy *networking_v1.NetworkPolicy) (map[string][]pcn_types.Pod, error) {
	var l = log.WithFields(log.Fields{
		"by":     DPS,
		"method": "GetPodsAffected()",
	})

	//	Get the namespace (for us, '*' means all namespaces)
	namespace := "*"
	if len(policy.ObjectMeta.Namespace) > 0 {
		namespace = policy.Namespace
	}

	var namespacesGroup map[string][]pcn_types.Pod

	//	Get the pods this policy must be applied to
	podsAffected, err := d.getPodsFromSelectors(&policy.Spec.PodSelector, nil, namespace)

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

func (d *DefaultPolicyParser) DoesPolicyAffectPod(policy *networking_v1.NetworkPolicy, pod *core_v1.Pod) bool {

	//	First, get pods affected by the policy.
	podsAffected, err := d.GetPodsAffected(policy)
	if err != nil {
		return false
	}

	//	Now let's check if this pod is included:
	//	Doesn't even affect anyone
	if len(podsAffected) < 1 {
		return false
	}

	//	Its namespace is not affected?
	podsInNamespace, exists := podsAffected[pod.Namespace]
	if !exists {
		return false
	}

	//	Finally, is this pod affected?
	for _, currentPod := range podsInNamespace {
		if currentPod.Pod.UID == pod.UID {
			return true
		}
	}

	return false
}

func (d *DefaultPolicyParser) DoesPolicyTargetPod(policy *networking_v1.NetworkPolicy, pod *core_v1.Pod) ([]k8sfirewall.ChainRule, []k8sfirewall.ChainRule) {

	//	Get the spec, with the ingress & egress rules
	spec := policy.Spec
	ingress, egress := d.ParsePolicyTypes(&spec)

	if ingress == nil && egress == nil {
		return []k8sfirewall.ChainRule{}, []k8sfirewall.ChainRule{}
	}

	var parseWait sync.WaitGroup
	parseLen := 0
	if ingress != nil {
		parseLen++
	}
	if egress != nil {
		parseLen++
	}
	parseWait.Add(parseLen)

	//-------------------------------------
	//	Check for ingress
	//-------------------------------------

	ingressRules := []k8sfirewall.ChainRule{}

	/*if ingress == nil || (ingress != nil && len(ingress) < 1) {
		ingressRules = append(ingressRules, k8sfirewall.ChainRule{
			Src:    pod.Status.PodIP,
			Action: "drop",
		})
	}*/

	if ingress != nil && len(ingress) > 0 {
		go func() {
			defer parseWait.Done()
			for _, rule := range ingress {
				generatedPorts := []parsedProtoPort{}
				proceed := true

				if rule.Ports != nil {
					if len(rule.Ports) > 0 {
						generatedPorts = d.ParsePorts(rule.Ports)

						if len(generatedPorts) == 0 {
							proceed = false
						}
					}
				}

				if len(rule.From) < 1 {
					for _, generatedPort := range generatedPorts {
						ingressRules = append(ingressRules, k8sfirewall.ChainRule{
							Src:     pod.Status.PodIP,
							L4proto: generatedPort.protocol,
							Sport:   generatedPort.port,
							Action:  "forward",
						})
					}
					if len(generatedPorts) < 1 {
						ingressRules = append(ingressRules, k8sfirewall.ChainRule{
							Src:    pod.Status.PodIP,
							Action: "forward",
						})
					}
				}

				for _, from := range rule.From {

					if proceed {
						if from.PodSelector != nil || from.NamespaceSelector != nil {
							podsFound, err := d.getPodsFromSelectors(from.PodSelector, from.NamespaceSelector, pod.Namespace)
							if err == nil {
								for _, podFound := range podsFound {
									if podFound.Pod.UID == pod.UID {
										//	Found it!
										for _, generatedPort := range generatedPorts {
											ingressRules = append(ingressRules, k8sfirewall.ChainRule{
												Src:     pod.Status.PodIP,
												L4proto: generatedPort.protocol,
												Sport:   generatedPort.port,
												Action:  "forward",
											})
										}
										if len(generatedPorts) < 1 {
											ingressRules = append(ingressRules, k8sfirewall.ChainRule{
												Src:    pod.Status.PodIP,
												Action: "forward",
											})
										}
									}
								}
							}
						}
					}
				}
			}
		}()
	}

	//-------------------------------------
	//	Check for egress
	//-------------------------------------

	egressRules := []k8sfirewall.ChainRule{}

	/*if egress == nil || (egress != nil && len(egress) < 1) {
		egressRules = append(egressRules, k8sfirewall.ChainRule{
			Src:    pod.Status.PodIP,
			Action: "drop",
		})
	}*/

	if egress != nil && len(egress) > 0 {
		go func() {
			defer parseWait.Done()
			for _, rule := range egress {

				generatedPorts := []parsedProtoPort{}
				proceed := true

				if rule.Ports != nil {
					if len(rule.Ports) > 0 {
						generatedPorts = d.ParsePorts(rule.Ports)

						if len(generatedPorts) == 0 {
							proceed = false
						}
					}
				}

				if len(rule.To) < 1 {
					for _, generatedPort := range generatedPorts {
						egressRules = append(egressRules, k8sfirewall.ChainRule{
							Dst:     pod.Status.PodIP,
							L4proto: generatedPort.protocol,
							Dport:   generatedPort.port,
							Action:  "forward",
						})
					}
					if len(generatedPorts) < 1 {
						egressRules = append(egressRules, k8sfirewall.ChainRule{
							Dst:    pod.Status.PodIP,
							Action: "forward",
						})
					}
				}

				for _, to := range rule.To {

					if proceed {
						if to.PodSelector != nil || to.NamespaceSelector != nil {
							podsFound, err := d.getPodsFromSelectors(to.PodSelector, to.NamespaceSelector, pod.Namespace)
							if err == nil {
								for _, podFound := range podsFound {
									if podFound.Pod.UID == pod.UID {
										//	Found it!
										for _, generatedPort := range generatedPorts {
											egressRules = append(egressRules, k8sfirewall.ChainRule{
												Dst:     pod.Status.PodIP,
												L4proto: generatedPort.protocol,
												Dport:   generatedPort.port,
												Action:  "forward",
											})
										}
										if len(generatedPorts) < 1 {
											egressRules = append(egressRules, k8sfirewall.ChainRule{
												Dst:    pod.Status.PodIP,
												Action: "forward",
											})
										}
									}
								}
							}
						}
					}
				}
			}
		}()
	}

	//	Wait for them to finish before doing the rest
	parseWait.Wait()

	return ingressRules, egressRules
}
