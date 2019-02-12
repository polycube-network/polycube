package networkpolicies

import (
	"testing"

	k8s_types "k8s.io/apimachinery/pkg/types"
	"k8s.io/apimachinery/pkg/util/intstr"

	pcn_types "github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/types"

	"github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/controllers"

	"github.com/stretchr/testify/mock"

	k8sfirewall "github.com/SunSince90/polycube/src/components/k8s/utils/k8sfirewall"
	"github.com/stretchr/testify/assert"
	core_v1 "k8s.io/api/core/v1"
	networking_v1 "k8s.io/api/networking/v1"
	meta_v1 "k8s.io/apimachinery/pkg/apis/meta/v1"
)

var GlobalPodSelector1 = meta_v1.LabelSelector{
	MatchLabels: map[string]string{
		"type":    "app",
		"version": "2.5",
	},
}
var GlobalNamespace1 = "Production"
var GlobalName1 = "TestDefaultPolicy"
var GlobalQuery1 = pcn_types.Query{
	Pod: []pcn_types.QueryObject{
		pcn_types.QueryObject{
			By:     "labels",
			Labels: GlobalPodSelector1.MatchLabels,
		},
	},
	Namespace: []pcn_types.QueryObject{
		pcn_types.QueryObject{
			By:   "name",
			Name: GlobalNamespace1,
		},
	},
}
var GlobalPodsFound1 = []pcn_types.Pod{
	pcn_types.Pod{
		Pod: core_v1.Pod{
			ObjectMeta: meta_v1.ObjectMeta{
				Namespace: GlobalNamespace1,
			},
			Status: core_v1.PodStatus{
				PodIP: "172.100.10.10",
			},
		},
		Veth: "veth123",
	},
	pcn_types.Pod{
		Pod: core_v1.Pod{
			ObjectMeta: meta_v1.ObjectMeta{
				Namespace: GlobalNamespace1,
			},
			Status: core_v1.PodStatus{
				PodIP: "172.100.20.20",
			},
		},
		Veth: "veth123",
	},
}

//	Mock the pod controller
type MockPodController struct {
	mock.Mock
}

func (m *MockPodController) Run()  {}
func (m *MockPodController) Stop() {}
func (m *MockPodController) Subscribe(event pcn_types.EventType, consumer func(*pcn_types.Pod)) (func(), error) {
	return func() {}, nil
}
func (m *MockPodController) GetPodsByName(name string, ns string) ([]pcn_types.Pod, error) {
	args := m.Called(name, ns)
	return args.Get(0).([]pcn_types.Pod), args.Error(1)
}
func (m *MockPodController) AsyncGetPodsByName(channel chan<- []pcn_types.Pod, name string, ns string) ([]pcn_types.Pod, error) {
	args := m.Called(channel, name, ns)
	return args.Get(0).([]pcn_types.Pod), args.Error(1)
}
func (m *MockPodController) GetPodsByLabels(labels map[string]string, ns string) ([]pcn_types.Pod, error) {
	args := m.Called(labels, ns)
	return args.Get(0).([]pcn_types.Pod), args.Error(1)
}
func (m *MockPodController) AsyncGetPodsByLabels(channel chan<- []pcn_types.Pod, labels map[string]string, ns string) ([]pcn_types.Pod, error) {
	args := m.Called(channel, labels, ns)
	return args.Get(0).([]pcn_types.Pod), args.Error(1)
}
func (m *MockPodController) GetPods(query pcn_types.Query) ([]pcn_types.Pod, error) {
	args := m.Called(query)
	return args.Get(0).([]pcn_types.Pod), args.Error(1)
}
func (m *MockPodController) AsyncGetPods(channel chan<- []pcn_types.Pod, query pcn_types.Query) ([]pcn_types.Pod, error) {
	args := m.Called(query)
	return args.Get(0).([]pcn_types.Pod), args.Error(1)
}

func Init(podController controllers.PodController) *NetworkPolicyManager {
	manager := NewNetworkPolicyManager(nil, podController, "url")

	return manager
}

func TestNotNil(t *testing.T) {
	manager := Init(nil)
	assert.NotNil(t, manager)
}

func CommonTest(t *testing.T, err error, generatedFirewall []k8sfirewall.Firewall) {
	assert.Nil(t, err)

	if err != nil {
		t.Errorf("error is not nil %s", err)
	}

	//assert.Equal(t, false, generatedFirewall.Interactive)
	//assert.Equal(t, 2, len(generatedFirewall.Chain))
}

func TestNoSpec(t *testing.T) {

	testObj := new(MockPodController)
	manager := Init(testObj)

	ingress, egress := manager.GetPolicyTypes(nil)

	assert.Zero(t, len(ingress))
	assert.Zero(t, len(egress))

	ingressChain, egressChain := manager.ParseDefaultRules(ingress, egress, "ns")

	assert.Equal(t, "ingress", ingressChain.Name)
	assert.Equal(t, "egress", egressChain.Name)

	assert.Zero(t, len(ingressChain.Rule))
	assert.Zero(t, len(egressChain.Rule))

	assert.Equal(t, "drop", ingressChain.Default_)
	assert.Equal(t, "drop", egressChain.Default_)
}

func TestPolicyTypes(t *testing.T) {
	testObj := new(MockPodController)
	manager := Init(testObj)

	spec := &networking_v1.NetworkPolicySpec{
		Ingress: []networking_v1.NetworkPolicyIngressRule{},
	}
	ingress, egress := manager.GetPolicyTypes(spec)
	assert.NotNil(t, ingress)
	assert.Empty(t, ingress)
	assert.Nil(t, egress)

	spec = &networking_v1.NetworkPolicySpec{
		PolicyTypes: []networking_v1.PolicyType{
			networking_v1.PolicyTypeIngress,
		},
		Ingress: []networking_v1.NetworkPolicyIngressRule{},
	}
	ingress, egress = manager.GetPolicyTypes(spec)
	assert.NotNil(t, ingress)
	assert.Empty(t, ingress)
	assert.Nil(t, egress)

	//	When you only have egress, you MUST specify it, otherwise it is incorrectly parsed
	spec = &networking_v1.NetworkPolicySpec{
		Egress: []networking_v1.NetworkPolicyEgressRule{},
	}
	ingress, egress = manager.GetPolicyTypes(spec)
	assert.Nil(t, ingress)
	assert.Empty(t, ingress)
	assert.Nil(t, egress)

	spec = &networking_v1.NetworkPolicySpec{
		PolicyTypes: []networking_v1.PolicyType{
			networking_v1.PolicyTypeEgress,
		},
		Egress: []networking_v1.NetworkPolicyEgressRule{},
	}
	ingress, egress = manager.GetPolicyTypes(spec)
	assert.NotNil(t, egress)
	assert.Empty(t, egress)
	assert.Nil(t, ingress)

	spec = &networking_v1.NetworkPolicySpec{
		PolicyTypes: []networking_v1.PolicyType{
			networking_v1.PolicyTypeEgress,
			networking_v1.PolicyTypeIngress,
		},
		Ingress: []networking_v1.NetworkPolicyIngressRule{},
		Egress:  []networking_v1.NetworkPolicyEgressRule{},
	}
	ingress, egress = manager.GetPolicyTypes(spec)
	assert.NotNil(t, egress)
	assert.NotNil(t, ingress)
	assert.Empty(t, egress)
	assert.Empty(t, ingress)
}

func TestIngressLenIsZero(t *testing.T) {

	// When Ingress array is empty, nothing is allowed in ingress
	ingress := []networking_v1.NetworkPolicyIngressRule{}

	testObj := new(MockPodController)
	manager := Init(testObj)

	ingressChain, egressChain := manager.ParseDefaultRules(ingress, nil, "ns")

	assert.Equal(t, "ingress", ingressChain.Name)
	assert.Equal(t, "egress", egressChain.Name)

	assert.Zero(t, len(ingressChain.Rule))
	assert.Zero(t, len(egressChain.Rule))

	assert.Equal(t, "drop", ingressChain.Default_)
	assert.Equal(t, "drop", egressChain.Default_)
}

