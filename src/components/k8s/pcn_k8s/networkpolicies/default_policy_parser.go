package networkpolicies

import (
	"errors"
	"fmt"
	"sync"

	pcn_controllers "github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/controllers"
	pcn_firewall "github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/networkpolicies/pcn_firewall"
	pcn_types "github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/types"
	k8sfirewall "github.com/SunSince90/polycube/src/components/k8s/utils/k8sfirewall"

	log "github.com/sirupsen/logrus"
	core_v1 "k8s.io/api/core/v1"
	networking_v1 "k8s.io/api/networking/v1"
	meta_v1 "k8s.io/apimachinery/pkg/apis/meta/v1"
)

type PcnDefaultPolicyParser interface {
	Parse(*networking_v1.NetworkPolicy, bool)
	//Parse(*networking_v1.NetworkPolicy, deploy bool) map[string]map[k8s_types.UID]pcn_types.ParsedRules
	ParsePolicyTypes(*networking_v1.NetworkPolicySpec) ([]networking_v1.NetworkPolicyIngressRule, []networking_v1.NetworkPolicyEgressRule, string)
	ParseRules([]networking_v1.NetworkPolicyIngressRule, []networking_v1.NetworkPolicyEgressRule, string) pcn_types.ParsedRules
	ParseIngress([]networking_v1.NetworkPolicyIngressRule, string) pcn_types.ParsedRules
	ParseEgress([]networking_v1.NetworkPolicyEgressRule, string) pcn_types.ParsedRules
	ParseIPBlock(*networking_v1.IPBlock, string) pcn_types.ParsedRules
	ParsePorts([]networking_v1.NetworkPolicyPort) []pcn_types.ProtoPort
	ParseSelectors(*meta_v1.LabelSelector, *meta_v1.LabelSelector, string, string) (pcn_types.ParsedRules, error)
	GetPodsAffected(*networking_v1.NetworkPolicy) (map[string][]core_v1.Pod, error)
	DoesPolicyAffectPod(*networking_v1.NetworkPolicy, *core_v1.Pod) bool
	DoesPolicyTargetPod(*networking_v1.NetworkPolicy, *core_v1.Pod) pcn_types.ParsedRules
	GetClusterActions([]networking_v1.NetworkPolicyIngressRule, []networking_v1.NetworkPolicyEgressRule, string) ([]pcn_types.FirewallAction, []pcn_types.FirewallAction)
	GetConnectionTemplate(string, string, string, string, []pcn_types.ProtoPort) pcn_types.ParsedRules
}

type DefaultPolicyParser struct {
	firewallManager    pcn_firewall.Manager
	podController      pcn_controllers.PodController
	supportedProtocols string
	node               string
	log                *log.Logger
}

func newDefaultPolicyParser(podController pcn_controllers.PodController, firewallManager pcn_firewall.Manager, nodeName string) *DefaultPolicyParser {
	return &DefaultPolicyParser{
		firewallManager:    firewallManager,
		podController:      podController,
		supportedProtocols: "TCP,UDP",
		node:               nodeName,
		log:                log.New(),
	}
}

func (d *DefaultPolicyParser) Parse(policy *networking_v1.NetworkPolicy, deploy bool) {

	var l = log.WithFields(log.Fields{
		"by":     DPS,
		"method": "Parse() [" + policy.Name + "]",
	})

	//l.Debugln("Going to parse a default policy")

	//-------------------------------------
	//	The basics
	//-------------------------------------

	nsPods, err := d.GetPodsAffected(policy)

	if err != nil {
		//l.Errorln("Error while trying to get pods affected", err)
		return
	}

	//	No pods found?
	if len(nsPods) < 1 {
		return
	}

	//	Get the spec, with the ingress & egress rules
	spec := policy.Spec
	ingress, egress, policyType := d.ParsePolicyTypes(&spec)
	if ingress == nil && egress == nil {
		l.Errorln("Policy doesn't have a spec: I don't know what to do!")
		return
	}

	//-------------------------------------
	//	Parse the rules
	//-------------------------------------

	//	Get the parsed chains
	//	TODO: goroutine for each ns as well?
	for ns, pods := range nsPods {

		//var nsLock sync.Mutex
		//parsed[ns] = map[k8s_types.UID]*ParsedPolicy{}

		//	Get the chains
		parsed := d.ParseRules(ingress, egress, ns)

		var podsWaitGroup sync.WaitGroup
		podsWaitGroup.Add(len(pods))

		for _, pod := range pods {

			go func(currentPod core_v1.Pod) {
				defer podsWaitGroup.Done()

				//	Running in this node? If not, getting the firewall is useless (it's not here.)
				//	TODO: https://godoc.org/k8s.io/api/core/v1#PodStatus, NodeName is correct?
				//	TODO: later, this will be placed after fullRules := ... because for now this function does not return anything.
				if currentPod.Spec.NodeName != d.node {
					return
				}

				//	Complete the rules by correctly adding the IPs
				//fullRules := d.FillChains(currentPod, parsed.Ingress, parsed.Egress)

				if deploy {
					//	Create the firewall (or get it if already exists)
					fw := d.firewallManager.GetOrCreate(currentPod)
					if fw == nil {
						//l.Errorln("Could not create firewall fw-", currentPod.Status.PodIP, ". Will stop here.")
						return
					}

					//-------------------------------------
					//	Inject the rules
					//-------------------------------------

					iErr, eErr := fw.EnforcePolicy(policy.Name, policyType, parsed.Ingress, parsed.Egress)

					//-------------------------------------
					//	Phew... all done for this pod
					//-------------------------------------

					//
					if iErr == nil || eErr == nil {
						/*nsLock.Lock()
						parsed[ns][currentPod.UID] = &ParsedPolicy{
							ingressChain: ingressChain,
							egressChain:  egressChain,
						}
						nsLock.Unlock()*/
					}
				} else {
					/*nsLock.Lock()
					parsed[ns][currentPod.UID] = &ParsedPolicy{
						ingressChain: ingressChain,
						egressChain:  egressChain,
					}
					nsLock.Unlock()*/
				}

			}(pod)
		}

		podsWaitGroup.Wait()
	}

	//return parsed
}

