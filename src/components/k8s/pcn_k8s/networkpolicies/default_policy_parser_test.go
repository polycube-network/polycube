package networkpolicies

import (
	"testing"

	core_v1 "k8s.io/api/core/v1"
	networking_v1 "k8s.io/api/networking/v1"
	meta_v1 "k8s.io/apimachinery/pkg/apis/meta/v1"
	"k8s.io/apimachinery/pkg/util/intstr"

	pcn_controllers "github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/controllers"
	pcn_types "github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/types"
	k8sfirewall "github.com/SunSince90/polycube/src/components/k8s/utils/k8sfirewall"
	"github.com/stretchr/testify/assert"
)

func Init(podController pcn_controllers.PodController) *DefaultPolicyParser {
	manager := newDefaultPolicyParser(podController, nil, "node")
	return manager
}

func TestNotNil(t *testing.T) {
	manager := Init(nil)
	assert.NotNil(t, manager)
}

func TestNoSpec(t *testing.T) {

	testObj := new(MockPodController)
	manager := Init(testObj)

	ingress, egress := manager.ParsePolicyTypes(nil)

	assert.Zero(t, len(ingress))
	assert.Zero(t, len(egress))

	result := manager.ParseRules(ingress, egress, "ns")

	assert.Zero(t, len(result.Ingress))
	assert.Zero(t, len(result.Egress))
}

