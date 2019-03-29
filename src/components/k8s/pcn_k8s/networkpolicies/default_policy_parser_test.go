package networkpolicies

import (
	"fmt"
	"testing"

	pcn_types "github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/types"
	k8sfirewall "github.com/SunSince90/polycube/src/components/k8s/utils/k8sfirewall"

	networking_v1 "k8s.io/api/networking/v1"

	pcn_controllers "github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/controllers"
	"github.com/stretchr/testify/assert"
	core_v1 "k8s.io/api/core/v1"
	meta_v1 "k8s.io/apimachinery/pkg/apis/meta/v1"
	"k8s.io/apimachinery/pkg/util/intstr"
)

func Init(podController pcn_controllers.PodController) *DefaultPolicyParser {
	manager := newDefaultPolicyParser(podController, nil, "node")
	return manager
}

func TestNotNil(t *testing.T) {
	/*manager := Init(nil)
	assert.NotNil(t, manager)*/
}

func TestPolicyTypes(t *testing.T) {
	testObj := new(MockPodController)
	parser := Init(testObj)

	//	Case 1: spec is nil
	ingress, egress, ptype := parser.ParsePolicyTypes(nil)
	assert.Empty(t, ingress)
	assert.Empty(t, egress)
	assert.Equal(t, "ingress", ptype)

	//	Case 2: only ingress
	spec := &networking_v1.NetworkPolicySpec{
		PolicyTypes: []networking_v1.PolicyType{
			networking_v1.PolicyTypeIngress,
		},
	}
	ingress, egress, ptype = parser.ParsePolicyTypes(spec)
	assert.Empty(t, ingress)
	assert.Empty(t, egress)
	assert.Equal(t, "ingress", ptype)

	//	Case 2: only ingress
	spec = &networking_v1.NetworkPolicySpec{
		PolicyTypes: []networking_v1.PolicyType{
			networking_v1.PolicyTypeEgress,
		},
		Egress: []networking_v1.NetworkPolicyEgressRule{
			networking_v1.NetworkPolicyEgressRule{
				To: []networking_v1.NetworkPolicyPeer{},
			},
		},
	}
	ingress, egress, ptype = parser.ParsePolicyTypes(spec)
	assert.Empty(t, ingress)
	assert.NotEmpty(t, egress)
	assert.Equal(t, "egress", ptype)

	//	Case 3: both
	spec = &networking_v1.NetworkPolicySpec{
		PolicyTypes: []networking_v1.PolicyType{
			networking_v1.PolicyTypeEgress,
			networking_v1.PolicyTypeIngress,
		},
		Egress: []networking_v1.NetworkPolicyEgressRule{
			networking_v1.NetworkPolicyEgressRule{
				To: []networking_v1.NetworkPolicyPeer{},
			},
		},
		Ingress: []networking_v1.NetworkPolicyIngressRule{
			networking_v1.NetworkPolicyIngressRule{
				From: []networking_v1.NetworkPolicyPeer{},
			},
		},
	}
	ingress, egress, ptype = parser.ParsePolicyTypes(spec)
	assert.NotEmpty(t, ingress)
	assert.NotEmpty(t, egress)
	assert.Equal(t, "*", ptype)
}

func TestRulesIsNil(t *testing.T) {
	testObj := new(MockPodController)
	parser := Init(testObj)

	//	Case 1
	result := parser.ParseIngress(nil, "*")
	assert.Empty(t, result.Ingress)
	assert.Empty(t, result.Egress)

	//	Case 2
	result = parser.ParseEgress(nil, "*")
	assert.Empty(t, result.Ingress)
	assert.Empty(t, result.Egress)
}

func TestRulesLenZero(t *testing.T) {
	testObj := new(MockPodController)
	parser := Init(testObj)

	//	Case 1
	expectedIngress := []k8sfirewall.ChainRule{
		k8sfirewall.ChainRule{
			Action: pcn_types.ActionDrop,
		},
	}
	result := parser.ParseIngress([]networking_v1.NetworkPolicyIngressRule{}, "ns")
	assert.ElementsMatch(t, expectedIngress, result.Ingress)
	assert.Empty(t, result.Egress)

	//	Case 2
	expectedEgress := []k8sfirewall.ChainRule{
		k8sfirewall.ChainRule{
			Action: pcn_types.ActionDrop,
		},
	}
	result = parser.ParseEgress([]networking_v1.NetworkPolicyEgressRule{}, "ns")
	assert.ElementsMatch(t, expectedEgress, result.Egress)
	assert.Empty(t, result.Ingress)
}