func (d *DefaultPolicyParser) ParseRules(ingress []networking_v1.NetworkPolicyIngressRule, egress []networking_v1.NetworkPolicyEgressRule, currentNamespace string) pcn_types.ParsedRules {
	//-------------------------------------
	//	Init
	//-------------------------------------

	/*var l = log.WithFields(log.Fields{
		"by":     DPS,
		"method": "ParseRules()",
	})*/

	parsed := pcn_types.ParsedRules{
		Ingress: []k8sfirewall.ChainRule{},
		Egress:  []k8sfirewall.ChainRule{},
	}

	var parseWait sync.WaitGroup
	//var parseLen = 0
	var lock sync.Mutex

	//l.Infof("Going to parse rules...")

	//-------------------------------------
	//	Parse the basics
	//-------------------------------------

	//	What to do when both are nil? Documentation is not clear about this...
	//	Let's just return the default action (when everything is empty, k8sfirewall generates a default)
	/*if ingress == nil && egress == nil {
		return parsed
	}*/

	//-------------------------------------
	//	Group pods by their namespace
	//-------------------------------------

	//	To speed things up, we're going to parse Ingress and Egress concurrently.
	//	But how many routines do I have to wait for? (we may only have one and not both...)
	/*if ingress != nil {
		parseLen++
	}
	if egress != nil {
		parseLen++
	}*/
	parseWait.Add(2)

	//-------------------------------------
	//	Parse the ingress rules
	//-------------------------------------

	//if ingress != nil {
	go func() {
		defer parseWait.Done()
		result := d.ParseIngress(ingress, currentNamespace)

		lock.Lock()
		parsed.Ingress = append(parsed.Ingress, result.Ingress...)
		parsed.Egress = append(parsed.Egress, result.Egress...)
		lock.Unlock()
	}()
	//}

	//-------------------------------------
	//	Parse the egress rules
	//-------------------------------------

	//if egress != nil {
	go func() {
		defer parseWait.Done()
		result := d.ParseEgress(egress, currentNamespace)

		lock.Lock()
		parsed.Ingress = append(parsed.Ingress, result.Ingress...)
		parsed.Egress = append(parsed.Egress, result.Egress...)
		lock.Unlock()
	}()
	//}

	//	Wait for them to finish before doing the rest
	parseWait.Wait()

	return parsed
}