func TestFromIsNil(t *testing.T) {

	//When From is empty, everything is allowed in ingress
	ingress := []networking_v1.NetworkPolicyIngressRule{
		networking_v1.NetworkPolicyIngressRule{
			From: nil,
		},
	}
	testObj := new(MockPodController)
	manager := Init(testObj)

	//	No need to check for egresschain anymore, the case when it is nil it's already been tested above
	ingressChain, _ := manager.ParseDefaultRules(ingress, nil, "ns")

	assert.Equal(t, "ingress", ingressChain.Name)
	assert.Zero(t, len(ingressChain.Rule))
	assert.Equal(t, "forward", ingressChain.Default_)
}

func TestFromIsEmpty(t *testing.T) {

	//When From is empty, everything is allowed in ingress
	ingress := []networking_v1.NetworkPolicyIngressRule{
		networking_v1.NetworkPolicyIngressRule{
			From: []networking_v1.NetworkPolicyPeer{},
		},
	}
	testObj := new(MockPodController)
	manager := Init(testObj)

	//	No need to check for egresschain anymore, the case when it is nil it's already been tested above
	ingressChain, _ := manager.ParseDefaultRules(ingress, nil, "ns")

	assert.Equal(t, "ingress", ingressChain.Name)
	assert.Zero(t, len(ingressChain.Rule))
	assert.Equal(t, "drop", ingressChain.Default_)
}

func TestToIsNil(t *testing.T) {

	//When To is empty, everything is allowed in egress
	egress := []networking_v1.NetworkPolicyEgressRule{
		networking_v1.NetworkPolicyEgressRule{
			To: nil,
		},
	}
	testObj := new(MockPodController)
	manager := Init(testObj)

	_, egressChain := manager.ParseDefaultRules(nil, egress, "ns")

	assert.Equal(t, "egress", egressChain.Name)
	assert.Zero(t, len(egressChain.Rule))
	assert.Equal(t, "forward", egressChain.Default_)
}

func TestToIsEmpty(t *testing.T) {

	//When To is empty, everything is allowed in egress
	egress := []networking_v1.NetworkPolicyEgressRule{
		networking_v1.NetworkPolicyEgressRule{
			To: []networking_v1.NetworkPolicyPeer{},
		},
	}
	testObj := new(MockPodController)
	manager := Init(testObj)

	_, egressChain := manager.ParseDefaultRules(nil, egress, "ns")

	assert.Equal(t, "egress", egressChain.Name)
	assert.Zero(t, len(egressChain.Rule))
	assert.Equal(t, "drop", egressChain.Default_)
}

func TestSingleRuleIPBlockNoExceptions(t *testing.T) {

	ipBlock := &networking_v1.IPBlock{
		CIDR: "192.168.1.20/32",
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
		},
	}
	expectedIngressRule := []k8sfirewall.ChainRule{
		k8sfirewall.ChainRule{
			Src:    ipBlock.CIDR,
			Action: "forward",
		},
	}

	iRules := manager.parseDefaultIPBlock(ipBlock, "ingress")
	assert.Equal(t, expectedIngressRule, iRules)
	ingressChain := manager.parseDefaultIngressRules(ingress, "ns")
	assert.Len(t, ingressChain.Rule, 1)
	assert.Equal(t, expectedIngressRule, ingressChain.Rule)

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
		},
	}
	expectedEgressRule := []k8sfirewall.ChainRule{
		k8sfirewall.ChainRule{
			Dst:    ipBlock.CIDR,
			Action: "forward",
		},
	}

	eRules := manager.parseDefaultIPBlock(ipBlock, "egress")
	assert.Equal(t, expectedEgressRule, eRules)
	egressChain := manager.parseDefaultEgressRules(egress, "ns")
	assert.Len(t, egressChain.Rule, 1)
	assert.Equal(t, expectedEgressRule, egressChain.Rule)
}

func TestSingleRuleIPBlockMultipleExceptions(t *testing.T) {

	ipBlock := &networking_v1.IPBlock{
		CIDR: "192.168.0.0/16",
		Except: []string{
			"192.168.10.0/24",
			"192.168.20.0/24",
			"192.168.30.0/24",
		},
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
		},
	}
	expectedIngressRule := []k8sfirewall.ChainRule{
		k8sfirewall.ChainRule{
			Src:    ipBlock.CIDR,
			Action: "forward",
		},
		k8sfirewall.ChainRule{
			Src:    ipBlock.Except[0],
			Action: "drop",
		},
		k8sfirewall.ChainRule{
			Src:    ipBlock.Except[1],
			Action: "drop",
		},
		k8sfirewall.ChainRule{
			Src:    ipBlock.Except[2],
			Action: "drop",
		},
	}

	iRules := manager.parseDefaultIPBlock(ipBlock, "ingress")
	assert.Equal(t, expectedIngressRule, iRules)
	ingressChain := manager.parseDefaultIngressRules(ingress, "ns")
	assert.Len(t, ingressChain.Rule, 4)
	assert.Equal(t, expectedIngressRule, ingressChain.Rule)

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
		},
	}
	expectedEgressRule := []k8sfirewall.ChainRule{
		k8sfirewall.ChainRule{
			Dst:    ipBlock.CIDR,
			Action: "forward",
		},
		k8sfirewall.ChainRule{
			Dst:    ipBlock.Except[0],
			Action: "drop",
		},
		k8sfirewall.ChainRule{
			Dst:    ipBlock.Except[1],
			Action: "drop",
		},
		k8sfirewall.ChainRule{
			Dst:    ipBlock.Except[2],
			Action: "drop",
		},
	}

	eRules := manager.parseDefaultIPBlock(ipBlock, "egress")
	assert.Equal(t, expectedEgressRule, eRules)
	egressChain := manager.parseDefaultEgressRules(egress, "ns")
	assert.Len(t, egressChain.Rule, 4)
	assert.Equal(t, expectedEgressRule, egressChain.Rule)
}

func TestSingleRuleIPBlockMultipleExceptionsWithProtocolNoPort(t *testing.T) {
	ipBlock := &networking_v1.IPBlock{
		CIDR: "192.168.0.0/16",
		Except: []string{
			"192.168.10.0/24",
			"192.168.20.0/24",
			"192.168.30.0/24",
		},
	}
	testObj := new(MockPodController)
	manager := Init(testObj)
	tcp := core_v1.ProtocolTCP
	ports := []networking_v1.NetworkPolicyPort{
		networking_v1.NetworkPolicyPort{
			Protocol: &tcp,
		},
	}

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
	expectedIngressRule := []k8sfirewall.ChainRule{
		k8sfirewall.ChainRule{
			Src:    ipBlock.CIDR,
			Action: "forward",
		},
		k8sfirewall.ChainRule{
			Src:    ipBlock.Except[0],
			Action: "drop",
		},
		k8sfirewall.ChainRule{
			Src:    ipBlock.Except[1],
			Action: "drop",
		},
		k8sfirewall.ChainRule{
			Src:    ipBlock.Except[2],
			Action: "drop",
		},
	}

	iRules := manager.parseDefaultIPBlock(ipBlock, "ingress")
	assert.Equal(t, expectedIngressRule, iRules)
	for i := 0; i < len(expectedIngressRule); i++ {
		expectedIngressRule[i].L4proto = "TCP"
	}
	ingressChain := manager.parseDefaultIngressRules(ingress, "ns")
	assert.Len(t, ingressChain.Rule, 4)
	assert.Equal(t, expectedIngressRule, ingressChain.Rule)

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
	expectedEgressRule := []k8sfirewall.ChainRule{
		k8sfirewall.ChainRule{
			Dst:    ipBlock.CIDR,
			Action: "forward",
		},
		k8sfirewall.ChainRule{
			Dst:    ipBlock.Except[0],
			Action: "drop",
		},
		k8sfirewall.ChainRule{
			Dst:    ipBlock.Except[1],
			Action: "drop",
		},
		k8sfirewall.ChainRule{
			Dst:    ipBlock.Except[2],
			Action: "drop",
		},
	}

	eRules := manager.parseDefaultIPBlock(ipBlock, "egress")
	assert.Equal(t, expectedEgressRule, eRules)
	for i := 0; i < len(expectedEgressRule); i++ {
		expectedEgressRule[i].L4proto = "TCP"
	}
	egressChain := manager.parseDefaultEgressRules(egress, "ns")
	assert.Len(t, egressChain.Rule, 4)
	assert.Equal(t, expectedEgressRule, egressChain.Rule)
}