func TestGetConnectionTemplate(t *testing.T) {

	testObj := new(MockPodController)
	parser := Init(testObj)

	src := "10.0.0.1"
	dst := "10.0.0.2"
	action := pcn_types.ActionForward
	direction := "ingress"
	ports := []pcn_types.ProtoPort{}

	//	Case 1
	expectedIngress := []k8sfirewall.ChainRule{
		k8sfirewall.ChainRule{
			Src:       src,
			Dst:       dst,
			Action:    action,
			Conntrack: pcn_types.ConnTrackNew,
		},
		k8sfirewall.ChainRule{
			Src:       src,
			Dst:       dst,
			Action:    action,
			Conntrack: pcn_types.ConnTrackEstablished,
		},
	}
	expectedEgress := []k8sfirewall.ChainRule{
		k8sfirewall.ChainRule{
			Src:       dst,
			Dst:       src,
			Action:    action,
			Conntrack: pcn_types.ConnTrackEstablished,
		},
	}

	result := parser.GetConnectionTemplate(direction, src, dst, action, ports)
	assert.ElementsMatch(t, expectedIngress, result.Ingress)
	assert.ElementsMatch(t, expectedEgress, result.Egress)

	for i := 0; i < len(result.Ingress); i++ {
		fmt.Printf("%+v\n", result.Ingress[i])
	}
	fmt.Println("---")
	//	Case 2
	direction = "egress"
	src = "10.0.0.3"
	dst = "10.0.0.4"
	action = pcn_types.ActionDrop

	expectedEgress = []k8sfirewall.ChainRule{
		k8sfirewall.ChainRule{
			Src:       src,
			Dst:       dst,
			Action:    action,
			Conntrack: pcn_types.ConnTrackNew,
		},
		k8sfirewall.ChainRule{
			Src:       src,
			Dst:       dst,
			Action:    action,
			Conntrack: pcn_types.ConnTrackEstablished,
		},
	}
	expectedIngress = []k8sfirewall.ChainRule{
		k8sfirewall.ChainRule{
			Src:       dst,
			Dst:       src,
			Action:    action,
			Conntrack: pcn_types.ConnTrackEstablished,
		},
	}

	result = parser.GetConnectionTemplate(direction, src, dst, action, ports)
	assert.ElementsMatch(t, expectedIngress, result.Ingress)
	assert.ElementsMatch(t, expectedEgress, result.Egress)

	for i := 0; i < len(result.Egress); i++ {
		fmt.Printf("%+v\n", result.Egress[i])
	}
}

func TestParseProtocolAndPort(t *testing.T) {
	stcp := core_v1.ProtocolSCTP
	udp := core_v1.ProtocolUDP
	tcp := core_v1.ProtocolTCP
	port := &intstr.IntOrString{
		IntVal: 6895,
	}
	ports := []networking_v1.NetworkPolicyPort{
		networking_v1.NetworkPolicyPort{
			Protocol: &udp,
			Port:     port,
		},
		networking_v1.NetworkPolicyPort{
			Protocol: &stcp,
			Port:     port,
		},
		networking_v1.NetworkPolicyPort{
			Protocol: &tcp,
			Port:     port,
		},
	}
	testObj := new(MockPodController)
	manager := Init(testObj)

	//	--- UDP
	supported, protocol, pport := manager.parseProtocolAndPort(ports[0])
	assert.Equal(t, "UDP", protocol)
	assert.Equal(t, int32(port.IntVal), pport)
	assert.True(t, supported)

	//	--- TCP
	supported, protocol, pport = manager.parseProtocolAndPort(ports[2])
	assert.Equal(t, "TCP", protocol)
	assert.Equal(t, int32(port.IntVal), pport)
	assert.True(t, supported)

	//	--- STCP
	supported, protocol, pport = manager.parseProtocolAndPort(ports[1])
	assert.Len(t, protocol, 0)
	assert.Zero(t, pport)
	assert.False(t, supported)
}

func TestParsePorts(t *testing.T) {
	testObj := new(MockPodController)
	parser := Init(testObj)

	// Case 1: protocol and port are nil
	///	Not very sure if protocol can be nil, but just in case ...
	expectedResult := []pcn_types.ProtoPort{
		pcn_types.ProtoPort{
			Port: 0,
		},
	}
	ports := []networking_v1.NetworkPolicyPort{
		networking_v1.NetworkPolicyPort{},
	}
	result := parser.ParsePorts(ports)
	assert.ElementsMatch(t, expectedResult, result)

	//	Case 2: port and protocol are not nil and there is an unsupported protocol
	udp := core_v1.ProtocolUDP
	ports[0] = networking_v1.NetworkPolicyPort{
		Port: &intstr.IntOrString{
			IntVal: 6895,
		},
		Protocol: &udp,
	}
	sctp := core_v1.ProtocolSCTP
	ports = append(ports, networking_v1.NetworkPolicyPort{
		Port: &intstr.IntOrString{
			IntVal: 6895,
		},
		Protocol: &sctp,
	})
	expectedResult = []pcn_types.ProtoPort{
		pcn_types.ProtoPort{
			Port:     ports[0].Port.IntVal,
			Protocol: "UDP",
		},
	}
	result = parser.ParsePorts(ports)
	assert.ElementsMatch(t, expectedResult, result)

	//	Case 3: only unsupported protocols
	ports = []networking_v1.NetworkPolicyPort{
		networking_v1.NetworkPolicyPort{
			Port: &intstr.IntOrString{
				IntVal: 6895,
			},
			Protocol: &sctp,
		},
	}
	result = parser.ParsePorts(ports)
	assert.Empty(t, result)

	//	case 3 ports is nil
	result = parser.ParsePorts(nil)
	assert.Empty(t, result)
}