func (d *DefaultPolicyParser) ParseIngress(rules []networking_v1.NetworkPolicyIngressRule, namespace string) pcn_types.ParsedRules {

	//-------------------------------------
	//	Init
	//-------------------------------------
	l := log.NewEntry(d.log)
	l.WithFields(log.Fields{"by": DPS, "method": "ParseIngress"})
	parsed := pcn_types.ParsedRules{
		Ingress: []k8sfirewall.ChainRule{},
		Egress:  []k8sfirewall.ChainRule{},
	}
	direction := "ingress"

	//-------------------------------------
	//	Preliminary checks
	//-------------------------------------

	//	Rules is nil?
	if rules == nil {
		return parsed
	}

	//	No rules?
	if len(rules) < 1 {
		//	Rules is empty: nothing is accepted

		parsed.Ingress = append(parsed.Ingress, k8sfirewall.ChainRule{
			Action: pcn_types.ActionDrop,
		})
		return parsed
	}

	//-------------------------------------
	//	Actual parsing
	//-------------------------------------
	for _, rule := range rules {

		//	The ports and rules generated in this iteration.
		generatedPorts := []pcn_types.ProtoPort{}
		generatedIngressRules := []k8sfirewall.ChainRule{}
		generatedEgressRules := []k8sfirewall.ChainRule{}

		//	Tells if we can go on parsing rules
		proceed := true

		//-------------------------------------
		//	Protocol & Port
		//-------------------------------------

		//	First, parse the protocol: so that if an unsupported protocol is listed, we silently ignore it.
		//	By doing it this way we don't have to remove rules later on
		if len(rule.Ports) > 0 {
			generatedPorts = d.ParsePorts(rule.Ports)

			//	If this rule consists of only unsupported protocols, then we can't go on!
			//	If we did, we would be creating wrong rules!
			//	We just need to ignore the rules, for now.
			//	But if there is at least one supported protocol, then we can proceed
			if len(generatedPorts) == 0 {
				proceed = false
			}
		}

		//-------------------------------------
		//	Peers
		//-------------------------------------

		//	From is {} ?
		if rule.From == nil && proceed {
			//	From is nil: ALL resources are allowed
			result := d.GetConnectionTemplate(direction, "", "", pcn_types.ActionForward, []pcn_types.ProtoPort{})
			generatedIngressRules = append(generatedIngressRules, result.Ingress...)
			generatedEgressRules = append(generatedEgressRules, result.Egress...)
		}

		for i := 0; rule.From != nil && i < len(rule.From) && proceed; i++ {
			from := rule.From[i]

			//-------------------------------------
			//	IPBlock
			//-------------------------------------
			if from.IPBlock != nil {
				ipblock := d.ParseIPBlock(from.IPBlock, direction)
				generatedIngressRules = append(generatedIngressRules, ipblock.Ingress...)
				generatedEgressRules = append(generatedEgressRules, ipblock.Egress...)
			}

			//-------------------------------------
			//	PodSelector And/Or NamespaceSelector
			//-------------------------------------
			if from.PodSelector != nil || from.NamespaceSelector != nil {
				rulesGot, err := d.ParseSelectors(from.PodSelector, from.NamespaceSelector, namespace, direction)

				if err == nil {
					generatedIngressRules = append(generatedIngressRules, rulesGot.Ingress...)
					generatedEgressRules = append(generatedEgressRules, rulesGot.Egress...)
				}
			}
		}

		//-------------------------------------
		//	Finalize
		//-------------------------------------

		//	No rules are going to be generated if proceed is false. So, this will return empty arrays in that case
		rulesWithPorts := d.insertPorts(generatedIngressRules, generatedEgressRules, generatedPorts)
		parsed.Ingress = append(parsed.Ingress, rulesWithPorts.Ingress...)
		parsed.Egress = append(parsed.Egress, rulesWithPorts.Egress...)

	}

	return parsed
}

func (d *DefaultPolicyParser) ParseEgress(rules []networking_v1.NetworkPolicyEgressRule, namespace string) pcn_types.ParsedRules {

	//-------------------------------------
	//	Init
	//-------------------------------------
	l := log.NewEntry(d.log)
	l.WithFields(log.Fields{"by": DPS, "method": "ParseEgress"})
	parsed := pcn_types.ParsedRules{
		Ingress: []k8sfirewall.ChainRule{},
		Egress:  []k8sfirewall.ChainRule{},
	}
	direction := "egress"

	//-------------------------------------
	//	Preliminary checks
	//-------------------------------------

	//	Rules is nil?
	if rules == nil {
		return parsed
	}

	//	No rules?
	if len(rules) < 1 {
		//	Rules is empty: nothing is accepted
		parsed.Egress = append(parsed.Egress, k8sfirewall.ChainRule{
			Action: pcn_types.ActionDrop,
		})
		return parsed
	}

	//-------------------------------------
	//	Actual parsing
	//-------------------------------------

	for _, rule := range rules {

		//	The ports and rules generated in this iteration.
		generatedPorts := []pcn_types.ProtoPort{}
		generatedIngressRules := []k8sfirewall.ChainRule{}
		generatedEgressRules := []k8sfirewall.ChainRule{}

		//	Tells if we can go on parsing rules
		proceed := true

		//-------------------------------------
		//	Protocol & Port
		//-------------------------------------

		//	First, parse the protocol: so that if an unsupported protocol is listed, we silently ignore it.
		//	By doing it this way we don't have to remove rules later on
		if len(rule.Ports) > 0 {
			generatedPorts = d.ParsePorts(rule.Ports)

			//	If this rule consists of only unsupported protocols, then we can't go on!
			//	If we did, we would be creating wrong rules!
			//	We just need to ignore the rules, for now.
			//	But if there is at least one supported protocol, then we can proceed
			if len(generatedPorts) == 0 {
				proceed = false
			}
		}

		//-------------------------------------
		//	Peers
		//-------------------------------------

		//	To is {} ?
		if rule.To == nil && proceed {
			result := d.GetConnectionTemplate(direction, "", "", pcn_types.ActionForward, []pcn_types.ProtoPort{})
			generatedIngressRules = append(generatedIngressRules, result.Ingress...)
			generatedEgressRules = append(generatedEgressRules, result.Egress...)
		}

		for i := 0; rule.To != nil && i < len(rule.To) && proceed; i++ {
			to := rule.To[i]

			//	IPBlock?
			if to.IPBlock != nil {
				ipblock := d.ParseIPBlock(to.IPBlock, direction)
				generatedIngressRules = append(generatedIngressRules, ipblock.Ingress...)
				generatedEgressRules = append(generatedEgressRules, ipblock.Egress...)
			}

			//	PodSelector Or NamespaceSelector?
			if to.PodSelector != nil || to.NamespaceSelector != nil {
				rulesGot, err := d.ParseSelectors(to.PodSelector, to.NamespaceSelector, namespace, direction)

				if err == nil {
					if len(rulesGot.Ingress) > 0 {
						generatedIngressRules = append(generatedIngressRules, rulesGot.Ingress...)
					}
					if len(rulesGot.Egress) > 0 {
						generatedEgressRules = append(generatedEgressRules, rulesGot.Egress...)
					}
				} else {
					l.Errorln("Error while parsing selectors:", err)
				}
			}
		}

		//-------------------------------------
		//	Finalize
		//-------------------------------------

		rulesWithPorts := d.insertPorts(generatedIngressRules, generatedEgressRules, generatedPorts)
		parsed.Ingress = append(parsed.Ingress, rulesWithPorts.Ingress...)
		parsed.Egress = append(parsed.Egress, rulesWithPorts.Egress...)
	}

	return parsed
}