func TestSingleRuleIPBlockMultipleExceptionsWithMultipleProtocolAndPort(t *testing.T) {
	ipBlock := &networking_v1.IPBlock{
		CIDR: "192.168.0.0/16",
		Except: []string{
			"192.168.10.0/24",
			"192.168.20.0/24",
			"192.168.30.0/24",
		},
	}
	tcp := core_v1.ProtocolTCP
	udp := core_v1.ProtocolUDP
	port := &intstr.IntOrString{
		IntVal: 80,
	}
	ports := []networking_v1.NetworkPolicyPort{
		networking_v1.NetworkPolicyPort{
			Protocol: &tcp,
			Port:     port,
		},
		networking_v1.NetworkPolicyPort{
			Protocol: &udp,
			Port:     port,
		},
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
	expectedIngressRule := []k8sfirewall.ChainRule{
		k8sfirewall.ChainRule{
			Src:     ipBlock.CIDR,
			Action:  "forward",
			L4proto: "TCP",
			Sport:   port.IntVal,
		},
		k8sfirewall.ChainRule{
			Src:     ipBlock.CIDR,
			Action:  "forward",
			L4proto: "UDP",
			Sport:   port.IntVal,
		},
		k8sfirewall.ChainRule{
			Src:     ipBlock.Except[0],
			Action:  "drop",
			L4proto: "TCP",
			Sport:   port.IntVal,
		},
		k8sfirewall.ChainRule{
			Src:     ipBlock.Except[0],
			Action:  "drop",
			L4proto: "UDP",
			Sport:   port.IntVal,
		},
		k8sfirewall.ChainRule{
			Src:     ipBlock.Except[1],
			Action:  "drop",
			L4proto: "TCP",
			Sport:   port.IntVal,
		},
		k8sfirewall.ChainRule{
			Src:     ipBlock.Except[1],
			Action:  "drop",
			L4proto: "UDP",
			Sport:   port.IntVal,
		},
		k8sfirewall.ChainRule{
			Src:     ipBlock.Except[2],
			Action:  "drop",
			L4proto: "TCP",
			Sport:   port.IntVal,
		},
		k8sfirewall.ChainRule{
			Src:     ipBlock.Except[2],
			Action:  "drop",
			L4proto: "UDP",
			Sport:   port.IntVal,
		},
	}

	ingressChain := manager.parseDefaultIngressRules(ingress, "ns")
	assert.Len(t, ingressChain.Rule, len(expectedIngressRule))
	assert.Equal(t, expectedIngressRule, ingressChain.Rule)

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
	expectedEgressRule := []k8sfirewall.ChainRule{
		k8sfirewall.ChainRule{
			Dst:     ipBlock.CIDR,
			Action:  "forward",
			L4proto: "TCP",
			Dport:   port.IntVal,
		},
		k8sfirewall.ChainRule{
			Dst:     ipBlock.CIDR,
			Action:  "forward",
			L4proto: "UDP",
			Dport:   port.IntVal,
		},
		k8sfirewall.ChainRule{
			Dst:     ipBlock.Except[0],
			Action:  "drop",
			L4proto: "TCP",
			Dport:   port.IntVal,
		},
		k8sfirewall.ChainRule{
			Dst:     ipBlock.Except[0],
			Action:  "drop",
			L4proto: "UDP",
			Dport:   port.IntVal,
		},
		k8sfirewall.ChainRule{
			Dst:     ipBlock.Except[1],
			Action:  "drop",
			L4proto: "TCP",
			Dport:   port.IntVal,
		},
		k8sfirewall.ChainRule{
			Dst:     ipBlock.Except[1],
			Action:  "drop",
			L4proto: "UDP",
			Dport:   port.IntVal,
		},
		k8sfirewall.ChainRule{
			Dst:     ipBlock.Except[2],
			Action:  "drop",
			L4proto: "TCP",
			Dport:   port.IntVal,
		},
		k8sfirewall.ChainRule{
			Dst:     ipBlock.Except[2],
			Action:  "drop",
			L4proto: "UDP",
			Dport:   port.IntVal,
		},
	}

	egressChain := manager.parseDefaultEgressRules(egress, "ns")
	assert.Len(t, egressChain.Rule, len(expectedEgressRule))
	assert.Equal(t, expectedEgressRule, egressChain.Rule)
}

func TestSupportedProtocols(t *testing.T) {
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
	supported, protocol, pport := manager.parseDefaultProtocolAndPort(ports[0])
	assert.Equal(t, "UDP", protocol)
	assert.Equal(t, int32(port.IntVal), pport)
	assert.True(t, supported)

	//	--- TCP
	supported, protocol, pport = manager.parseDefaultProtocolAndPort(ports[2])
	assert.Equal(t, "TCP", protocol)
	assert.Equal(t, int32(port.IntVal), pport)
	assert.True(t, supported)

	//	--- STCP
	supported, protocol, pport = manager.parseDefaultProtocolAndPort(ports[1])
	assert.Len(t, protocol, 0)
	assert.Zero(t, pport)
	assert.False(t, supported)

	//	--- Generated ports
	expectedGeneratedPorts := []parsedProtoPort{
		parsedProtoPort{
			port:     int32(port.IntVal),
			protocol: "TCP",
		},
		parsedProtoPort{
			port:     int32(port.IntVal),
			protocol: "UDP",
		},
	}
	parsedPP := manager.generatePorts(ports)
	assert.ElementsMatch(t, parsedPP, expectedGeneratedPorts)
}
func TestGeneratedSupportedProtocolsWithUnsupported(t *testing.T) {
	stcp := core_v1.ProtocolSCTP
	tcp := core_v1.ProtocolTCP
	port := &intstr.IntOrString{
		IntVal: 6895,
	}
	ports := []networking_v1.NetworkPolicyPort{
		networking_v1.NetworkPolicyPort{
			Protocol: &stcp,
			Port:     port,
		},
		networking_v1.NetworkPolicyPort{
			Protocol: &tcp,
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
	expectedIngressRules := []k8sfirewall.ChainRule{
		k8sfirewall.ChainRule{
			Src:     ipBlock.CIDR,
			Action:  "forward",
			L4proto: "TCP",
			Sport:   int32(port.IntVal),
		},
	}

	ingressChain := manager.parseDefaultIngressRules(ingress, "ns")
	assert.ElementsMatch(t, expectedIngressRules, ingressChain.Rule)

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
	expectedEgressRule := []k8sfirewall.ChainRule{
		k8sfirewall.ChainRule{
			Dst:     ipBlock.CIDR,
			Action:  "forward",
			L4proto: "TCP",
			Dport:   int32(port.IntVal),
		},
	}

	egressChain := manager.parseDefaultEgressRules(egress, "ns")
	assert.ElementsMatch(t, expectedEgressRule, egressChain.Rule)
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

	ingressChain := manager.parseDefaultIngressRules(ingress, "ns")
	assert.Empty(t, ingressChain.Rule)

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

	egressChain := manager.parseDefaultEgressRules(egress, "ns")
	assert.Empty(t, egressChain.Rule)
}

func TestProtocolIsNil(t *testing.T) {
	//	Not very sure if protocol can be nil, but just in case ...
	port := &intstr.IntOrString{
		IntVal: 6895,
	}
	ports := []networking_v1.NetworkPolicyPort{
		networking_v1.NetworkPolicyPort{
			Port: port,
		},
	}
	ipBlock := &networking_v1.IPBlock{
		CIDR:   "192.168.0.0/16",
		Except: []string{"192.168.10.0/16"},
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
	expectedIngressRules := []k8sfirewall.ChainRule{
		k8sfirewall.ChainRule{
			Src:     ipBlock.CIDR,
			Action:  "forward",
			L4proto: "TCP",
			Sport:   int32(port.IntVal),
		},
		k8sfirewall.ChainRule{
			Src:     ipBlock.CIDR,
			Action:  "forward",
			L4proto: "UDP",
			Sport:   int32(port.IntVal),
		},
		k8sfirewall.ChainRule{
			Src:     ipBlock.Except[0],
			Action:  "drop",
			L4proto: "TCP",
			Sport:   int32(port.IntVal),
		},
		k8sfirewall.ChainRule{
			Src:     ipBlock.Except[0],
			Action:  "drop",
			L4proto: "UDP",
			Sport:   int32(port.IntVal),
		},
	}

	ingressChain := manager.parseDefaultIngressRules(ingress, "ns")
	assert.ElementsMatch(t, expectedIngressRules, ingressChain.Rule)

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
	expectedEgressRule := []k8sfirewall.ChainRule{
		k8sfirewall.ChainRule{
			Dst:     ipBlock.CIDR,
			Action:  "forward",
			L4proto: "TCP",
			Dport:   int32(port.IntVal),
		},
		k8sfirewall.ChainRule{
			Dst:     ipBlock.CIDR,
			Action:  "forward",
			L4proto: "UDP",
			Dport:   int32(port.IntVal),
		},
		k8sfirewall.ChainRule{
			Dst:     ipBlock.Except[0],
			Action:  "drop",
			L4proto: "TCP",
			Dport:   int32(port.IntVal),
		},
		k8sfirewall.ChainRule{
			Dst:     ipBlock.Except[0],
			Action:  "drop",
			L4proto: "UDP",
			Dport:   int32(port.IntVal),
		},
	}

	egressChain := manager.parseDefaultEgressRules(egress, "ns")
	assert.ElementsMatch(t, expectedEgressRule, egressChain.Rule)
}

func TestPodSelectorSameNamespace(t *testing.T) {
	tcp := core_v1.ProtocolTCP
	port := &intstr.IntOrString{
		IntVal: 6895,
	}
	ports := []networking_v1.NetworkPolicyPort{
		networking_v1.NetworkPolicyPort{
			Port:     port,
			Protocol: &tcp,
		},
	}
	labels := map[string]string{
		"v":     "1.2",
		"cache": "redis",
	}
	podSelector := &meta_v1.LabelSelector{
		MatchLabels: labels,
	}
	testObj := new(MockPodController)
	podsFound := []pcn_types.Pod{
		pcn_types.Pod{
			Pod: core_v1.Pod{
				Status: core_v1.PodStatus{
					PodIP: "172.10.30.30",
				},
			},
			Veth: "veth123",
		},
		pcn_types.Pod{
			Pod: core_v1.Pod{
				Status: core_v1.PodStatus{
					PodIP: "172.10.50.30",
				},
			},
			Veth: "veth123",
		},
	}
	testObj.On("GetPods", pcn_types.Query{
		Pod: []pcn_types.QueryObject{
			pcn_types.QueryObject{
				By:     "labels",
				Labels: labels,
			},
		},
		Namespace: []pcn_types.QueryObject{
			pcn_types.QueryObject{
				By:   "name",
				Name: GlobalNamespace1,
			},
		},
	}).Return(podsFound, nil)
	manager := Init(testObj)

	//---------------------------------
	//	Ingress
	//---------------------------------

	ingress := []networking_v1.NetworkPolicyIngressRule{
		networking_v1.NetworkPolicyIngressRule{
			From: []networking_v1.NetworkPolicyPeer{
				networking_v1.NetworkPolicyPeer{
					PodSelector: podSelector,
				},
			},
			Ports: ports,
		},
	}
	expectedIngressRules := []k8sfirewall.ChainRule{
		k8sfirewall.ChainRule{
			Src:    podsFound[0].Pod.Status.PodIP,
			Action: "forward",
		},
		k8sfirewall.ChainRule{
			Src:    podsFound[1].Pod.Status.PodIP,
			Action: "forward",
		},
	}
	rules, err := manager.parseDefaultSelectors(podSelector, nil, GlobalNamespace1, "ingress")
	assert.Nil(t, err)
	assert.ElementsMatch(t, expectedIngressRules, rules)
	for i := 0; i < len(expectedIngressRules); i++ {
		expectedIngressRules[i].L4proto = "TCP"
		expectedIngressRules[i].Sport = int32(port.IntVal)
	}
	ingressChain := manager.parseDefaultIngressRules(ingress, GlobalNamespace1)
	assert.ElementsMatch(t, expectedIngressRules, ingressChain.Rule)

	//---------------------------------
	//	Egress
	//---------------------------------

	egress := []networking_v1.NetworkPolicyEgressRule{
		networking_v1.NetworkPolicyEgressRule{
			To: []networking_v1.NetworkPolicyPeer{
				networking_v1.NetworkPolicyPeer{
					PodSelector: podSelector,
				},
			},
			Ports: ports,
		},
	}
	expectedEgressRules := []k8sfirewall.ChainRule{
		k8sfirewall.ChainRule{
			Dst:    podsFound[0].Pod.Status.PodIP,
			Action: "forward",
		},
		k8sfirewall.ChainRule{
			Dst:    podsFound[1].Pod.Status.PodIP,
			Action: "forward",
		},
	}

	rules, err = manager.parseDefaultSelectors(podSelector, nil, GlobalNamespace1, "egress")
	assert.Nil(t, err)
	assert.ElementsMatch(t, expectedEgressRules, rules)
	for i := 0; i < len(expectedEgressRules); i++ {
		expectedEgressRules[i].L4proto = "TCP"
		expectedEgressRules[i].Dport = int32(port.IntVal)
	}
	egressChain := manager.parseDefaultEgressRules(egress, GlobalNamespace1)
	assert.ElementsMatch(t, expectedEgressRules, egressChain.Rule)
}

func TestPodSelectorNoPodsFound(t *testing.T) {
	tcp := core_v1.ProtocolTCP
	port := &intstr.IntOrString{
		IntVal: 6895,
	}
	ports := []networking_v1.NetworkPolicyPort{
		networking_v1.NetworkPolicyPort{
			Port:     port,
			Protocol: &tcp,
		},
	}
	labels := map[string]string{
		"v":     "1.2",
		"cache": "redis",
	}
	podSelector := &meta_v1.LabelSelector{
		MatchLabels: labels,
	}
	testObj := new(MockPodController)
	podsFound := []pcn_types.Pod{}
	testObj.On("GetPods", pcn_types.Query{
		Pod: []pcn_types.QueryObject{
			pcn_types.QueryObject{
				By:     "labels",
				Labels: labels,
			},
		},
		Namespace: []pcn_types.QueryObject{
			pcn_types.QueryObject{
				By:   "name",
				Name: GlobalNamespace1,
			},
		},
	}).Return(podsFound, nil)
	manager := Init(testObj)

	//---------------------------------
	//	Ingress
	//---------------------------------

	ingress := []networking_v1.NetworkPolicyIngressRule{
		networking_v1.NetworkPolicyIngressRule{
			From: []networking_v1.NetworkPolicyPeer{
				networking_v1.NetworkPolicyPeer{
					PodSelector: podSelector,
				},
			},
			Ports: ports,
		},
	}
	rules, err := manager.parseDefaultSelectors(podSelector, nil, GlobalNamespace1, "ingress")
	assert.Nil(t, err)
	assert.Empty(t, rules)
	ingressChain := manager.parseDefaultIngressRules(ingress, GlobalNamespace1)
	assert.Empty(t, ingressChain.Rule)

	//---------------------------------
	//	Egress
	//---------------------------------

	egress := []networking_v1.NetworkPolicyEgressRule{
		networking_v1.NetworkPolicyEgressRule{
			To: []networking_v1.NetworkPolicyPeer{
				networking_v1.NetworkPolicyPeer{
					PodSelector: podSelector,
				},
			},
			Ports: ports,
		},
	}
	rules, err = manager.parseDefaultSelectors(podSelector, nil, GlobalNamespace1, "egress")
	assert.Nil(t, err)
	assert.Empty(t, rules)
	egressChain := manager.parseDefaultEgressRules(egress, GlobalNamespace1)
	assert.Empty(t, egressChain.Rule)
}

func TestPodMatchExpressionReturnsNoRules(t *testing.T) {
	tcp := core_v1.ProtocolTCP
	port := &intstr.IntOrString{
		IntVal: 6895,
	}
	ports := []networking_v1.NetworkPolicyPort{
		networking_v1.NetworkPolicyPort{
			Port:     port,
			Protocol: &tcp,
		},
	}
	expression := []meta_v1.LabelSelectorRequirement{
		meta_v1.LabelSelectorRequirement{
			Key:      "cache",
			Operator: "In",
			Values:   []string{"redis", "memcached"},
		},
	}
	podSelector := &meta_v1.LabelSelector{
		MatchExpressions: expression,
	}
	testObj := new(MockPodController)
	manager := Init(testObj)

	pods, err := manager.getPodsFromDefaultSelectors(podSelector, nil, GlobalNamespace1)
	assert.Nil(t, pods)
	assert.NotNil(t, err)

	rules, err := manager.parseDefaultSelectors(podSelector, nil, GlobalName1, "ingress")
	assert.Nil(t, rules)
	assert.NotNil(t, err)

	//---------------------------------
	//	Ingress
	//---------------------------------

	ingress := []networking_v1.NetworkPolicyIngressRule{
		networking_v1.NetworkPolicyIngressRule{
			From: []networking_v1.NetworkPolicyPeer{
				networking_v1.NetworkPolicyPeer{
					PodSelector: podSelector,
				},
			},
			Ports: ports,
		},
	}
	ingressChain := manager.parseDefaultIngressRules(ingress, GlobalNamespace1)
	assert.Empty(t, ingressChain.Rule)

	//---------------------------------
	//	Egress
	//---------------------------------

	egress := []networking_v1.NetworkPolicyEgressRule{
		networking_v1.NetworkPolicyEgressRule{
			To: []networking_v1.NetworkPolicyPeer{
				networking_v1.NetworkPolicyPeer{
					PodSelector: podSelector,
				},
			},
			Ports: ports,
		},
	}
	egressChain := manager.parseDefaultEgressRules(egress, GlobalNamespace1)
	assert.Empty(t, egressChain.Rule)
}

func TestEmptyAndNilPodSelectorSelectsAllPodsInSameNamespace(t *testing.T) {
	//	This function just serves to check if query is being correctly built

	podSelector := &meta_v1.LabelSelector{
		MatchLabels: map[string]string{},
	}
	podsFound := []pcn_types.Pod{
		pcn_types.Pod{
			Pod: core_v1.Pod{
				Status: core_v1.PodStatus{
					PodIP: "172.10.30.30",
				},
			},
			Veth: "veth123",
		},
		pcn_types.Pod{
			Pod: core_v1.Pod{
				Status: core_v1.PodStatus{
					PodIP: "172.10.50.30",
				},
			},
			Veth: "veth123",
		},
	}
	testObj := new(MockPodController)
	testObj.On("GetPods", pcn_types.Query{
		Pod: []pcn_types.QueryObject{
			pcn_types.QueryObject{
				By:   "name",
				Name: "*",
			},
		},
		Namespace: []pcn_types.QueryObject{
			pcn_types.QueryObject{
				By:   "name",
				Name: GlobalNamespace1,
			},
		},
	}).Return(podsFound, nil)
	manager := Init(testObj)

	pods, err := manager.getPodsFromDefaultSelectors(podSelector, nil, GlobalNamespace1)
	assert.Nil(t, err)
	assert.NotNil(t, pods)
	assert.ElementsMatch(t, podsFound, pods)

	podSelector.MatchLabels = nil
	pods, err = manager.getPodsFromDefaultSelectors(podSelector, nil, GlobalNamespace1)
	assert.Nil(t, err)
	assert.NotNil(t, pods)
	assert.ElementsMatch(t, podsFound, pods)
}

func TestSelectAllFromSpecificNamespace(t *testing.T) {
	tcp := core_v1.ProtocolTCP
	port := &intstr.IntOrString{
		IntVal: 6895,
	}
	ports := []networking_v1.NetworkPolicyPort{
		networking_v1.NetworkPolicyPort{
			Port:     port,
			Protocol: &tcp,
		},
	}
	podSelector := &meta_v1.LabelSelector{
		MatchLabels: map[string]string{},
	}
	namespaceLabels := map[string]string{
		"environment": "production",
	}
	namespaceSelector := &meta_v1.LabelSelector{
		MatchLabels: namespaceLabels,
	}
	podsFound := []pcn_types.Pod{
		pcn_types.Pod{
			Pod: core_v1.Pod{
				ObjectMeta: meta_v1.ObjectMeta{
					Namespace: GlobalNamespace1,
				},
				Status: core_v1.PodStatus{
					PodIP: "172.10.30.30",
				},
			},
			Veth: "veth123",
		},
		pcn_types.Pod{
			Pod: core_v1.Pod{
				ObjectMeta: meta_v1.ObjectMeta{
					Namespace: GlobalNamespace1,
				},
				Status: core_v1.PodStatus{
					PodIP: "172.10.50.30",
				},
			},
			Veth: "veth123",
		},
	}
	testObj := new(MockPodController)
	testObj.On("GetPods", pcn_types.Query{
		Pod: []pcn_types.QueryObject{
			pcn_types.QueryObject{
				By:   "name",
				Name: "*",
			},
		},
		Namespace: []pcn_types.QueryObject{
			pcn_types.QueryObject{
				By:     "labels",
				Labels: namespaceLabels,
			},
		},
	}).Return(podsFound, nil)
	manager := Init(testObj)

	//---------------------------------
	//	Ingress
	//---------------------------------

	ingress := []networking_v1.NetworkPolicyIngressRule{
		networking_v1.NetworkPolicyIngressRule{
			From: []networking_v1.NetworkPolicyPeer{
				networking_v1.NetworkPolicyPeer{
					PodSelector:       podSelector,
					NamespaceSelector: namespaceSelector,
				},
			},
			Ports: ports,
		},
	}
	//	With whole podselector set to nil...
	ingress1 := []networking_v1.NetworkPolicyIngressRule{
		networking_v1.NetworkPolicyIngressRule{
			From: []networking_v1.NetworkPolicyPeer{
				networking_v1.NetworkPolicyPeer{
					NamespaceSelector: namespaceSelector,
				},
			},
			Ports: ports,
		},
	}
	expectedIngressRules := []k8sfirewall.ChainRule{
		k8sfirewall.ChainRule{
			Src:     podsFound[0].Pod.Status.PodIP,
			Action:  "forward",
			L4proto: "TCP",
			Sport:   int32(port.IntVal),
		},
		k8sfirewall.ChainRule{
			Src:     podsFound[1].Pod.Status.PodIP,
			Action:  "forward",
			L4proto: "TCP",
			Sport:   int32(port.IntVal),
		},
	}
	ingressChain := manager.parseDefaultIngressRules(ingress, GlobalNamespace1)
	assert.ElementsMatch(t, expectedIngressRules, ingressChain.Rule)
	ingressChain = manager.parseDefaultIngressRules(ingress1, GlobalNamespace1)
	assert.ElementsMatch(t, expectedIngressRules, ingressChain.Rule)

	//---------------------------------
	//	Egress
	//---------------------------------

	egress := []networking_v1.NetworkPolicyEgressRule{
		networking_v1.NetworkPolicyEgressRule{
			To: []networking_v1.NetworkPolicyPeer{
				networking_v1.NetworkPolicyPeer{
					PodSelector:       podSelector,
					NamespaceSelector: namespaceSelector,
				},
			},
			Ports: ports,
		},
	}
	egress1 := []networking_v1.NetworkPolicyEgressRule{
		networking_v1.NetworkPolicyEgressRule{
			To: []networking_v1.NetworkPolicyPeer{
				networking_v1.NetworkPolicyPeer{
					NamespaceSelector: namespaceSelector,
				},
			},
			Ports: ports,
		},
	}
	expectedEgressRules := []k8sfirewall.ChainRule{
		k8sfirewall.ChainRule{
			Dst:     podsFound[0].Pod.Status.PodIP,
			Action:  "forward",
			L4proto: "TCP",
			Dport:   int32(port.IntVal),
		},
		k8sfirewall.ChainRule{
			Dst:     podsFound[1].Pod.Status.PodIP,
			Action:  "forward",
			L4proto: "TCP",
			Dport:   int32(port.IntVal),
		},
	}
	egressChain := manager.parseDefaultEgressRules(egress, GlobalNamespace1)
	assert.ElementsMatch(t, expectedEgressRules, egressChain.Rule)
	egressChain = manager.parseDefaultEgressRules(egress1, GlobalNamespace1)
	assert.ElementsMatch(t, expectedEgressRules, egressChain.Rule)
}

func TestBlockAllFromSpecificNamespace(t *testing.T) {
	tcp := core_v1.ProtocolTCP
	port := &intstr.IntOrString{
		IntVal: 6895,
	}
	ports := []networking_v1.NetworkPolicyPort{
		networking_v1.NetworkPolicyPort{
			Port:     port,
			Protocol: &tcp,
		},
	}
	namespaceLabels := map[string]string{
		"environment": "production",
	}
	namespaceSelector := &meta_v1.LabelSelector{
		MatchLabels: namespaceLabels,
	}
	podsFound := []pcn_types.Pod{
		pcn_types.Pod{
			Pod: core_v1.Pod{
				ObjectMeta: meta_v1.ObjectMeta{
					Namespace: GlobalNamespace1,
				},
				Status: core_v1.PodStatus{
					PodIP: "172.10.30.30",
				},
			},
			Veth: "veth123",
		},
		pcn_types.Pod{
			Pod: core_v1.Pod{
				ObjectMeta: meta_v1.ObjectMeta{
					Namespace: GlobalNamespace1,
				},
				Status: core_v1.PodStatus{
					PodIP: "172.10.50.30",
				},
			},
			Veth: "veth123",
		},
	}
	testObj := new(MockPodController)
	testObj.On("GetPods", pcn_types.Query{
		Pod: []pcn_types.QueryObject{
			pcn_types.QueryObject{
				By:   "name",
				Name: "*",
			},
		},
		Namespace: []pcn_types.QueryObject{
			pcn_types.QueryObject{
				By:     "labels",
				Labels: namespaceLabels,
			},
		},
	}).Return(podsFound, nil)
	manager := Init(testObj)

	//---------------------------------
	//	Ingress
	//---------------------------------

	ingress := []networking_v1.NetworkPolicyIngressRule{
		networking_v1.NetworkPolicyIngressRule{
			From: []networking_v1.NetworkPolicyPeer{
				networking_v1.NetworkPolicyPeer{
					PodSelector: &meta_v1.LabelSelector{
						MatchLabels: nil,
					},
					NamespaceSelector: namespaceSelector,
				},
			},
			Ports: ports,
		},
	}
	expectedIngressRules := []k8sfirewall.ChainRule{
		k8sfirewall.ChainRule{
			Src:     podsFound[0].Pod.Status.PodIP,
			Action:  "drop",
			L4proto: "TCP",
			Sport:   int32(port.IntVal),
		},
		k8sfirewall.ChainRule{
			Src:     podsFound[1].Pod.Status.PodIP,
			Action:  "drop",
			L4proto: "TCP",
			Sport:   int32(port.IntVal),
		},
	}
	ingressChain := manager.parseDefaultIngressRules(ingress, GlobalNamespace1)
	assert.ElementsMatch(t, expectedIngressRules, ingressChain.Rule)

	//---------------------------------
	//	Egress
	//---------------------------------

	egress := []networking_v1.NetworkPolicyEgressRule{
		networking_v1.NetworkPolicyEgressRule{
			To: []networking_v1.NetworkPolicyPeer{
				networking_v1.NetworkPolicyPeer{
					PodSelector: &meta_v1.LabelSelector{
						MatchLabels: nil,
					},
					NamespaceSelector: namespaceSelector,
				},
			},
			Ports: ports,
		},
	}
	expectedEgressRules := []k8sfirewall.ChainRule{
		k8sfirewall.ChainRule{
			Dst:     podsFound[0].Pod.Status.PodIP,
			Action:  "drop",
			L4proto: "TCP",
			Dport:   int32(port.IntVal),
		},
		k8sfirewall.ChainRule{
			Dst:     podsFound[1].Pod.Status.PodIP,
			Action:  "drop",
			L4proto: "TCP",
			Dport:   int32(port.IntVal),
		},
	}
	egressChain := manager.parseDefaultEgressRules(egress, GlobalNamespace1)
	assert.ElementsMatch(t, expectedEgressRules, egressChain.Rule)
}

func TestPodsAffected(t *testing.T) {
	podSelector := &meta_v1.LabelSelector{
		MatchLabels: map[string]string{
			"app":   "books-library",
			"cache": "redis",
		},
	}
	podsFound := []pcn_types.Pod{
		pcn_types.Pod{
			Pod: core_v1.Pod{
				ObjectMeta: meta_v1.ObjectMeta{
					Namespace: GlobalNamespace1,
				},
				Status: core_v1.PodStatus{
					PodIP: "172.10.30.30",
				},
			},
			Veth: "veth123",
		},
		pcn_types.Pod{
			Pod: core_v1.Pod{
				ObjectMeta: meta_v1.ObjectMeta{
					Namespace: GlobalNamespace1,
				},
				Status: core_v1.PodStatus{
					PodIP: "172.10.50.30",
				},
			},
			Veth: "veth123",
		},
	}
	testObj := new(MockPodController)
	testObj.On("GetPods", pcn_types.Query{
		Pod: []pcn_types.QueryObject{
			pcn_types.QueryObject{
				By:     "labels",
				Labels: podSelector.MatchLabels,
			},
		},
		Namespace: []pcn_types.QueryObject{
			pcn_types.QueryObject{
				By:   "name",
				Name: GlobalNamespace1,
			},
		},
	}).Return(podsFound, nil)
	expectedPods := map[string][]pcn_types.Pod{
		GlobalNamespace1: podsFound,
	}
	manager := Init(testObj)

	podsAffected, err := manager.getPodsAffectedByDefaultPolicy(podSelector, GlobalNamespace1)
	assert.Nil(t, err)
	assert.Equal(t, expectedPods, podsAffected)

	podsFound = []pcn_types.Pod{
		pcn_types.Pod{
			Pod: core_v1.Pod{
				ObjectMeta: meta_v1.ObjectMeta{
					Namespace: GlobalNamespace1,
				},
				Status: core_v1.PodStatus{
					PodIP: "172.10.30.30",
				},
			},
			Veth: "veth123",
		},
		pcn_types.Pod{
			Pod: core_v1.Pod{
				ObjectMeta: meta_v1.ObjectMeta{
					Namespace: GlobalNamespace1,
				},
				Status: core_v1.PodStatus{
					PodIP: "172.10.50.30",
				},
			},
			Veth: "veth123",
		},
		pcn_types.Pod{
			Pod: core_v1.Pod{
				ObjectMeta: meta_v1.ObjectMeta{
					Namespace: "Beta",
				},
				Status: core_v1.PodStatus{
					PodIP: "172.10.70.30",
				},
			},
			Veth: "veth123",
		},
	}
	testObj.On("GetPods", pcn_types.Query{
		Pod: []pcn_types.QueryObject{
			pcn_types.QueryObject{
				By:     "labels",
				Labels: podSelector.MatchLabels,
			},
		},
		Namespace: []pcn_types.QueryObject{
			pcn_types.QueryObject{
				By:   "name",
				Name: "*",
			},
		},
	}).Return(podsFound, nil)
	expectedPods = map[string][]pcn_types.Pod{
		GlobalNamespace1: []pcn_types.Pod{
			podsFound[0],
			podsFound[1],
		},
		"Beta": []pcn_types.Pod{
			podsFound[2],
		},
	}

	podsAffected, err = manager.getPodsAffectedByDefaultPolicy(podSelector, "*")
	assert.Nil(t, err)
	assert.Equal(t, expectedPods, podsAffected)

	testObj.On("GetPods", pcn_types.Query{
		Pod: []pcn_types.QueryObject{
			pcn_types.QueryObject{
				By:     "labels",
				Labels: podSelector.MatchLabels,
			},
		},
		Namespace: []pcn_types.QueryObject{
			pcn_types.QueryObject{
				By:   "name",
				Name: "Staging",
			},
		},
	}).Return([]pcn_types.Pod{}, nil)
	podsAffected, err = manager.getPodsAffectedByDefaultPolicy(podSelector, "Staging")
	assert.Nil(t, err)
	assert.Empty(t, podsAffected)
}

func TestIds(t *testing.T) {
	testObj := new(MockPodController)
	manager := Init(testObj)

	rules := k8sfirewall.Chain{
		Name: "ingress",
		Rule: []k8sfirewall.ChainRule{
			k8sfirewall.ChainRule{
				L4proto: "TCP",
				Action:  "drop",
				Src:     "192.168.1.1",
			},
			k8sfirewall.ChainRule{
				L4proto: "UDP",
				Action:  "drop",
				Src:     "192.168.1.2",
			},
			k8sfirewall.ChainRule{
				L4proto: "TCP",
				Action:  "drop",
				Src:     "192.16.15.25",
			},
			k8sfirewall.ChainRule{
				L4proto: "UDP",
				Action:  "drop",
				Src:     "192.16.15.25",
			},
		},
	}

	expectedRules := k8sfirewall.Chain{
		Name: "ingress",
		Rule: []k8sfirewall.ChainRule{
			k8sfirewall.ChainRule{
				L4proto: "TCP",
				Action:  "drop",
				Src:     "192.168.1.1",
				Dst:     "192.16.15.25",
			},
			k8sfirewall.ChainRule{
				L4proto: "UDP",
				Action:  "drop",
				Src:     "192.168.1.2",
				Dst:     "192.16.15.25",
			},
		},
	}
	pod := pcn_types.Pod{
		Pod: core_v1.Pod{
			Status: core_v1.PodStatus{
				PodIP: "192.16.15.25",
			},
		},
	}

	genRules := manager.putIPs(pod, rules)
	rules.Rule = genRules
	assert.ElementsMatch(t, expectedRules.Rule, rules.Rule)

	rules = k8sfirewall.Chain{
		Name: "egress",
		Rule: []k8sfirewall.ChainRule{
			k8sfirewall.ChainRule{
				L4proto: "TCP",
				Action:  "drop",
				Dst:     "192.168.1.1",
			},
			k8sfirewall.ChainRule{
				L4proto: "UDP",
				Action:  "drop",
				Dst:     "192.168.1.2",
			},
			k8sfirewall.ChainRule{
				L4proto: "TCP",
				Action:  "drop",
				Dst:     "192.16.15.25",
			},
			k8sfirewall.ChainRule{
				L4proto: "UDP",
				Action:  "drop",
				Dst:     "192.16.15.25",
			},
		},
	}

	expectedRules = k8sfirewall.Chain{
		Name: "egress",
		Rule: []k8sfirewall.ChainRule{
			k8sfirewall.ChainRule{
				L4proto: "TCP",
				Action:  "drop",
				Dst:     "192.168.1.1",

				Src: "192.16.15.25",
			},
			k8sfirewall.ChainRule{
				L4proto: "UDP",
				Action:  "drop",
				Dst:     "192.168.1.2",

				Src: "192.16.15.25",
			},
		},
	}
	pod = pcn_types.Pod{
		Pod: core_v1.Pod{
			Status: core_v1.PodStatus{
				PodIP: "192.16.15.25",
			},
		},
	}

	genRules = manager.putIPs(pod, rules)
	rules.Rule = genRules
	assert.ElementsMatch(t, expectedRules.Rule, rules.Rule)
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
	namespaceLabels := map[string]string{
		"environment": "production",
	}
	podLabels := map[string]string{
		"type":    "app",
		"version": "2.0",
	}
	policy := &networking_v1.NetworkPolicy{
		ObjectMeta: meta_v1.ObjectMeta{
			Name: GlobalName1,
			//Namespace: GlobalNamespace1,
		},
		Spec: networking_v1.NetworkPolicySpec{
			PodSelector: GlobalPodSelector1,
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

	//	In the Production namespace
	productionPods := []pcn_types.Pod{
		pcn_types.Pod{
			Pod: core_v1.Pod{
				ObjectMeta: meta_v1.ObjectMeta{
					Namespace: GlobalNamespace1,
					UID:       "PRODUCTION-POD-UID-1",
				},
				Status: core_v1.PodStatus{
					PodIP: "172.20.20.20",
				},
			},
			Veth: "veth123",
		},
	}

	//	In the Beta namespace
	betaPods := []pcn_types.Pod{
		pcn_types.Pod{
			Pod: core_v1.Pod{
				ObjectMeta: meta_v1.ObjectMeta{
					Namespace: "Beta",
					UID:       "BETA-POD-UID-1",
				},
				Status: core_v1.PodStatus{
					PodIP: "172.40.40.40",
				},
			},
			Veth: "veth512",
		},
	}

	allPods := []pcn_types.Pod{}
	allPods = append(allPods, productionPods...)
	allPods = append(allPods, betaPods...)

	//---------------------------------
	//	Faking the Pod controller
	//---------------------------------

	testObj := new(MockPodController)
	testObj.On("GetPods", pcn_types.Query{
		Pod: []pcn_types.QueryObject{
			pcn_types.QueryObject{
				By:     "labels",
				Labels: GlobalPodSelector1.MatchLabels,
			},
		},
		Namespace: []pcn_types.QueryObject{
			pcn_types.QueryObject{
				By:   "name",
				Name: "*",
			},
		},
	}).Return(allPods, nil)
	testObj.On("GetPods", pcn_types.Query{
		Pod: []pcn_types.QueryObject{
			pcn_types.QueryObject{
				By:   "name",
				Name: "*",
			},
		},
		Namespace: []pcn_types.QueryObject{
			pcn_types.QueryObject{
				By:   "name",
				Name: "*",
			},
		},
	}).Return(allPods, nil)
	testObj.On("GetPods", pcn_types.Query{
		Pod: []pcn_types.QueryObject{
			pcn_types.QueryObject{
				By:     "labels",
				Labels: podLabels,
			},
		},
		Namespace: []pcn_types.QueryObject{
			pcn_types.QueryObject{
				By:     "labels",
				Labels: namespaceLabels,
			},
		},
	}).Return(productionPods, nil)

	manager := Init(testObj)
	manager.DeployNewPolicy(policy)
	deployedFirewalls := manager.deployedFirewalls
	assert.Len(t, deployedFirewalls, 2)

	//---------------------------------
	//	What we expect for the first pod
	//---------------------------------

	//	IngressChain
	expectedIngressChain := k8sfirewall.Chain{
		Name:     "ingress",
		Default_: "drop",
		Rule: []k8sfirewall.ChainRule{
			k8sfirewall.ChainRule{
				L4proto: "TCP",
				Sport:   port1.IntVal,
				Src:     "192.168.0.0/16",
				Dst:     productionPods[0].Pod.Status.PodIP,
				Action:  "forward",
				//Id:      1,
			},
			k8sfirewall.ChainRule{
				L4proto: "UDP",
				Sport:   port1.IntVal,
				Src:     "192.168.0.0/16",
				Dst:     productionPods[0].Pod.Status.PodIP,
				Action:  "forward",
				//Id:      2,
			},
			k8sfirewall.ChainRule{
				L4proto: "TCP",
				Sport:   port1.IntVal,
				Src:     "192.168.2.0/24",
				Dst:     productionPods[0].Pod.Status.PodIP,
				Action:  "drop",
				//Id:      3,
			},
			k8sfirewall.ChainRule{
				L4proto: "UDP",
				Sport:   port1.IntVal,
				Src:     "192.168.2.0/24",
				Dst:     productionPods[0].Pod.Status.PodIP,
				Action:  "drop",
				//Id:      4,
			},
			k8sfirewall.ChainRule{
				L4proto: "TCP",
				Sport:   port1.IntVal,
				Src:     betaPods[0].Pod.Status.PodIP,
				Dst:     productionPods[0].Pod.Status.PodIP,
				Action:  "forward",
				//Id:      5,
			},
			k8sfirewall.ChainRule{
				L4proto: "UDP",
				Sport:   port1.IntVal,
				Src:     betaPods[0].Pod.Status.PodIP,
				Dst:     productionPods[0].Pod.Status.PodIP,
				Action:  "forward",
				//Id:      6,
			},
		},
	}

	productionPod := productionPods[0]
	productionPodFw, ok := deployedFirewalls["fw-"+productionPod.Pod.UID]
	assert.True(t, ok)
	assert.Equal(t, string("fw-"+productionPod.Pod.UID), productionPodFw.firewall.Name)
	/*fmt.Println("---")
	for i := 0; i < len(expectedIngressChain.Rule); i++ {
		fmt.Printf("%+v\n", expectedIngressChain.Rule[i])
		fmt.Printf("%+v\n", productionPodFw.ingressChain.Rule[i])
		fmt.Println("---")
	}*/
	rules, ok := productionPodFw.rules[policy.Name]
	assert.Len(t, productionPodFw.rules, 1)
	assert.True(t, ok)
	assert.ElementsMatch(t, expectedIngressChain.Rule, rules.ingress)

	expectedEgressChain := k8sfirewall.Chain{
		Name:     "ingress",
		Default_: "drop",
		Rule: []k8sfirewall.ChainRule{
			k8sfirewall.ChainRule{
				L4proto: "TCP",
				Dport:   port2.IntVal,
				Dst:     "192.169.0.0/16",
				Src:     productionPods[0].Pod.Status.PodIP,
				Action:  "forward",
				//Id:      1,
			},
			k8sfirewall.ChainRule{
				L4proto: "TCP",
				Dport:   port3.IntVal,
				Dst:     "192.169.0.0/16",
				Src:     productionPods[0].Pod.Status.PodIP,
				Action:  "forward",
				//Id:      2,
			},
			k8sfirewall.ChainRule{
				L4proto: "TCP",
				Dport:   port2.IntVal,
				Dst:     "192.169.2.0/24",
				Src:     productionPods[0].Pod.Status.PodIP,
				Action:  "drop",
				//Id:      3,
			},
			k8sfirewall.ChainRule{
				L4proto: "TCP",
				Dport:   port3.IntVal,
				Dst:     "192.169.2.0/24",
				Src:     productionPods[0].Pod.Status.PodIP,
				Action:  "drop",
				//Id:      4,
			},
			k8sfirewall.ChainRule{
				L4proto: "TCP",
				Dport:   port4.IntVal,
				Src:     productionPods[0].Pod.Status.PodIP,
				Action:  "forward",
				//Id:      5,
			},
			k8sfirewall.ChainRule{
				L4proto: "UDP",
				Dport:   port4.IntVal,
				Src:     productionPods[0].Pod.Status.PodIP,
				Action:  "forward",
				//Id:      6,
			},
		},
	}

	/*fmt.Println("---")
	for i := 0; i < len(expectedEgressChain.Rule); i++ {
		fmt.Printf("%+v\n", expectedEgressChain.Rule[i])
		fmt.Printf("%+v\n", productionPodFw.egressChain.Rule[i])
		fmt.Println("---")
	}*/
	assert.ElementsMatch(t, expectedEgressChain.Rule, rules.egress)
	expecteduids := []k8s_types.UID{k8s_types.UID("PRODUCTION-POD-UID-1"), k8s_types.UID("BETA-POD-UID-1")}
	policyRulesids := manager.deployedPolicies
	assert.Len(t, policyRulesids, 1)
	assert.ElementsMatch(t, expecteduids, policyRulesids[policy.Name])
}

func TestRemovePolicy(t *testing.T) {
	testObj := new(MockPodController)
	manager := Init(testObj)

	policyToDelete := "delete"
	ingressChain := k8sfirewall.Chain{
		Name: "ingress",
		Rule: []k8sfirewall.ChainRule{
			k8sfirewall.ChainRule{
				Id: 1,
			},
			k8sfirewall.ChainRule{
				Id: 2,
			},
			k8sfirewall.ChainRule{
				Id: 3,
			},
			k8sfirewall.ChainRule{
				Id: 4,
			},
		},
	}
	egressChain := k8sfirewall.Chain{
		Name: "egress",
		Rule: []k8sfirewall.ChainRule{
			k8sfirewall.ChainRule{
				Id: 1,
			},
			k8sfirewall.ChainRule{
				Id: 2,
			},
			k8sfirewall.ChainRule{
				Id: 3,
			},
			k8sfirewall.ChainRule{
				Id: 4,
			},
			k8sfirewall.ChainRule{
				Id: 5,
			},
		},
	}
	firewallDeployed := deployedFirewall{
		firewall: &k8sfirewall.Firewall{},
		rules: map[string]*policyRules{
			"keep": &policyRules{
				ingress: []k8sfirewall.ChainRule{
					ingressChain.Rule[0],
					ingressChain.Rule[3],
				},
				egress: []k8sfirewall.ChainRule{
					egressChain.Rule[0],
					egressChain.Rule[1],
					egressChain.Rule[4],
				},
			},
			policyToDelete: &policyRules{
				ingress: ingressChain.Rule[1:2],
				egress:  egressChain.Rule[2:3],
			},
		},
		ingressChain: &ingressChain,
		egressChain:  &egressChain,
	}
	manager.deployedFirewalls[k8s_types.UID("fw-"+"POD")] = &firewallDeployed
	manager.deployedPolicies[policyToDelete] = []k8s_types.UID{
		k8s_types.UID("fw-" + "POD"),
	}
	policy := networking_v1.NetworkPolicy{
		ObjectMeta: meta_v1.ObjectMeta{
			Name: policyToDelete,
		},
	}

	manager.RemovePolicy(&policy)
	result := manager.deployedFirewalls[k8s_types.UID("fw-POD")]
	assert.Len(t, result.rules, 1)
	_, exists := result.rules[policy.Name]
	assert.False(t, exists)
}