func TestInsertPorts(t *testing.T) {
	testObj := new(MockPodController)
	parser := Init(testObj)

	src := "10.0.0.1"
	dst := "10.0.0.2"
	action := pcn_types.ActionForward

	//	This is tested above
	ingress := []k8sfirewall.ChainRule{
		k8sfirewall.ChainRule{
			Src:    src,
			Dst:    dst,
			Action: action,
		},
	}
	egress := []k8sfirewall.ChainRule{

		k8sfirewall.ChainRule{
			Src:    src,
			Dst:    dst,
			Action: action,
		},
	}

	//	This is tested above
	port1 := int32(1111)
	port2 := int32(2222)
	udp := core_v1.ProtocolUDP
	tcp := core_v1.ProtocolTCP
	ports := []networking_v1.NetworkPolicyPort{
		networking_v1.NetworkPolicyPort{
			Port: &intstr.IntOrString{
				IntVal: port1,
			},
			Protocol: &udp,
		},
		networking_v1.NetworkPolicyPort{
			Port: &intstr.IntOrString{
				IntVal: port2,
			},
			Protocol: &tcp,
		},
	}

	//	Case 1: nil ports
	fullRules := parser.insertPorts(ingress, egress, []pcn_types.ProtoPort{})
	assert.ElementsMatch(t, fullRules.Ingress, ingress)
	assert.ElementsMatch(t, fullRules.Egress, egress)

	//	Case 2: no ports
	fullRules = parser.insertPorts(ingress, egress, []pcn_types.ProtoPort{})
	assert.ElementsMatch(t, fullRules.Ingress, ingress)
	assert.ElementsMatch(t, fullRules.Egress, egress)

	//	Case 3, ports
	parsedPorts := parser.ParsePorts(ports)
	fullRules = parser.insertPorts(ingress, egress, parsedPorts)
	assert.Len(t, fullRules.Ingress, len(ingress)*len(ports))
	assert.Len(t, fullRules.Egress, len(egress)*len(ports))

	expectedIngress := []k8sfirewall.ChainRule{
		k8sfirewall.ChainRule{
			Src:     src,
			Dst:     dst,
			Action:  action,
			Dport:   port1,
			L4proto: "UDP",
		},
		k8sfirewall.ChainRule{
			Src:     src,
			Dst:     dst,
			Action:  action,
			Dport:   port2,
			L4proto: "TCP",
		},
	}
	expectedEgress := []k8sfirewall.ChainRule{
		k8sfirewall.ChainRule{
			Src:     src,
			Dst:     dst,
			Action:  action,
			Sport:   port1,
			L4proto: "UDP",
		},
		k8sfirewall.ChainRule{
			Src:     src,
			Dst:     dst,
			Action:  action,
			Sport:   port2,
			L4proto: "TCP",
		},
	}
	assert.ElementsMatch(t, fullRules.Ingress, expectedIngress)
	assert.ElementsMatch(t, fullRules.Egress, expectedEgress)

}

func TestParseIPBlock(t *testing.T) {
	testObj := new(MockPodController)
	parser := Init(testObj)

	//	case 1: block is nil
	result := parser.ParseIPBlock(nil, "ingress")
	assert.Empty(t, result.Ingress)
	assert.Empty(t, result.Egress)

	//	case 2: block is ""
	block := &networking_v1.IPBlock{
		CIDR: "",
	}
	result = parser.ParseIPBlock(block, "ingress")
	assert.Empty(t, result.Ingress)
	assert.Empty(t, result.Egress)

	//	case 3(ingress): block with exceptions
	block.CIDR = "10.0.0.0/24"
	block.Except = []string{"10.0.0.0/25"}
	expectedIngress := []k8sfirewall.ChainRule{
		k8sfirewall.ChainRule{
			Src:       block.CIDR,
			Action:    pcn_types.ActionForward,
			Conntrack: pcn_types.ConnTrackNew,
		},
		k8sfirewall.ChainRule{
			Src:       block.CIDR,
			Action:    pcn_types.ActionForward,
			Conntrack: pcn_types.ConnTrackEstablished,
		},
		k8sfirewall.ChainRule{
			Src:    block.Except[0],
			Action: pcn_types.ActionDrop,
		},
	}
	expectedEgress := []k8sfirewall.ChainRule{
		k8sfirewall.ChainRule{
			Dst:       block.CIDR,
			Action:    pcn_types.ActionForward,
			Conntrack: pcn_types.ConnTrackEstablished,
		},
	}
	result = parser.ParseIPBlock(block, "ingress")
	assert.ElementsMatch(t, expectedIngress, result.Ingress)
	assert.ElementsMatch(t, expectedEgress, result.Egress)

	//	case 3(egress): block with exceptions
	block.CIDR = "10.0.0.0/24"
	block.Except = []string{"10.0.0.0/25"}
	expectedEgress = []k8sfirewall.ChainRule{
		k8sfirewall.ChainRule{
			Dst:       block.CIDR,
			Action:    pcn_types.ActionForward,
			Conntrack: pcn_types.ConnTrackNew,
		},
		k8sfirewall.ChainRule{
			Dst:       block.CIDR,
			Action:    pcn_types.ActionForward,
			Conntrack: pcn_types.ConnTrackEstablished,
		},
		k8sfirewall.ChainRule{
			Dst:    block.Except[0],
			Action: pcn_types.ActionDrop,
		},
	}
	expectedIngress = []k8sfirewall.ChainRule{
		k8sfirewall.ChainRule{
			Src:       block.CIDR,
			Action:    pcn_types.ActionForward,
			Conntrack: pcn_types.ConnTrackEstablished,
		},
	}
	result = parser.ParseIPBlock(block, "egress")
	assert.ElementsMatch(t, expectedIngress, result.Ingress)
	assert.ElementsMatch(t, expectedEgress, result.Egress)
}