func (d *DefaultPolicyParser) insertPorts(generatedIngressRules, generatedEgressRules []k8sfirewall.ChainRule, generatedPorts []pcn_types.ProtoPort) pcn_types.ParsedRules {

	//	Don't make me go through this if there is no ports
	if len(generatedPorts) < 1 {
		return pcn_types.ParsedRules{
			Ingress: generatedIngressRules,
			Egress:  generatedEgressRules,
		}
	}

	parsed := pcn_types.ParsedRules{
		Ingress: []k8sfirewall.ChainRule{},
		Egress:  []k8sfirewall.ChainRule{},
	}

	var waitForChains sync.WaitGroup
	waitForChains.Add(2)

	go func() {
		defer waitForChains.Done()
		//	Finally, for each parsed rule, apply the ports that have been found
		//	But only if you have at least one port
		for i := 0; i < len(generatedIngressRules); i++ {
			rule := generatedIngressRules[i]
			for _, generatedPort := range generatedPorts {
				edited := rule
				edited.Dport = generatedPort.Port
				edited.L4proto = generatedPort.Protocol
				parsed.Ingress = append(parsed.Ingress, edited)
			}
		}
	}()

	go func() {
		defer waitForChains.Done()
		for i := 0; i < len(generatedEgressRules); i++ {
			rule := generatedEgressRules[i]
			for _, generatedPort := range generatedPorts {
				edited := rule
				edited.Sport = generatedPort.Port
				edited.L4proto = generatedPort.Protocol
				parsed.Egress = append(parsed.Egress, edited)
			}
		}
	}()
	waitForChains.Wait()

	return parsed
}

func (d *DefaultPolicyParser) ParsePolicyTypes(policySpec *networking_v1.NetworkPolicySpec) ([]networking_v1.NetworkPolicyIngressRule, []networking_v1.NetworkPolicyEgressRule, string) {

	var ingress []networking_v1.NetworkPolicyIngressRule
	var egress []networking_v1.NetworkPolicyEgressRule

	ingress = nil
	egress = nil
	policyType := "*"

	//	What if spec is not even there?
	if policySpec == nil {
		return nil, nil, "ingress"
	}

	//	Documentation is not very specific about the possibility of PolicyTypes being [], so I made this dumb piece of code just in case
	if policySpec.PolicyTypes == nil {
		ingress = policySpec.Ingress
		policyType = "ingress"
	} else {
		if len(policySpec.PolicyTypes) < 1 {
			ingress = policySpec.Ingress
			policyType = "ingress"
		} else {
			policyTypes := policySpec.PolicyTypes

			for _, val := range policyTypes {
				//	Can't use if-else because user may disable validation and insert
				//	trash values
				if val == networking_v1.PolicyTypeIngress {
					ingress = policySpec.Ingress
					policyType = "ingress"
				}
				if val == networking_v1.PolicyTypeEgress {
					egress = policySpec.Egress
					policyType = "egress"
				}
			}

			if ingress != nil && egress != nil {
				policyType = "*"
			}
		}
	}

	return ingress, egress, policyType
}

func (d *DefaultPolicyParser) ParseIPBlock(block *networking_v1.IPBlock, k8sDirection string) pcn_types.ParsedRules {

	parsed := pcn_types.ParsedRules{
		Ingress: []k8sfirewall.ChainRule{},
		Egress:  []k8sfirewall.ChainRule{},
	}

	//	Actually, these two cannot happen with kubernetes
	if block == nil {
		return parsed
	}

	if len(block.CIDR) < 1 {
		return parsed
	}

	//	Add the default one
	cidrRules := pcn_types.ParsedRules{}
	if k8sDirection == "ingress" {
		cidrRules = d.GetConnectionTemplate(k8sDirection, block.CIDR, "", pcn_types.ActionForward, []pcn_types.ProtoPort{})
	} else {
		cidrRules = d.GetConnectionTemplate(k8sDirection, "", block.CIDR, pcn_types.ActionForward, []pcn_types.ProtoPort{})
	}

	parsed.Ingress = append(parsed.Ingress, cidrRules.Ingress...)
	parsed.Egress = append(parsed.Egress, cidrRules.Egress...)

	//	Loop through all exceptions
	for _, exception := range block.Except {
		exceptionRule := k8sfirewall.ChainRule{
			Action: pcn_types.ActionDrop,
		}

		if k8sDirection == "ingress" {
			exceptionRule.Src = exception
			parsed.Ingress = append(parsed.Ingress, exceptionRule)
		} else {
			exceptionRule.Dst = exception
			parsed.Egress = append(parsed.Egress, exceptionRule)
		}
	}

	return parsed
}