func TestPolicyTypes(t *testing.T) {
	testObj := new(MockPodController)
	manager := Init(testObj)

	spec := &networking_v1.NetworkPolicySpec{
		Ingress: []networking_v1.NetworkPolicyIngressRule{},
	}
	ingress, egress := manager.ParsePolicyTypes(spec)
	assert.NotNil(t, ingress)
	assert.Empty(t, ingress)
	assert.Nil(t, egress)

	spec = &networking_v1.NetworkPolicySpec{
		PolicyTypes: []networking_v1.PolicyType{
			networking_v1.PolicyTypeIngress,
		},
		Ingress: []networking_v1.NetworkPolicyIngressRule{},
	}
	ingress, egress = manager.ParsePolicyTypes(spec)
	assert.NotNil(t, ingress)
	assert.Empty(t, ingress)
	assert.Nil(t, egress)

	//	When you only have egress, you MUST specify it, otherwise it is incorrectly parsed
	spec = &networking_v1.NetworkPolicySpec{
		Egress: []networking_v1.NetworkPolicyEgressRule{},
	}
	ingress, egress = manager.ParsePolicyTypes(spec)
	assert.Nil(t, ingress)
	assert.Empty(t, ingress)
	assert.Nil(t, egress)

	spec = &networking_v1.NetworkPolicySpec{
		PolicyTypes: []networking_v1.PolicyType{
			networking_v1.PolicyTypeEgress,
		},
		Egress: []networking_v1.NetworkPolicyEgressRule{},
	}
	ingress, egress = manager.ParsePolicyTypes(spec)
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
	ingress, egress = manager.ParsePolicyTypes(spec)
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

	result := manager.ParseRules(ingress, nil, "ns")

	assert.Zero(t, len(result.Ingress))
	assert.Zero(t, len(result.Egress))
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

	result := manager.ParseRules(ingress, nil, "ns")

	assert.Zero(t, len(result.Ingress))

	//	This test should change the default action to forward, but unfortunately this does not happen
	// as it is not possible to do that on k8sfirewall
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
	result := manager.ParseRules(ingress, nil, "ns")

	assert.Zero(t, len(result.Ingress))
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

	result := manager.ParseRules(nil, egress, "ns")

	assert.Zero(t, len(result.Egress))
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

	result := manager.ParseRules(nil, egress, "ns")
	assert.Zero(t, len(result.Egress))
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
	expectedIngress := []k8sfirewall.ChainRule{
		k8sfirewall.ChainRule{
			Src:    ipBlock.CIDR,
			Action: "forward",
		},
	}
	expectedEgress := []k8sfirewall.ChainRule{
		k8sfirewall.ChainRule{
			Dst:    ipBlock.CIDR,
			Action: "forward",
		},
	}

	result := manager.ParseIPBlock(ipBlock, "ingress")
	assert.ElementsMatch(t, expectedIngress, result.Ingress)
	assert.ElementsMatch(t, expectedEgress, result.Egress)
	result = manager.ParseIngress(ingress, "ns")
	assert.Len(t, result.Ingress, 1)
	assert.Len(t, result.Egress, 1)
	assert.Equal(t, expectedIngress, result.Ingress)
	assert.Equal(t, expectedEgress, result.Egress)

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
	expectedIngress = []k8sfirewall.ChainRule{
		k8sfirewall.ChainRule{
			Src:    ipBlock.CIDR,
			Action: "forward",
		},
	}
	expectedEgress = []k8sfirewall.ChainRule{
		k8sfirewall.ChainRule{
			Dst:    ipBlock.CIDR,
			Action: "forward",
		},
	}

	result = manager.ParseIPBlock(ipBlock, "egress")
	assert.ElementsMatch(t, expectedIngress, result.Ingress)
	assert.ElementsMatch(t, expectedEgress, result.Egress)
	result = manager.ParseEgress(egress, "ns")
	assert.Len(t, result.Ingress, 1)
	assert.Len(t, result.Egress, 1)
	assert.Equal(t, expectedIngress, result.Ingress)
	assert.Equal(t, expectedEgress, result.Egress)
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
	expectedIngress := []k8sfirewall.ChainRule{
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
	expectedEgress := []k8sfirewall.ChainRule{
		k8sfirewall.ChainRule{
			Dst:    ipBlock.CIDR,
			Action: "forward",
		},
	}

	result := manager.ParseIPBlock(ipBlock, "ingress")
	assert.ElementsMatch(t, expectedEgress, result.Egress)
	assert.ElementsMatch(t, expectedIngress, result.Ingress)
	result = manager.ParseIngress(ingress, "ns")
	assert.ElementsMatch(t, expectedEgress, result.Egress)
	assert.ElementsMatch(t, expectedIngress, result.Ingress)

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
	expectedEgress = []k8sfirewall.ChainRule{
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
	expectedIngress = []k8sfirewall.ChainRule{
		k8sfirewall.ChainRule{
			Src:    ipBlock.CIDR,
			Action: "forward",
		},
	}

	result = manager.ParseIPBlock(ipBlock, "egress")
	assert.ElementsMatch(t, expectedEgress, result.Egress)
	assert.ElementsMatch(t, expectedIngress, result.Ingress)
	result = manager.ParseEgress(egress, "ns")
	assert.ElementsMatch(t, expectedEgress, result.Egress)
	assert.ElementsMatch(t, expectedIngress, result.Ingress)
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
	expectedIngress := []k8sfirewall.ChainRule{
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
	expectedEgress := []k8sfirewall.ChainRule{
		k8sfirewall.ChainRule{
			Dst:    ipBlock.CIDR,
			Action: "forward",
		},
	}

	result := manager.ParseIPBlock(ipBlock, "ingress")
	assert.ElementsMatch(t, expectedIngress, result.Ingress)
	assert.ElementsMatch(t, expectedEgress, result.Egress)
	result = manager.ParseIngress(ingress, "ns")
	assert.Len(t, result.Ingress, len(expectedIngress))
	assert.Len(t, result.Egress, len(expectedEgress))
	for _, r := range result.Ingress {
		assert.Equal(t, "TCP", r.L4proto)
	}
	for _, r := range result.Egress {
		assert.Equal(t, "TCP", r.L4proto)
	}

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
	expectedEgress = []k8sfirewall.ChainRule{
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
	expectedIngress = []k8sfirewall.ChainRule{
		k8sfirewall.ChainRule{
			Src:    ipBlock.CIDR,
			Action: "forward",
		},
	}

	result = manager.ParseIPBlock(ipBlock, "egress")
	assert.ElementsMatch(t, expectedIngress, result.Ingress)
	assert.ElementsMatch(t, expectedEgress, result.Egress)
	result = manager.ParseEgress(egress, "ns")
	assert.Len(t, result.Ingress, len(expectedIngress))
	assert.Len(t, result.Egress, len(expectedEgress))
	for _, r := range result.Ingress {
		assert.Equal(t, "TCP", r.L4proto)
	}
	for _, r := range result.Egress {
		assert.Equal(t, "TCP", r.L4proto)
	}
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
	expectedIngress := []k8sfirewall.ChainRule{
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
	expectedEgress := []k8sfirewall.ChainRule{
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
	}

	result := manager.ParseIngress(ingress, "ns")
	assert.ElementsMatch(t, expectedIngress, result.Ingress)
	assert.ElementsMatch(t, expectedEgress, result.Egress)

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
	expectedEgress = []k8sfirewall.ChainRule{
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
	expectedIngress = []k8sfirewall.ChainRule{
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
	}

	result = manager.ParseEgress(egress, "ns")
	assert.ElementsMatch(t, expectedIngress, result.Ingress)
	assert.ElementsMatch(t, expectedEgress, result.Egress)
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
	parsedPP := manager.ParsePorts(ports)
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
	expectedIngress := []k8sfirewall.ChainRule{
		k8sfirewall.ChainRule{
			Src:     ipBlock.CIDR,
			Action:  "forward",
			L4proto: "TCP",
			Sport:   int32(port.IntVal),
		},
	}
	expectedEgress := []k8sfirewall.ChainRule{
		k8sfirewall.ChainRule{
			Dst:     ipBlock.CIDR,
			Action:  "forward",
			L4proto: "TCP",
			Dport:   int32(port.IntVal),
		},
	}

	result := manager.ParseIngress(ingress, "ns")
	assert.ElementsMatch(t, expectedIngress, result.Ingress)
	assert.ElementsMatch(t, expectedEgress, result.Egress)

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
	assert.ElementsMatch(t, expectedIngress, result.Ingress)
	assert.ElementsMatch(t, expectedEgress, result.Egress)
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
	expectedIngress := []k8sfirewall.ChainRule{
		k8sfirewall.ChainRule{
			Src:    ipBlock.CIDR,
			Action: "forward",
			Sport:  int32(port.IntVal),
		},
		k8sfirewall.ChainRule{
			Src:    ipBlock.Except[0],
			Action: "drop",
			Sport:  int32(port.IntVal),
		},
	}
	expectedEgress := []k8sfirewall.ChainRule{
		k8sfirewall.ChainRule{
			Dst:    ipBlock.CIDR,
			Action: "forward",
			Dport:  int32(port.IntVal),
		},
	}

	result := manager.ParseIngress(ingress, "ns")
	assert.ElementsMatch(t, expectedIngress, result.Ingress)
	assert.ElementsMatch(t, expectedEgress, result.Egress)

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
	expectedEgress = []k8sfirewall.ChainRule{
		k8sfirewall.ChainRule{
			Dst:    ipBlock.CIDR,
			Action: "forward",
			Dport:  int32(port.IntVal),
		},
		k8sfirewall.ChainRule{
			Dst:    ipBlock.Except[0],
			Action: "drop",
			Dport:  int32(port.IntVal),
		},
	}
	expectedIngress = []k8sfirewall.ChainRule{
		k8sfirewall.ChainRule{
			Src:    ipBlock.CIDR,
			Action: "forward",
			Sport:  int32(port.IntVal),
		},
	}

	result = manager.ParseEgress(egress, "ns")
	assert.ElementsMatch(t, expectedIngress, result.Ingress)
	assert.ElementsMatch(t, expectedEgress, result.Egress)
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
	testObj.On("GetPods", pcn_types.PodQuery{
		Pod: pcn_types.PodQueryObject{
			By:     "labels",
			Labels: labels,
		},
		Namespace: pcn_types.PodQueryObject{
			By:   "name",
			Name: ProductionNamespace,
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
	expectedResult := pcn_types.ParsedRules{
		Ingress: []k8sfirewall.ChainRule{
			k8sfirewall.ChainRule{
				Src: podsFound[0].Pod.Status.PodIP,
				//L4proto: "TCP",
				//Dst: int32(port.IntVal),
				Action: "forward",
			},
			k8sfirewall.ChainRule{
				Src: podsFound[1].Pod.Status.PodIP,
				//L4proto: "TCP",
				//Dst: int32(port.IntVal),
				Action: "forward",
			},
		},
		Egress: []k8sfirewall.ChainRule{
			k8sfirewall.ChainRule{
				Dst: podsFound[0].Pod.Status.PodIP,
				//L4proto: "TCP",
				//Sport: int32(port.IntVal),
				Action: "forward",
			},
			k8sfirewall.ChainRule{
				Dst: podsFound[1].Pod.Status.PodIP,
				//L4proto: "TCP",
				//Sport: int32(port.IntVal),
				Action: "forward",
			},
		},
	}

	result, err := manager.ParseSelectors(podSelector, nil, ProductionNamespace)
	assert.Nil(t, err)
	assert.Equal(t, expectedResult, result)

	result = manager.ParseIngress(ingress, ProductionNamespace)
	withProtocols := expectedResult
	for i := 0; i < len(withProtocols.Ingress); i++ {
		withProtocols.Ingress[i].L4proto = "TCP"
		withProtocols.Ingress[i].Sport = int32(port.IntVal)
	}
	for i := 0; i < len(withProtocols.Egress); i++ {
		withProtocols.Egress[i].L4proto = "TCP"
		withProtocols.Egress[i].Dport = int32(port.IntVal)
	}
	assert.Equal(t, withProtocols, result)

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

	result = manager.ParseEgress(egress, ProductionNamespace)
	withProtocols = expectedResult
	for i := 0; i < len(withProtocols.Ingress); i++ {
		withProtocols.Ingress[i].L4proto = "TCP"
		withProtocols.Ingress[i].Sport = int32(port.IntVal)
	}
	for i := 0; i < len(withProtocols.Egress); i++ {
		withProtocols.Egress[i].L4proto = "TCP"
		withProtocols.Egress[i].Dport = int32(port.IntVal)
	}
	assert.Equal(t, withProtocols, result)
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
	testObj.On("GetPods", pcn_types.PodQuery{
		Pod: pcn_types.PodQueryObject{
			By:     "labels",
			Labels: labels,
		},
		Namespace: pcn_types.PodQueryObject{
			By:   "name",
			Name: ProductionNamespace,
		},
	}).Return(podsFound, nil)
	manager := Init(testObj)

	result, err := manager.ParseSelectors(podSelector, nil, ProductionNamespace)
	assert.Nil(t, err)
	assert.Empty(t, result.Ingress)
	assert.Empty(t, result.Egress)

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

	result = manager.ParseIngress(ingress, ProductionNamespace)
	assert.Empty(t, result.Ingress)
	assert.Empty(t, result.Egress)

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

	result = manager.ParseEgress(egress, ProductionNamespace)
	assert.Empty(t, result.Ingress)
	assert.Empty(t, result.Egress)
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

	pods, err := manager.getPodsFromSelectors(podSelector, nil, ProductionNamespace)
	assert.Nil(t, pods)
	assert.NotNil(t, err)

	result, err := manager.ParseSelectors(podSelector, nil, TestDefaultPolicyName)
	assert.Empty(t, result.Ingress)
	assert.Empty(t, result.Egress)
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
	result = manager.ParseIngress(ingress, ProductionNamespace)
	assert.Empty(t, result.Ingress)
	assert.Empty(t, result.Egress)

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
	result = manager.ParseEgress(egress, ProductionNamespace)
	assert.Empty(t, result.Ingress)
	assert.Empty(t, result.Egress)
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
	testObj.On("GetPods", pcn_types.PodQuery{
		Pod: pcn_types.PodQueryObject{
			By:   "name",
			Name: "*",
		},
		Namespace: pcn_types.PodQueryObject{
			By:   "name",
			Name: ProductionNamespace,
		},
	}).Return(podsFound, nil)
	manager := Init(testObj)

	pods, err := manager.getPodsFromSelectors(podSelector, nil, ProductionNamespace)
	assert.Nil(t, err)
	assert.NotNil(t, pods)
	assert.ElementsMatch(t, podsFound, pods)

	podSelector.MatchLabels = nil
	pods, err = manager.getPodsFromSelectors(podSelector, nil, ProductionNamespace)
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
					Namespace: ProductionNamespace,
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
					Namespace: ProductionNamespace,
				},
				Status: core_v1.PodStatus{
					PodIP: "172.10.50.30",
				},
			},
			Veth: "veth123",
		},
	}
	testObj := new(MockPodController)
	testObj.On("GetPods", pcn_types.PodQuery{
		Pod: pcn_types.PodQueryObject{
			By:   "name",
			Name: "*",
		},
		Namespace: pcn_types.PodQueryObject{
			By:     "labels",
			Labels: namespaceLabels,
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
	expectedResult := pcn_types.ParsedRules{
		Ingress: []k8sfirewall.ChainRule{
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
		},
		Egress: []k8sfirewall.ChainRule{
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
		},
	}

	result := manager.ParseIngress(ingress, ProductionNamespace)
	assert.Equal(t, expectedResult, result)
	result = manager.ParseIngress(ingress1, ProductionNamespace)
	assert.Equal(t, expectedResult, result)

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

	result = manager.ParseEgress(egress, ProductionNamespace)
	assert.Equal(t, expectedResult, result)
	result = manager.ParseEgress(egress1, ProductionNamespace)
	assert.Equal(t, expectedResult, result)
}

func TestPodsAffected(t *testing.T) {
	//	From a specific namespace
	podSelector := meta_v1.LabelSelector{
		MatchLabels: map[string]string{
			"app":   "books-library",
			"cache": "redis",
		},
	}
	podsFound := []pcn_types.Pod{
		pcn_types.Pod{
			Pod: core_v1.Pod{
				ObjectMeta: meta_v1.ObjectMeta{
					Namespace: ProductionNamespace,
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
					Namespace: ProductionNamespace,
				},
				Status: core_v1.PodStatus{
					PodIP: "172.10.50.30",
				},
			},
			Veth: "veth123",
		},
	}
	testObj := new(MockPodController)
	testObj.On("GetPods", pcn_types.PodQuery{
		Pod: pcn_types.PodQueryObject{
			By:     "labels",
			Labels: podSelector.MatchLabels,
		},
		Namespace: pcn_types.PodQueryObject{
			By:   "name",
			Name: ProductionNamespace,
		},
	}).Return(podsFound, nil)
	expectedPods := map[string][]pcn_types.Pod{
		ProductionNamespace: podsFound,
	}
	manager := Init(testObj)
	policy := networking_v1.NetworkPolicy{
		ObjectMeta: meta_v1.ObjectMeta{
			Namespace: ProductionNamespace,
		},
		Spec: networking_v1.NetworkPolicySpec{
			PodSelector: podSelector,
		},
	}
	podsAffected, err := manager.GetPodsAffected(&policy)
	assert.Nil(t, err)
	assert.Equal(t, expectedPods, podsAffected)

	//	All namespaces
	podsFound = []pcn_types.Pod{
		pcn_types.Pod{
			Pod: core_v1.Pod{
				ObjectMeta: meta_v1.ObjectMeta{
					Namespace: ProductionNamespace,
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
					Namespace: ProductionNamespace,
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
					Namespace: BetaNamespace,
				},
				Status: core_v1.PodStatus{
					PodIP: "172.10.70.30",
				},
			},
			Veth: "veth123",
		},
	}
	testObj.On("GetPods", pcn_types.PodQuery{
		Pod: pcn_types.PodQueryObject{
			By:     "labels",
			Labels: podSelector.MatchLabels,
		},
		Namespace: pcn_types.PodQueryObject{
			By:   "name",
			Name: "*",
		},
	}).Return(podsFound, nil)
	expectedPods = map[string][]pcn_types.Pod{
		ProductionNamespace: []pcn_types.Pod{
			podsFound[0],
			podsFound[1],
		},
		BetaNamespace: []pcn_types.Pod{
			podsFound[2],
		},
	}
	policy = networking_v1.NetworkPolicy{
		//ObjectMeta: meta_v1.ObjectMeta{
		//	Namespace: ProductionNamespace,
		//},
		Spec: networking_v1.NetworkPolicySpec{
			PodSelector: podSelector,
		},
	}
	podsAffected, err = manager.GetPodsAffected(&policy)
	assert.Nil(t, err)
	assert.Equal(t, expectedPods, podsAffected)

	policy = networking_v1.NetworkPolicy{
		ObjectMeta: meta_v1.ObjectMeta{
			Namespace: StagingNamespace,
		},
		Spec: networking_v1.NetworkPolicySpec{
			PodSelector: podSelector,
		},
	}
	testObj.On("GetPods", pcn_types.PodQuery{
		Pod: pcn_types.PodQueryObject{
			By:     "labels",
			Labels: podSelector.MatchLabels,
		},
		Namespace: pcn_types.PodQueryObject{
			By:   "name",
			Name: StagingNamespace,
		},
	}).Return([]pcn_types.Pod{}, nil)
	podsAffected, err = manager.GetPodsAffected(&policy)
	assert.Nil(t, err)
	assert.Empty(t, podsAffected)
}

func TestIPs(t *testing.T) {
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

	genRules := manager.putIPs(pod, rules.Rule, "ingress")
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

	genRules = manager.putIPs(pod, rules.Rule, "egress")
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
			Name: TestDefaultPolicyName,
			//Namespace: ProductionNamespace,
		},
		Spec: networking_v1.NetworkPolicySpec{
			PodSelector: AppPodSelector,
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

	allPods := []pcn_types.Pod{}
	allPods = append(allPods, ProductionPods...)
	allPods = append(allPods, BetaPods...)

	//---------------------------------
	//	Faking the Pod controller
	//---------------------------------

	testObj := new(MockPodController)
	testObj.On("GetPods", pcn_types.PodQuery{
		Pod: pcn_types.PodQueryObject{
			By:     "labels",
			Labels: AppPodSelector.MatchLabels,
		},
		Namespace: pcn_types.PodQueryObject{
			By:   "name",
			Name: "*",
		},
	}).Return(allPods, nil)
	testObj.On("GetPods", pcn_types.PodQuery{
		Pod: pcn_types.PodQueryObject{
			By:   "name",
			Name: "*",
		},
		Namespace: pcn_types.PodQueryObject{
			By:   "name",
			Name: "*",
		},
	}).Return(allPods, nil)
	testObj.On("GetPods", pcn_types.PodQuery{
		Pod: pcn_types.PodQueryObject{
			By:     "labels",
			Labels: podLabels,
		},
		Namespace: pcn_types.PodQueryObject{
			By:     "labels",
			Labels: namespaceLabels,
		},
	}).Return(ProductionPods, nil)

	manager := Init(testObj)

	nsPods, _ := manager.GetPodsAffected(policy)
	assert.Len(t, nsPods, 2)

	productionNsPods, exists := nsPods[ProductionNamespace]
	assert.True(t, exists)
	assert.Len(t, productionNsPods, len(ProductionPods))

	spec := policy.Spec
	ingress, egress := manager.ParsePolicyTypes(&spec)

	firstResult := manager.ParseRules(ingress, egress, ProductionNamespace)

	//---------------------------------
	//	What we expect for the first pod
	//---------------------------------

	productionPod := ProductionPods[0]
	firstResult = manager.FillChains(productionPod, firstResult.Ingress, firstResult.Egress)

	expectedResult := pcn_types.ParsedRules{}

	//	Ingress
	expectedResult.Ingress = []k8sfirewall.ChainRule{
		k8sfirewall.ChainRule{
			L4proto: "TCP",
			Sport:   port1.IntVal,
			Src:     "192.168.2.0/24",
			Dst:     productionPod.Pod.Status.PodIP,
			Action:  "drop",
		},
		k8sfirewall.ChainRule{
			L4proto: "UDP",
			Sport:   port1.IntVal,
			Src:     "192.168.2.0/24",
			Dst:     productionPod.Pod.Status.PodIP,
			Action:  "drop",
		},
		k8sfirewall.ChainRule{
			L4proto: "TCP",
			Sport:   port1.IntVal,
			Src:     "192.168.0.0/16",
			Dst:     productionPod.Pod.Status.PodIP,
			Action:  "forward",
		},
		k8sfirewall.ChainRule{
			L4proto: "UDP",
			Sport:   port1.IntVal,
			Src:     "192.168.0.0/16",
			Dst:     productionPod.Pod.Status.PodIP,
			Action:  "forward",
		},
		k8sfirewall.ChainRule{
			L4proto: "TCP",
			Sport:   port1.IntVal,
			Src:     BetaPods[0].Pod.Status.PodIP,
			Dst:     productionPod.Pod.Status.PodIP,
			Action:  "forward",
		},
		k8sfirewall.ChainRule{
			L4proto: "UDP",
			Sport:   port1.IntVal,
			Src:     BetaPods[0].Pod.Status.PodIP,
			Dst:     productionPod.Pod.Status.PodIP,
			Action:  "forward",
		},
		//	2
		k8sfirewall.ChainRule{
			L4proto: "TCP",
			Sport:   port2.IntVal,
			Src:     "192.169.0.0/16",
			Dst:     productionPod.Pod.Status.PodIP,
			Action:  "forward",
		},
		k8sfirewall.ChainRule{
			L4proto: "TCP",
			Sport:   port3.IntVal,
			Src:     "192.169.0.0/16",
			Dst:     productionPod.Pod.Status.PodIP,
			Action:  "forward",
		},
		//3
		k8sfirewall.ChainRule{
			L4proto: "TCP",
			Sport:   port4.IntVal,
			Dst:     productionPod.Pod.Status.PodIP,
			Action:  "forward",
		},
		k8sfirewall.ChainRule{
			L4proto: "UDP",
			Sport:   port4.IntVal,
			Dst:     productionPod.Pod.Status.PodIP,
			Action:  "forward",
		},
	}
	assert.ElementsMatch(t, expectedResult.Ingress, firstResult.Ingress)

	//	Egress

	expectedResult.Egress = []k8sfirewall.ChainRule{
		k8sfirewall.ChainRule{
			L4proto: "TCP",
			Dport:   port1.IntVal,
			Dst:     "192.168.0.0/16",
			Src:     productionPod.Pod.Status.PodIP,
			Action:  "forward",
		},
		k8sfirewall.ChainRule{
			L4proto: "UDP",
			Dport:   port1.IntVal,
			Dst:     "192.168.0.0/16",
			Src:     productionPod.Pod.Status.PodIP,
			Action:  "forward",
		},
		k8sfirewall.ChainRule{
			L4proto: "TCP",
			Dport:   port1.IntVal,
			Dst:     BetaPods[0].Pod.Status.PodIP,
			Src:     productionPod.Pod.Status.PodIP,
			Action:  "forward",
		},
		k8sfirewall.ChainRule{
			L4proto: "UDP",
			Dport:   port1.IntVal,
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
	assert.ElementsMatch(t, expectedResult.Ingress, firstResult.Ingress)
}

func TestIsPodAffected(t *testing.T) {
	//---------------------------------
	//	The Policy
	//---------------------------------

	/*tcp := core_v1.ProtocolTCP
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
	}*/
	/*testLabels := map[string]string{
		"test1": "1",
		"test2": "2",
	}
	testLabels1 := map[string]string{
		"test1": "1",
		"test2": "2",
		"test3": "3",
	}
	policy := &networking_v1.NetworkPolicy{
		ObjectMeta: meta_v1.ObjectMeta{
			Name: TestDefaultPolicyName,
			//Namespace: ProductionNamespace,
		},
		Spec: networking_v1.NetworkPolicySpec{
			PodSelector: meta_v1.LabelSelector{
				MatchLabels: testLabels,
			},
		},
	}

	//---------------------------------
	//	The pods in this fake cluster
	//---------------------------------

	allPods := []pcn_types.Pod{}
	testPod := pcn_types.Pod{
		Pod: core_v1.Pod{
			ObjectMeta: meta_v1.ObjectMeta{
				Namespace: ProductionNamespace,
				UID:       "PRODUCTION-POD-UID-1",
				Labels:    testLabels,
			},
			Status: core_v1.PodStatus{
				PodIP: "172.20.20.20",
			},
		},
		Veth: "veth123",
	}
	testPod1 := pcn_types.Pod{
		Pod: core_v1.Pod{
			ObjectMeta: meta_v1.ObjectMeta{
				Namespace: ProductionNamespace,
				UID:       "PRODUCTION-POD-UID-2",
				Labels:    testLabels1,
			},
			Status: core_v1.PodStatus{
				PodIP: "172.20.20.20",
			},
		},
		Veth: "veth123",
	}

	allPods = append(allPods, testPod)

	//---------------------------------
	//	Faking the Pod controller
	//---------------------------------

	testObj := new(MockPodController)
	testObj.On("GetPods", pcn_types.PodQuery{
		Pod: pcn_types.PodQueryObject{
			By:     "labels",
			Labels: testLabels,
		},
		Namespace: pcn_types.PodQueryObject{
			By:   "name",
			Name: "*",
		},
	}).Return([]pcn_types.Pod{allPods[0]}, nil)
	testObj.On("GetPods", pcn_types.PodQuery{
		Pod: pcn_types.PodQueryObject{
			By:     "labels",
			Labels: testLabels1,
		},
		Namespace: pcn_types.PodQueryObject{
			By:   "name",
			Name: "*",
		},
	}).Return([]pcn_types.Pod{}, nil)

	manager := Init(testObj)

	result := manager.DoesPolicyAffectPod(policy, &testPod.Pod)
	assert.True(t, result)

	result = manager.DoesPolicyAffectPod(policy, &testPod1.Pod)
	assert.False(t, result)*/
}

func TestPolicyTargetsPod(t *testing.T) {
	//---------------------------------
	//	The Policy
	//---------------------------------

	/*tcp := core_v1.ProtocolTCP
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
			Name: TestDefaultPolicyName,
			//Namespace: ProductionNamespace,
		},
		Spec: networking_v1.NetworkPolicySpec{
			PodSelector: AppPodSelector,
			PolicyTypes: []networking_v1.PolicyType{
				networking_v1.PolicyTypeIngress,
				networking_v1.PolicyTypeEgress,
			},
			Ingress: []networking_v1.NetworkPolicyIngressRule{*/
	/*
		Ingress - First rule
		Block the following if it comes with TCP6895 Or UDP6895
			- what comes from 192.168.2.0/24

		Allow the following if it comes with TCP6895 & UDP6895
			- what comes from 192.168.0.0/16 (see exceptions above)
			- All pods that belong to all namespaces
	*/
	/*networking_v1.NetworkPolicyIngressRule{
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
	Egress: []networking_v1.NetworkPolicyEgressRule{*/
	/*
		Egress - First Rule
		Block the following if it comes with TCP8080 Or TCP80:
			- what comes from 192.169.2.0/24

		Allow the following if it comes with TCP8080 Or TCP80:
			- what comes from 192.169.0.0/16 (see exceptions above)
			- all pods with labels podLabels that belong to certain namespace (namespaceLabels)
	*/
	/*networking_v1.NetworkPolicyEgressRule{
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
	},*/
	/*
		Egress - Second Rule
		Allow everything that goes to port 5000
	*/
	/*networking_v1.NetworkPolicyEgressRule{
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

	allPods := []pcn_types.Pod{}
	allPods = append(allPods, ProductionPods...)
	allPods = append(allPods, BetaPods...)

	//---------------------------------
	//	Faking the Pod controller
	//---------------------------------

	testObj := new(MockPodController)
	testObj.On("GetPods", pcn_types.PodQuery{
		Pod: pcn_types.PodQueryObject{
			By:     "labels",
			Labels: AppPodSelector.MatchLabels,
		},
		Namespace: pcn_types.PodQueryObject{
			By:   "name",
			Name: "*",
		},
	}).Return(allPods, nil)
	testObj.On("GetPods", pcn_types.PodQuery{
		Pod: pcn_types.PodQueryObject{
			By:   "name",
			Name: "*",
		},
		Namespace: pcn_types.PodQueryObject{
			By:   "name",
			Name: "*",
		},
	}).Return(allPods, nil)
	testObj.On("GetPods", pcn_types.PodQuery{
		Pod: pcn_types.PodQueryObject{
			By:     "labels",
			Labels: podLabels,
		},
		Namespace: pcn_types.PodQueryObject{
			By:     "labels",
			Labels: namespaceLabels,
		},
	}).Return(ProductionPods, nil)

	expectedIngress := []k8sfirewall.ChainRule{
		k8sfirewall.ChainRule{
			Src:     ProductionPods[0].Pod.Status.PodIP,
			L4proto: "TCP",
			Sport:   6895,
			Action:  "forward",
		},
		k8sfirewall.ChainRule{
			Src:     ProductionPods[0].Pod.Status.PodIP,
			L4proto: "UDP",
			Sport:   6895,
			Action:  "forward",
		},
	}
	expectedEgress := []k8sfirewall.ChainRule{
		k8sfirewall.ChainRule{
			Dst:     ProductionPods[0].Pod.Status.PodIP,
			L4proto: "UDP",
			Dport:   5000,
			Action:  "forward",
		},
		k8sfirewall.ChainRule{
			Dst:     ProductionPods[0].Pod.Status.PodIP,
			L4proto: "TCP",
			Dport:   5000,
			Action:  "forward",
		},
		k8sfirewall.ChainRule{
			Dst:     ProductionPods[0].Pod.Status.PodIP,
			L4proto: "TCP",
			Dport:   80,
			Action:  "forward",
		},
		k8sfirewall.ChainRule{
			Dst:     ProductionPods[0].Pod.Status.PodIP,
			L4proto: "TCP",
			Dport:   8080,
			Action:  "forward",
		},
	}

	manager := Init(testObj)
	ingress, egress := manager.DoesPolicyTargetPod(policy, &ProductionPods[0].Pod)

	assert.NotEmpty(t, ingress)
	assert.Len(t, ingress, 2)
	assert.ElementsMatch(t, expectedIngress, ingress)
	assert.NotEmpty(t, egress)
	assert.ElementsMatch(t, expectedEgress, egress)*/
}