func TestBuildPodQueries(t *testing.T) {
	testObj := new(MockPodController)
	parser := Init(testObj)
	ns := "namespace"
	//	case 1: matchexpressions
	podSelector := &meta_v1.LabelSelector{
		MatchExpressions: []meta_v1.LabelSelectorRequirement{},
	}
	_, _, err := parser.buildPodQueries(podSelector, nil, "")
	assert.Error(t, err)
	nsSelector := &meta_v1.LabelSelector{
		MatchExpressions: []meta_v1.LabelSelectorRequirement{},
	}
	_, _, err = parser.buildPodQueries(nil, nsSelector, "")
	assert.Error(t, err)

	//	case 2: select all pods and all namespaces
	//	a): both nil
	expectedP := pcn_types.ObjectQuery{
		By:   "name",
		Name: "*",
	}
	expectedN := pcn_types.ObjectQuery{
		By:   "name",
		Name: ns,
	}
	p, n, err := parser.buildPodQueries(nil, nil, ns)
	assert.NoError(t, err)
	assert.Equal(t, expectedP.By, p.By)
	assert.Equal(t, expectedP.Name, p.Name)
	assert.Equal(t, expectedN.By, n.By)
	assert.Equal(t, expectedN.Name, n.Name)

	//	b) empty labels
	podSelector = &meta_v1.LabelSelector{}
	nsSelector = &meta_v1.LabelSelector{}
	expectedN.Name = "*"
	podSelector.MatchLabels = map[string]string{}
	nsSelector.MatchLabels = map[string]string{}
	p, n, err = parser.buildPodQueries(podSelector, nsSelector, ns)
	assert.NoError(t, err)
	assert.Equal(t, expectedP.By, p.By)
	assert.Equal(t, expectedP.Name, p.Name)
	assert.Equal(t, expectedN.By, n.By)
	assert.Equal(t, expectedN.Name, n.Name)

	//	Case 3: specific labels for both
	podSelector.MatchLabels = map[string]string{"app": "pcn", "v": "2"}
	nsSelector.MatchLabels = map[string]string{"env": "production"}
	expectedN = pcn_types.ObjectQuery{
		By:     "labels",
		Labels: nsSelector.MatchLabels,
	}
	expectedP = pcn_types.ObjectQuery{
		By:     "labels",
		Labels: podSelector.MatchLabels,
	}
	p, n, err = parser.buildPodQueries(podSelector, nsSelector, ns)
	assert.NoError(t, err)
	assert.Equal(t, expectedP.By, p.By)
	assert.Equal(t, expectedP.Name, p.Name)
	assert.Equal(t, expectedP.Labels, p.Labels)
	assert.Equal(t, expectedN.By, n.By)
	assert.Equal(t, expectedN.Name, n.Name)
	assert.Equal(t, expectedN.Labels, n.Labels)
}

func TestParseSelectors(t *testing.T) {
	//	No need to do anything here: it just makes use of already tested functions.
}

func TestOnlyUnsupportedProtocol(t *testing.T) {
	//	if the policy only consists of unsupported protocol, then it is better
	//	not to generate rules at all, instead of creating wrong rules!
	stcp := core_v1.ProtocolSCTP
	port := &intstr.IntOrString{
		IntVal: 6895,
	}
	ports := []networking_v1.NetworkPolicyPort{
		networking_v1.NetworkPolicyPort{
			Protocol: &stcp,
			Port:     port,
		},
	}
	ipBlock := &networking_v1.IPBlock{
		CIDR: "192.168.0.0/16",
	}
	testObj := new(MockPodController)
	manager := Init(testObj)

	//---------------------------------
	//	Ingress
	//---------------------------------

	ingress := []networking_v1.NetworkPolicyIngressRule{
		networking_v1.NetworkPolicyIngressRule{
			From: []networking_v1.NetworkPolicyPeer{
				networking_v1.NetworkPolicyPeer{
					IPBlock: ipBlock,
				},
			},
			Ports: ports,
		},
	}

	result := manager.ParseIngress(ingress, "ns")
	assert.Empty(t, result.Ingress)
	assert.Empty(t, result.Egress)

	//---------------------------------
	//	Egress
	//---------------------------------

	egress := []networking_v1.NetworkPolicyEgressRule{
		networking_v1.NetworkPolicyEgressRule{
			To: []networking_v1.NetworkPolicyPeer{
				networking_v1.NetworkPolicyPeer{
					IPBlock: ipBlock,
				},
			},
			Ports: ports,
		},
	}

	result = manager.ParseEgress(egress, "ns")
	assert.Empty(t, result.Ingress)
	assert.Empty(t, result.Egress)
}