func (d *DefaultPolicyParser) ParsePorts(ports []networking_v1.NetworkPolicyPort) []pcn_types.ProtoPort {

	//	Init
	generatedPorts := []pcn_types.ProtoPort{}

	for _, port := range ports {

		//	If protocol is nil, then we have to get all protocols
		if port.Protocol == nil {

			//	If the port is not nil, default port is not 0
			var defaultPort int32
			if port.Port != nil {
				defaultPort = int32(port.Port.IntValue())
			}

			generatedPorts = append(generatedPorts, pcn_types.ProtoPort{
				Port: defaultPort,
			})

		} else {
			//	else parse the protocol
			supported, proto, port := d.parseProtocolAndPort(port)

			//	Our firewall does not support SCTP, so we check if protocol is supported
			if supported {
				generatedPorts = append(generatedPorts, pcn_types.ProtoPort{
					Protocol: proto,
					Port:     port,
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

func (d *DefaultPolicyParser) ParseSelectors(podSelector, namespaceSelector *meta_v1.LabelSelector, namespace, direction string) (pcn_types.ParsedRules, error) {

	//	init
	rules := pcn_types.ParsedRules{
		Ingress: []k8sfirewall.ChainRule{},
		Egress:  []k8sfirewall.ChainRule{},
	}

	//	First build the query
	podQuery, nsQuery, err := d.buildPodQueries(podSelector, namespaceSelector, namespace)
	if err != nil {
		return rules, err
	}

	//	Now get the pods
	podsFound, err := d.podController.GetPods(podQuery, nsQuery)
	if err != nil {
		return rules, fmt.Errorf("Error while trying to get pods %s", err.Error())
	}

	//	Now build the pods
	for _, pod := range podsFound {
		parsed := pcn_types.ParsedRules{}
		if direction == "ingress" {
			parsed = d.GetConnectionTemplate(direction, pod.Status.PodIP, "", pcn_types.ActionForward, []pcn_types.ProtoPort{})
		} else {
			parsed = d.GetConnectionTemplate(direction, "", pod.Status.PodIP, pcn_types.ActionForward, []pcn_types.ProtoPort{})
		}

		parsed.Ingress = append(parsed.Ingress, parsed.Ingress...)
		parsed.Egress = append(parsed.Egress, parsed.Egress...)
	}

	return rules, nil
}

func (d *DefaultPolicyParser) buildPodQueries(podSelector, namespaceSelector *meta_v1.LabelSelector, namespace string) (pcn_types.ObjectQuery, pcn_types.ObjectQuery, error) {

	//	Init
	queryPod := pcn_types.ObjectQuery{}
	queryNs := pcn_types.ObjectQuery{}

	//	If podSelector is nil: select everything and then block
	//	If podSelector is empty (len = 0): select everything and then forward
	//	NOTE: blocking everything is not the same as setting a default rule to block anything!
	//	Because that way we would also be preventing external connections from accessing our pods.
	//	Instead, we need to block all pods individually, so we can't solve it by just creating a default rule:
	//	we cannot know if user will deploy a policy to allow ipblocks in advance.

	//	Build the query
	if podSelector != nil {

		//	This is not supported yet...
		if podSelector.MatchExpressions != nil {
			return queryPod, queryNs, errors.New("MatchExpressions on pod selector is not supported yet")
		}

		//	Empty labels means "select everything"
		//	Nil labels means do not select anything. Which, for us, means deny access to those pods (see below)
		if len(podSelector.MatchLabels) < 1 {
			queryPod = pcn_types.ObjectQuery{
				By:   "name",
				Name: "*",
			}
		} else {
			queryPod = pcn_types.ObjectQuery{
				By:     "labels",
				Labels: podSelector.MatchLabels,
			}
		}
	} else {
		queryPod = pcn_types.ObjectQuery{
			By:   "name",
			Name: "*",
		}
	}

	//	Namespace selector
	if namespaceSelector != nil {

		//	Match expressions?
		if namespaceSelector.MatchExpressions != nil {
			//	This is not supported yet...
			return queryPod, queryNs, errors.New("MatchExpressions on namespace selector is not supported yet")
		}

		if len(namespaceSelector.MatchLabels) > 0 {
			//	Parse the match labels (like for the pod)
			queryNs = pcn_types.ObjectQuery{
				By:     "labels",
				Labels: namespaceSelector.MatchLabels,
			}
		} else {
			//	No labels: as per documentation, this means ALL namespaces
			queryNs = pcn_types.ObjectQuery{
				By:   "name",
				Name: "*",
			}
		}
	} else {
		//	If namespace selector is nil, we're going to use the one we found on the policy
		queryNs = pcn_types.ObjectQuery{
			By:   "name",
			Name: namespace,
		}
	}

	return queryPod, queryNs, nil
}

//	TO-BE-REMOVED
func (d *DefaultPolicyParser) getPodsFromSelectors(podSelector, namespaceSelector *meta_v1.LabelSelector, namespace string) ([]core_v1.Pod, error) {

	//	Init
	l := log.NewEntry(d.log)
	l.WithFields(log.Fields{"by": DPS, "method": "getPodsFromSelectors()"})
	podsFound := []core_v1.Pod{}
	queryPod := pcn_types.ObjectQuery{}
	queryNs := pcn_types.ObjectQuery{}

	//	If podSelector is nil: select everything and then block
	//	If podSelector is empty (len = 0): select everything and then forward
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
		if len(podSelector.MatchLabels) < 1 {
			queryPod = pcn_types.ObjectQuery{
				By:   "name",
				Name: "*",
			}
		} else {
			queryPod = pcn_types.ObjectQuery{
				By:     "labels",
				Labels: podSelector.MatchLabels,
			}
		}
	} else {
		queryPod = pcn_types.ObjectQuery{
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
			queryNs = pcn_types.ObjectQuery{
				By:     "labels",
				Labels: namespaceSelector.MatchLabels,
			}
		} else {
			//	No labels: as per documentation, this means ALL namespaces
			queryNs = pcn_types.ObjectQuery{
				By:   "name",
				Name: "*",
			}
		}
	} else {
		//	If namespace selector is nil, we're going to use the one we found on the policy
		queryNs = pcn_types.ObjectQuery{
			By:   "name",
			Name: namespace,
		}
	}

	//	Now get the pods
	podsFound, err := d.podController.GetPods(queryPod, queryNs)

	if err != nil {
		l.Errorln("error when trying to get pods:", err)
		return nil, errors.New("error when trying to get pods")
	}

	return podsFound, nil
}

//	TO-BE-REMOVED
func (d *DefaultPolicyParser) GetPodsAffected(policy *networking_v1.NetworkPolicy) (map[string][]core_v1.Pod, error) {
	/*var l = log.WithFields(log.Fields{
		"by":     DPS,
		"method": "GetPodsAffected()",
	})*/

	//	Get the namespace (for us, '*' means all namespaces)
	namespace := "*"
	if len(policy.ObjectMeta.Namespace) > 0 {
		namespace = policy.Namespace
	}

	var namespacesGroup map[string][]core_v1.Pod

	//	Get the pods this policy must be applied to
	podsAffected, err := d.getPodsFromSelectors(&policy.Spec.PodSelector, nil, namespace)

	//	Something happened?
	if err != nil {
		return nil, err
	}

	//	No pods found?
	if len(podsAffected) < 1 {
		return namespacesGroup, nil
	}

	//-------------------------------------
	//	Group pods by their namespace
	//-------------------------------------

	namespacesGroup = make(map[string][]core_v1.Pod)

	//	This is important! Some rules specify that the restriction must be applied to the namespace they BELONG, not every single one.
	//	Example: IPBlock rules don't consider namespaces, but PodSelector (and/or NamespaceSelector) may restrict access based on the
	//	namespace that particular pod is in (i.e.: only allowing pods from the same namespace they are found).
	//	But unfortunately, we don't know in advance if the policy only consists of IPBlock-s without parsing it first.
	//	So, we need to group our found pods by their namespace, in order to do a correct parsing.
	for _, pod := range podsAffected {

		if _, ok := namespacesGroup[pod.Namespace]; !ok {
			namespacesGroup[pod.Namespace] = []core_v1.Pod{}
		}

		namespacesGroup[pod.Namespace] = append(namespacesGroup[pod.Namespace], pod)
	}

	return namespacesGroup, nil
}

//	TO-BE-REMOVED
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
		if currentPod.UID == pod.UID {
			return true
		}
	}

	return false
}

//	TO-BE-REMOVED
func (d *DefaultPolicyParser) DoesPolicyTargetPod(policy *networking_v1.NetworkPolicy, pod *core_v1.Pod) pcn_types.ParsedRules {

	//	Get the spec, with the ingress & egress rules
	spec := policy.Spec
	ingress, egress, _ := d.ParsePolicyTypes(&spec)

	if ingress == nil && egress == nil {
		return pcn_types.ParsedRules{
			Ingress: []k8sfirewall.ChainRule{},
			Egress:  []k8sfirewall.ChainRule{},
		}
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

	ingressRules := []k8sfirewall.ChainRule{}
	egressRules := []k8sfirewall.ChainRule{}

	var iLock sync.Mutex
	var eLock sync.Mutex

	appendRules := func(generatedIngress, generatedEgress []k8sfirewall.ChainRule) {
		if len(generatedIngress) > 0 {
			iLock.Lock()
			ingressRules = append(ingressRules, generatedIngress...)
			iLock.Unlock()
		}
		if len(generatedEgress) > 0 {
			eLock.Lock()
			egressRules = append(egressRules, generatedEgress...)
			eLock.Unlock()
		}
	}

	namespace := "*"
	if len(policy.ObjectMeta.Namespace) > 0 {
		namespace = policy.Namespace
	}

	//-------------------------------------
	//	Check for ingress
	//-------------------------------------

	if ingress != nil && len(ingress) > 0 {
		go func() {
			defer parseWait.Done()
			for _, rule := range ingress {
				generatedPorts := []pcn_types.ProtoPort{}
				proceed := true

				//	Let's first get the ports, so that I can see if there are unsupported protocols
				if rule.Ports != nil {
					if len(rule.Ports) > 0 {
						generatedPorts = d.ParsePorts(rule.Ports)

						if len(generatedPorts) == 0 {
							proceed = false
						}
					}
				}

				//	Now instead let's see if this pod is listed
				for i := 0; i < len(rule.From) && proceed; i++ {
					from := rule.From[i]
					if from.PodSelector != nil || from.NamespaceSelector != nil {
						podsFound, err := d.getPodsFromSelectors(from.PodSelector, from.NamespaceSelector, namespace)

						if err == nil {
							parsed := d.generateRulesForPod(podsFound, pod, generatedPorts, "ingress")
							appendRules(parsed.Ingress, parsed.Egress)
						}
					}
				}
			}
		}()
	}

	//-------------------------------------
	//	Check for egress
	//-------------------------------------

	if egress != nil && len(egress) > 0 {
		go func() {
			defer parseWait.Done()
			for _, rule := range egress {
				generatedPorts := []pcn_types.ProtoPort{}
				proceed := true

				if rule.Ports != nil {
					if len(rule.Ports) > 0 {
						generatedPorts = d.ParsePorts(rule.Ports)

						if len(generatedPorts) == 0 {
							proceed = false
						}
					}
				}

				for i := 0; i < len(rule.To) && proceed; i++ {
					to := rule.To[i]
					if to.PodSelector != nil || to.NamespaceSelector != nil {
						podsFound, err := d.getPodsFromSelectors(to.PodSelector, to.NamespaceSelector, namespace)

						if err == nil {
							parsed := d.generateRulesForPod(podsFound, pod, generatedPorts, "egress")
							appendRules(parsed.Ingress, parsed.Egress)
						}
					}
				}
			}
		}()
	}

	//	Wait for them to finish before doing the rest
	parseWait.Wait()

	return pcn_types.ParsedRules{
		Ingress: ingressRules,
		Egress:  egressRules,
	}
}

func (d *DefaultPolicyParser) GetConnectionTemplate(direction, src, dst, action string, ports []pcn_types.ProtoPort) pcn_types.ParsedRules {

	twoRules := make([]k8sfirewall.ChainRule, 2)
	oneRule := make([]k8sfirewall.ChainRule, 1)

	twoRules[0] = k8sfirewall.ChainRule{
		Src:       src,
		Dst:       dst,
		Action:    action,
		Conntrack: pcn_types.ConnTrackNew,
	}
	twoRules[1] = k8sfirewall.ChainRule{
		Src:       src,
		Dst:       dst,
		Action:    action,
		Conntrack: pcn_types.ConnTrackEstablished,
	}
	oneRule[0] = k8sfirewall.ChainRule{
		Src:       dst,
		Dst:       src,
		Action:    action,
		Conntrack: pcn_types.ConnTrackEstablished,
	}

	if direction == "ingress" {

		if len(ports) > 0 {
			withPorts := d.insertPorts(twoRules, oneRule, ports)
			twoRules, oneRule = withPorts.Ingress, withPorts.Egress
		}

		return pcn_types.ParsedRules{
			Ingress: twoRules,
			Egress:  oneRule,
		}
	}

	if len(ports) > 0 {
		withPorts := d.insertPorts(oneRule, twoRules, ports)
		oneRule, twoRules = withPorts.Ingress, withPorts.Egress
	}

	return pcn_types.ParsedRules{
		Ingress: oneRule,
		Egress:  twoRules,
	}

}

func (d *DefaultPolicyParser) GetClusterActions(ingress []networking_v1.NetworkPolicyIngressRule, egress []networking_v1.NetworkPolicyEgressRule, currentNamespace string) ([]pcn_types.FirewallAction, []pcn_types.FirewallAction) {
	ingressActions := []pcn_types.FirewallAction{}
	egressActions := []pcn_types.FirewallAction{}
	var waitActions sync.WaitGroup
	waitActions.Add(2)

	//	Ingress
	go func() {
		defer waitActions.Done()
		if ingress == nil {
			return
		}

		for _, i := range ingress {

			ports := d.ParsePorts(i.Ports)

			for _, f := range i.From {
				action := pcn_types.FirewallAction{}
				ok := true

				//	Matchexpression is not supported
				if (f.PodSelector != nil && f.PodSelector.MatchExpressions != nil && len(f.PodSelector.MatchExpressions) > 0) ||
					(f.NamespaceSelector != nil && f.NamespaceSelector.MatchExpressions != nil && len(f.NamespaceSelector.MatchExpressions) > 0) {
					ok = false
				}

				//	If no selectors, then don't do anything
				if f.PodSelector == nil && f.NamespaceSelector == nil {
					ok = false
				}

				if ok {
					if f.PodSelector != nil && f.PodSelector.MatchLabels != nil {
						action.PodLabels = f.PodSelector.MatchLabels
					}

					if f.NamespaceSelector != nil && f.NamespaceSelector.MatchLabels != nil {
						action.NamespaceLabels = f.NamespaceSelector.MatchLabels
					} else {
						action.NamespaceName = currentNamespace
					}

					action.Ports = ports
					action.Action = pcn_types.Forward
					ingressActions = append(ingressActions, action)
				}
			}
		}
	}()

	//	Egress
	go func() {
		defer waitActions.Done()
		if egress == nil {
			return
		}

		for _, e := range egress {

			ports := d.ParsePorts(e.Ports)

			for _, t := range e.To {

				action := pcn_types.FirewallAction{}
				ok := true

				if (t.PodSelector.MatchExpressions != nil && len(t.PodSelector.MatchExpressions) > 0) ||
					(t.NamespaceSelector.MatchExpressions != nil && len(t.NamespaceSelector.MatchExpressions) > 0) {
					ok = false
				}

				//	If no selectors, then don't do anything
				if t.PodSelector == nil && t.NamespaceSelector == nil {
					ok = false
				}

				if ok {
					if t.PodSelector != nil && t.PodSelector.MatchLabels != nil {
						action.PodLabels = t.PodSelector.MatchLabels
					}

					if t.NamespaceSelector != nil && t.NamespaceSelector.MatchLabels != nil {
						action.NamespaceLabels = t.NamespaceSelector.MatchLabels
					}

					action.Ports = ports
					action.Action = pcn_types.Forward
					egressActions = append(egressActions, action)
				}
			}
		}
	}()

	waitActions.Wait()

	return ingressActions, egressActions
}

func (d *DefaultPolicyParser) generateRulesForPod(podsFound []core_v1.Pod, pod *core_v1.Pod, generatedPorts []pcn_types.ProtoPort, direction string) pcn_types.ParsedRules {
	generatedIngress := []k8sfirewall.ChainRule{}
	generatedEgress := []k8sfirewall.ChainRule{}

	for j := 0; j < len(podsFound); j++ {
		podFound := podsFound[j]
		if podFound.UID == pod.UID {
			for _, generatedPort := range generatedPorts {
				if direction == "ingress" {
					generatedIngress = append(generatedIngress, k8sfirewall.ChainRule{
						Src:       pod.Status.PodIP,
						L4proto:   generatedPort.Protocol,
						Dport:     generatedPort.Port,
						Action:    pcn_types.ActionForward,
						Conntrack: pcn_types.ConnTrackNew,
					})
					generatedIngress = append(generatedIngress, k8sfirewall.ChainRule{
						Src:       pod.Status.PodIP,
						L4proto:   generatedPort.Protocol,
						Dport:     generatedPort.Port,
						Action:    pcn_types.ActionForward,
						Conntrack: pcn_types.ConnTrackEstablished,
					})
					generatedEgress = append(generatedEgress, k8sfirewall.ChainRule{
						Dst:       pod.Status.PodIP,
						L4proto:   generatedPort.Protocol,
						Sport:     generatedPort.Port,
						Action:    pcn_types.ActionForward,
						Conntrack: pcn_types.ConnTrackEstablished,
					})
				}
				if direction == "egress" {
					generatedEgress = append(generatedEgress, k8sfirewall.ChainRule{
						Dst:       pod.Status.PodIP,
						L4proto:   generatedPort.Protocol,
						Sport:     generatedPort.Port,
						Action:    pcn_types.ActionForward,
						Conntrack: pcn_types.ConnTrackNew,
					})
					generatedEgress = append(generatedEgress, k8sfirewall.ChainRule{
						Dst:       pod.Status.PodIP,
						L4proto:   generatedPort.Protocol,
						Sport:     generatedPort.Port,
						Action:    pcn_types.ActionForward,
						Conntrack: pcn_types.ConnTrackEstablished,
					})
					generatedIngress = append(generatedIngress, k8sfirewall.ChainRule{
						Src:       pod.Status.PodIP,
						L4proto:   generatedPort.Protocol,
						Dport:     generatedPort.Port,
						Action:    pcn_types.ActionForward,
						Conntrack: pcn_types.ConnTrackEstablished,
					})
				}
			}
			if len(generatedPorts) < 1 {
				if direction == "ingress" {
					generatedIngress = append(generatedIngress, k8sfirewall.ChainRule{
						Src:       pod.Status.PodIP,
						Action:    pcn_types.ActionForward,
						Conntrack: pcn_types.ConnTrackNew,
					})
					generatedIngress = append(generatedIngress, k8sfirewall.ChainRule{
						Src:       pod.Status.PodIP,
						Action:    pcn_types.ActionForward,
						Conntrack: pcn_types.ConnTrackEstablished,
					})
					generatedEgress = append(generatedEgress, k8sfirewall.ChainRule{
						Dst:       pod.Status.PodIP,
						Action:    pcn_types.ActionForward,
						Conntrack: pcn_types.ConnTrackEstablished,
					})
				}
				if direction == "egress" {
					generatedEgress = append(generatedEgress, k8sfirewall.ChainRule{
						Dst:       pod.Status.PodIP,
						Action:    pcn_types.ActionForward,
						Conntrack: pcn_types.ConnTrackNew,
					})
					generatedEgress = append(generatedEgress, k8sfirewall.ChainRule{
						Dst:       pod.Status.PodIP,
						Action:    pcn_types.ActionForward,
						Conntrack: pcn_types.ConnTrackEstablished,
					})
					generatedIngress = append(generatedIngress, k8sfirewall.ChainRule{
						Src:       pod.Status.PodIP,
						Action:    pcn_types.ActionForward,
						Conntrack: pcn_types.ConnTrackEstablished,
					})
				}
			}
		}
	}

	return pcn_types.ParsedRules{
		Ingress: generatedIngress,
		Egress:  generatedEgress,
	}
}