func TestComplexPolicy(t *testing.T) {
	//---------------------------------
	//	The Policy
	//---------------------------------
	tcp := core_v1.ProtocolTCP
	udp := core_v1.ProtocolUDP
	port1 := &intstr.IntOrString{
		IntVal: 6895,
	}
	port2 := &intstr.IntOrString{
		IntVal: 8080,
	}
	port3 := &intstr.IntOrString{
		IntVal: 80,
	}
	port4 := &intstr.IntOrString{
		IntVal: 5000,
	}
	policy := &networking_v1.NetworkPolicy{
		ObjectMeta: meta_v1.ObjectMeta{
			Name: TestDefaultPolicyName,
			//Namespace: ProductionNamespace,
		},
		Spec: networking_v1.NetworkPolicySpec{
			PodSelector: meta_v1.LabelSelector{
				MatchLabels: LabelsPodsInProduction,
			},
			PolicyTypes: []networking_v1.PolicyType{
				networking_v1.PolicyTypeIngress,
				networking_v1.PolicyTypeEgress,
			},
			Ingress: []networking_v1.NetworkPolicyIngressRule{
				/*
					Ingress - First rule
					Block the following if it comes with TCP6895 Or UDP6895
						- what comes from 192.168.2.0/24

					Allow the following if it comes with TCP6895 & UDP6895
						- what comes from 192.168.0.0/16 (see exceptions above)
						- All pods that belong to all namespaces
				*/
				networking_v1.NetworkPolicyIngressRule{
					From: []networking_v1.NetworkPolicyPeer{
						networking_v1.NetworkPolicyPeer{
							IPBlock: &networking_v1.IPBlock{
								CIDR: "192.168.0.0/16",
								Except: []string{
									"192.168.2.0/24",
								},
							},
							PodSelector: &meta_v1.LabelSelector{
								MatchLabels: map[string]string{},
							},
							NamespaceSelector: &meta_v1.LabelSelector{
								MatchLabels: map[string]string{},
							},
						},
					},
					Ports: []networking_v1.NetworkPolicyPort{
						networking_v1.NetworkPolicyPort{
							Protocol: &tcp,
							Port:     port1,
						},
						networking_v1.NetworkPolicyPort{
							Protocol: &udp,
							Port:     port1,
						},
					},
				},
			},
			Egress: []networking_v1.NetworkPolicyEgressRule{
				/*
					Egress - First Rule
					Block the following if it comes with TCP8080 Or TCP80:
						- what comes from 192.169.2.0/24

					Allow the following if it comes with TCP8080 Or TCP80:
						- what comes from 192.169.0.0/16 (see exceptions above)
						- all pods with labels podLabels that belong to certain namespace (namespaceLabels)
				*/
				networking_v1.NetworkPolicyEgressRule{
					To: []networking_v1.NetworkPolicyPeer{
						networking_v1.NetworkPolicyPeer{
							IPBlock: &networking_v1.IPBlock{
								CIDR: "192.169.0.0/16",
								Except: []string{
									"192.169.2.0/24",
								},
							},
							PodSelector: &meta_v1.LabelSelector{
								MatchLabels: LabelsPodsInBeta,
							},
							NamespaceSelector: &meta_v1.LabelSelector{
								MatchLabels: BetaNsLabels,
							},
						},
					},
					Ports: []networking_v1.NetworkPolicyPort{
						networking_v1.NetworkPolicyPort{
							Protocol: &tcp,
							Port:     port2,
						},
						networking_v1.NetworkPolicyPort{
							Protocol: &tcp,
							Port:     port3,
						},
					},
				},
				/*
					Egress - Second Rule
					Allow everything that goes to port 5000
				*/
				networking_v1.NetworkPolicyEgressRule{
					Ports: []networking_v1.NetworkPolicyPort{
						networking_v1.NetworkPolicyPort{
							Protocol: &tcp,
							Port:     port4,
						},
						networking_v1.NetworkPolicyPort{
							Protocol: &udp,
							Port:     port4,
						},
					},
				},
			},
		},
	}

	//---------------------------------
	//	The pods in this fake cluster
	//---------------------------------

	//	The pods are in mock_global_test.go
	allPods := []core_v1.Pod{}
	allPods = append(allPods, PodsInProduction...)
	allPods = append(allPods, PodsInBeta...)

	//---------------------------------
	//	Faking the Pod controller
	//---------------------------------

	testObj := new(MockPodController)

	//	Get pods in production
	testObj.On("GetPods", pcn_types.ObjectQuery{
		By:     "labels",
		Labels: LabelsPodsInProduction,
	}, pcn_types.ObjectQuery{
		By:   "name",
		Name: "*",
	}).Return(PodsInProduction, nil)

	//	Get ALL pods
	testObj.On("GetPods", pcn_types.ObjectQuery{
		By:   "name",
		Name: "*",
	}, pcn_types.ObjectQuery{
		By:   "name",
		Name: "*",
	}).Return(allPods, nil)

	testObj.On("GetPods", pcn_types.ObjectQuery{
		By:     "labels",
		Labels: LabelsPodsInBeta,
	}, pcn_types.ObjectQuery{
		By:     "labels",
		Labels: BetaNsLabels,
	}).Return(PodsInBeta, nil)

	parser := Init(testObj)

	nsPods, _ := parser.GetPodsAffected(policy)
	assert.Len(t, nsPods, 1)
	productionNsPods, exists := nsPods[ProductionNamespace]
	assert.True(t, exists)
	assert.Len(t, productionNsPods, len(PodsInProduction))

	spec := policy.Spec
	ingress, egress, _ := parser.ParsePolicyTypes(&spec)
	result := parser.ParseRules(ingress, egress, ProductionNamespace)

	assert.NotEmpty(t, result.Ingress)
	assert.NotEmpty(t, result.Egress)

	//---------------------------------
	//	What we expect for the first pod
	//---------------------------------

	fmt.Println("----- INGRESS -----")
	for _, rule := range result.Ingress {
		fmt.Printf("%+v\n", rule)
	}
	fmt.Println("-------------------")

	fmt.Println("----- EGRESS -----")
	for _, rule := range result.Egress {
		fmt.Printf("%+v\n", rule)
	}
	fmt.Println("------------------")

	/* =PPPPPPPPPPPPP
	//	Ingress
	expectedResult.Ingress = []k8sfirewall.ChainRule{
		k8sfirewall.ChainRule{
			L4proto: "TCP",
			Dport:   port1.IntVal,
			Src:     "192.168.2.0/24",
			Dst:     productionPod.Pod.Status.PodIP,
			Action:  "drop",
		},
		k8sfirewall.ChainRule{
			L4proto: "UDP",
			Dport:   port1.IntVal,
			Src:     "192.168.2.0/24",
			Dst:     productionPod.Pod.Status.PodIP,
			Action:  "drop",
		},
		k8sfirewall.ChainRule{
			L4proto: "TCP",
			Dport:   port1.IntVal,
			Src:     "192.168.0.0/16",
			Dst:     productionPod.Pod.Status.PodIP,
			Action:  "forward",
		},
		k8sfirewall.ChainRule{
			L4proto: "UDP",
			Dport:   port1.IntVal,
			Src:     "192.168.0.0/16",
			Dst:     productionPod.Pod.Status.PodIP,
			Action:  "forward",
		},
		k8sfirewall.ChainRule{
			L4proto: "TCP",
			Dport:   port1.IntVal,
			Src:     BetaPods[0].Pod.Status.PodIP,
			Dst:     productionPod.Pod.Status.PodIP,
			Action:  "forward",
		},
		k8sfirewall.ChainRule{
			L4proto: "UDP",
			Dport:   port1.IntVal,
			Src:     BetaPods[0].Pod.Status.PodIP,
			Dst:     productionPod.Pod.Status.PodIP,
			Action:  "forward",
		},
		//	2
		k8sfirewall.ChainRule{
			L4proto: "TCP",
			Dport:   port2.IntVal,
			Src:     "192.169.0.0/16",
			Dst:     productionPod.Pod.Status.PodIP,
			Action:  "forward",
		},
		k8sfirewall.ChainRule{
			L4proto: "TCP",
			Dport:   port3.IntVal,
			Src:     "192.169.0.0/16",
			Dst:     productionPod.Pod.Status.PodIP,
			Action:  "forward",
		},
		//3
		k8sfirewall.ChainRule{
			L4proto: "TCP",
			Dport:   port4.IntVal,
			Dst:     productionPod.Pod.Status.PodIP,
			Action:  "forward",
		},
		k8sfirewall.ChainRule{
			L4proto: "UDP",
			Dport:   port4.IntVal,
			Dst:     productionPod.Pod.Status.PodIP,
			Action:  "forward",
		},
	}
	assert.ElementsMatch(t, expectedResult.Ingress, firstResult.Ingress)

	//	Egress

	expectedResult.Egress = []k8sfirewall.ChainRule{
		k8sfirewall.ChainRule{
			L4proto: "TCP",
			Sport:   port1.IntVal,
			Dst:     "192.168.0.0/16",
			Src:     productionPod.Pod.Status.PodIP,
			Action:  "forward",
		},
		k8sfirewall.ChainRule{
			L4proto: "UDP",
			Sport:   port1.IntVal,
			Dst:     "192.168.0.0/16",
			Src:     productionPod.Pod.Status.PodIP,
			Action:  "forward",
		},
		k8sfirewall.ChainRule{
			L4proto: "TCP",
			Sport:   port1.IntVal,
			Dst:     BetaPods[0].Pod.Status.PodIP,
			Src:     productionPod.Pod.Status.PodIP,
			Action:  "forward",
		},
		k8sfirewall.ChainRule{
			L4proto: "UDP",
			Sport:   port1.IntVal,
			Dst:     BetaPods[0].Pod.Status.PodIP,
			Src:     productionPod.Pod.Status.PodIP,
			Action:  "forward",
		},
		//	2
		k8sfirewall.ChainRule{
			L4proto: "TCP",
			Sport:   port2.IntVal,
			Dst:     "192.169.2.0/24",
			Src:     productionPod.Pod.Status.PodIP,
			Action:  "forward",
		},
		k8sfirewall.ChainRule{
			L4proto: "TCP",
			Sport:   port3.IntVal,
			Dst:     "192.169.2.0/24",
			Src:     productionPod.Pod.Status.PodIP,
			Action:  "forward",
		},
		k8sfirewall.ChainRule{
			L4proto: "TCP",
			Sport:   port2.IntVal,
			Dst:     "192.169.0.0/16",
			Src:     productionPod.Pod.Status.PodIP,
			Action:  "forward",
		},
		k8sfirewall.ChainRule{
			L4proto: "TCP",
			Sport:   port3.IntVal,
			Dst:     "192.169.0.0/16",
			Src:     productionPod.Pod.Status.PodIP,
			Action:  "forward",
		},
		//3
		k8sfirewall.ChainRule{
			L4proto: "TCP",
			Sport:   port4.IntVal,
			Src:     productionPod.Pod.Status.PodIP,
			Action:  "forward",
		},
		k8sfirewall.ChainRule{
			L4proto: "UDP",
			Sport:   port4.IntVal,
			Src:     productionPod.Pod.Status.PodIP,
			Action:  "forward",
		},
	}
	assert.ElementsMatch(t, expectedResult.Ingress, firstResult.Ingress)*/
}

func TestRulesforPod(t *testing.T) {
	/*noPorts := []parsedProtoPort{}
	ports := []parsedProtoPort{
		parsedProtoPort{
			port:     8080,
			protocol: "TCP",
		},
		parsedProtoPort{
			port:     8181,
			protocol: "UDP",
		},
	}
	pod := core_v1.Pod{
		ObjectMeta: meta_v1.ObjectMeta{
			UID: "MY-POD-1-100",
		},
		Status: core_v1.PodStatus{
			PodIP: "100.100.100.100",
		},
	}
	notFound := []pcn_types.Pod{
		pcn_types.Pod{
			Pod: core_v1.Pod{
				ObjectMeta: meta_v1.ObjectMeta{
					UID: "MY-POD-2-150",
				},
				Status: core_v1.PodStatus{
					PodIP: "150.150.150.150",
				},
			},
		},
	}
	found := []pcn_types.Pod{
		pcn_types.Pod{
			Pod: pod,
		},
	}

	manager := Init(nil)

	//	The pod has not been found
	result := manager.generateRulesForPod(notFound, &pod, noPorts)
	assert.Empty(t, result.Ingress)
	assert.Empty(t, result.Egress)

	//	The pod has been found and there are no ports
	expectedIngress := []k8sfirewall.ChainRule{
		k8sfirewall.ChainRule{
			Src:    pod.Status.PodIP,
			Action: "forward",
		},
	}
	expectedEgress := []k8sfirewall.ChainRule{
		k8sfirewall.ChainRule{
			Dst:    pod.Status.PodIP,
			Action: "forward",
		},
	}
	result = manager.generateRulesForPod(found, &pod, noPorts)
	assert.NotEmpty(t, result.Ingress)
	assert.NotEmpty(t, result.Egress)
	assert.ElementsMatch(t, expectedIngress, result.Ingress)
	assert.ElementsMatch(t, expectedEgress, result.Egress)

	//	The pod has been found and there are ports
	expectedIngress = []k8sfirewall.ChainRule{
		k8sfirewall.ChainRule{
			Src:     pod.Status.PodIP,
			L4proto: "TCP",
			Dport:   ports[0].port,
			Action:  "forward",
		},
		k8sfirewall.ChainRule{
			Src:     pod.Status.PodIP,
			L4proto: "UDP",
			Dport:   ports[1].port,
			Action:  "forward",
		},
	}
	expectedEgress = []k8sfirewall.ChainRule{
		k8sfirewall.ChainRule{
			Dst:     pod.Status.PodIP,
			L4proto: "TCP",
			Sport:   ports[0].port,
			Action:  "forward",
		},
		k8sfirewall.ChainRule{
			Dst:     pod.Status.PodIP,
			L4proto: "UDP",
			Sport:   ports[1].port,
			Action:  "forward",
		},
	}
	result = manager.generateRulesForPod(found, &pod, ports)
	assert.NotEmpty(t, result.Ingress)
	assert.NotEmpty(t, result.Egress)
	assert.ElementsMatch(t, expectedIngress, result.Ingress)
	assert.ElementsMatch(t, expectedEgress, result.Egress)*/

}

func TestGetClusterActions(t *testing.T) {
	tcp := core_v1.ProtocolTCP
	udp := core_v1.ProtocolUDP
	port1 := &intstr.IntOrString{
		IntVal: 6895,
	}
	port2 := &intstr.IntOrString{
		IntVal: 8080,
	}
	port3 := &intstr.IntOrString{
		IntVal: 80,
	}
	namespaceLabels := map[string]string{
		"environment": "production",
	}
	betaLabels := map[string]string{
		"app": "in-beta",
	}
	podLabels := map[string]string{
		"type":    "app",
		"version": "2.0",
	}

	policy := &networking_v1.NetworkPolicy{
		ObjectMeta: meta_v1.ObjectMeta{
			Name:      TestDefaultPolicyName,
			Namespace: ProductionNamespace,
		},
		Spec: networking_v1.NetworkPolicySpec{
			PodSelector: meta_v1.LabelSelector{
				MatchLabels: LabelsPodsInProduction,
			},
			PolicyTypes: []networking_v1.PolicyType{
				networking_v1.PolicyTypeIngress,
				//networking_v1.PolicyTypeEgress,
			},
			Ingress: []networking_v1.NetworkPolicyIngressRule{

				//	First Rule: all pods inside my namespace
				networking_v1.NetworkPolicyIngressRule{
					From: []networking_v1.NetworkPolicyPeer{
						networking_v1.NetworkPolicyPeer{

							PodSelector: &meta_v1.LabelSelector{
								MatchLabels: map[string]string{},
							},
						},
					},
					Ports: []networking_v1.NetworkPolicyPort{
						networking_v1.NetworkPolicyPort{
							Protocol: &tcp,
							Port:     port1,
						},
						networking_v1.NetworkPolicyPort{
							Protocol: &udp,
							Port:     port1,
						},
					},
				},

				//	Second rule: pods with specific labels no ports, in my same namespace
				//	OR all pods in specific namespace
				networking_v1.NetworkPolicyIngressRule{
					From: []networking_v1.NetworkPolicyPeer{
						networking_v1.NetworkPolicyPeer{
							PodSelector: &meta_v1.LabelSelector{
								MatchLabels: podLabels,
							},
						},

						networking_v1.NetworkPolicyPeer{
							NamespaceSelector: &meta_v1.LabelSelector{
								MatchLabels: betaLabels,
							},
						},
					},
				},

				//	Fourth rule: pods with specific labels in namespace with specific labels
				networking_v1.NetworkPolicyIngressRule{
					From: []networking_v1.NetworkPolicyPeer{
						networking_v1.NetworkPolicyPeer{

							PodSelector: &meta_v1.LabelSelector{
								MatchLabels: podLabels,
							},
							NamespaceSelector: &meta_v1.LabelSelector{
								MatchLabels: namespaceLabels,
							},
						},
					},
				},
			},
		},
	}

	testObj := new(MockPodController)
	parser := Init(testObj)

	expectedIngress := []pcn_types.FirewallAction{
		pcn_types.FirewallAction{
			PodLabels:     policy.Spec.Ingress[0].From[0].PodSelector.MatchLabels,
			NamespaceName: policy.Namespace,
			Actions:       parser.GetConnectionTemplate("ingress", "10.0.0.1", "", pcn_types.ActionForward, []pcn_types.ProtoPort{}),
		},
		pcn_types.FirewallAction{
			PodLabels:     policy.Spec.Ingress[1].From[0].PodSelector.MatchLabels,
			NamespaceName: policy.Namespace,
			Actions:       parser.GetConnectionTemplate("ingress", "10.0.0.1", "", pcn_types.ActionForward, []pcn_types.ProtoPort{}),
		},
		pcn_types.FirewallAction{
			NamespaceLabels: policy.Spec.Ingress[1].From[1].NamespaceSelector.MatchLabels,
			Actions:         parser.GetConnectionTemplate("ingress", "10.0.0.1", "", pcn_types.ActionForward, []pcn_types.ProtoPort{}),
		},
		pcn_types.FirewallAction{
			PodLabels:       policy.Spec.Ingress[2].From[0].PodSelector.MatchLabels,
			NamespaceLabels: policy.Spec.Ingress[2].From[0].NamespaceSelector.MatchLabels,
			Actions:         parser.GetConnectionTemplate("ingress", "10.0.0.1", "", pcn_types.ActionForward, []pcn_types.ProtoPort{}),
		},
	}

	ingress, egress, _ := parser.ParsePolicyTypes(&policy.Spec)
	actions := parser.GetClusterActions(ingress, egress, policy.Namespace)

	//	No need to test for the rules as they are tested in GetTemplate
	for i := 0; i < len(actions.Ingress); i++ {
		assert.Equal(t, actions.Ingress[i].NamespaceName, expectedIngress[i].NamespaceName)
		assert.Equal(t, actions.Ingress[i].NamespaceLabels, expectedIngress[i].NamespaceLabels)
		assert.Equal(t, actions.Ingress[i].PodLabels, expectedIngress[i].PodLabels)
		fmt.Printf("%+v\n", actions.Ingress[i])
		/*for t := 0; t < len(actions.Ingress[i].Actions.Ingress); t++ {
			fmt.Printf("---%+v\n", actions.Ingress[i].Actions.Ingress[t])
		}*/
		fmt.Println("--")
	}
	assert.Empty(t, actions.Egress)

	//	With egress as well

	policy.Spec.Egress = []networking_v1.NetworkPolicyEgressRule{

		networking_v1.NetworkPolicyEgressRule{
			To: []networking_v1.NetworkPolicyPeer{
				networking_v1.NetworkPolicyPeer{
					PodSelector: &meta_v1.LabelSelector{
						MatchLabels: podLabels,
					},
					NamespaceSelector: &meta_v1.LabelSelector{
						MatchLabels: namespaceLabels,
					},
				},
			},
			Ports: []networking_v1.NetworkPolicyPort{
				networking_v1.NetworkPolicyPort{
					Protocol: &tcp,
					Port:     port2,
				},
				networking_v1.NetworkPolicyPort{
					Protocol: &tcp,
					Port:     port3,
				},
			},
		},
		networking_v1.NetworkPolicyEgressRule{
			Ports: []networking_v1.NetworkPolicyPort{
				networking_v1.NetworkPolicyPort{
					Protocol: &tcp,
					Port:     port2,
				},
				networking_v1.NetworkPolicyPort{
					Protocol: &udp,
					Port:     port3,
				},
			},
		},
	}
	policy.Spec.PolicyTypes = append(policy.Spec.PolicyTypes, networking_v1.PolicyTypeEgress)

	expectedEgress := []pcn_types.FirewallAction{
		pcn_types.FirewallAction{
			PodLabels:       policy.Spec.Egress[0].To[0].PodSelector.MatchLabels,
			NamespaceLabels: policy.Spec.Egress[0].To[0].NamespaceSelector.MatchLabels,
		},
	}
	ingress, egress, _ = parser.ParsePolicyTypes(&policy.Spec)
	actions = parser.GetClusterActions(ingress, egress, policy.Namespace)

	for i := 0; i < len(actions.Egress); i++ {
		assert.Equal(t, actions.Egress[i].NamespaceName, expectedEgress[i].NamespaceName)
		assert.Equal(t, actions.Egress[i].NamespaceLabels, expectedEgress[i].NamespaceLabels)
		assert.Equal(t, actions.Egress[i].PodLabels, expectedEgress[i].PodLabels)
	}
}
