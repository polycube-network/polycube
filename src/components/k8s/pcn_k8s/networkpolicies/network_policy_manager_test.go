package networkpolicies

import (
	"testing"

	"github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/controllers"
	podquery "github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/types/podquery"

	"github.com/stretchr/testify/mock"

	events "github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/types/events"
	polycube_pod "github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/types/polycubepod"
	k8sfirewall "github.com/SunSince90/polycube/src/components/k8s/utils/k8sfirewall"
	"github.com/stretchr/testify/assert"
	core_v1 "k8s.io/api/core/v1"
	networking_v1 "k8s.io/api/networking/v1"
	meta_v1 "k8s.io/apimachinery/pkg/apis/meta/v1"
	"k8s.io/apimachinery/pkg/util/intstr"
)

//	Mock the pod controller
type MockPodController struct {
	mock.Mock
}

func (m *MockPodController) Run()  {}
func (m *MockPodController) Stop() {}
func (m *MockPodController) Subscribe(event events.EventType, consumer func(*polycube_pod.Pod)) (func(), error) {
	return func() {}, nil
}
func (m *MockPodController) GetPodsByName(name string, ns string) ([]polycube_pod.Pod, error) {
	args := m.Called(name, ns)
	return args.Get(0).([]polycube_pod.Pod), args.Error(1)
}
func (m *MockPodController) AsyncGetPodsByName(channel chan<- []polycube_pod.Pod, name string, ns string) ([]polycube_pod.Pod, error) {
	args := m.Called(channel, name, ns)
	return args.Get(0).([]polycube_pod.Pod), args.Error(1)
}
func (m *MockPodController) GetPodsByLabels(labels map[string]string, ns string) ([]polycube_pod.Pod, error) {
	args := m.Called(labels, ns)
	return args.Get(0).([]polycube_pod.Pod), args.Error(1)
}
func (m *MockPodController) AsyncGetPodsByLabels(channel chan<- []polycube_pod.Pod, labels map[string]string, ns string) ([]polycube_pod.Pod, error) {
	args := m.Called(channel, labels, ns)
	return args.Get(0).([]polycube_pod.Pod), args.Error(1)
}
func (m *MockPodController) GetPods(query podquery.Query) ([]polycube_pod.Pod, error) {
	args := m.Called(query)
	return args.Get(0).([]polycube_pod.Pod), args.Error(1)
}
func (m *MockPodController) AsyncGetPods(channel chan<- []polycube_pod.Pod, query podquery.Query) ([]polycube_pod.Pod, error) {
	args := m.Called(query)
	return args.Get(0).([]polycube_pod.Pod), args.Error(1)
}

func Init(podController controllers.PodController) *NetworkPolicyManager {

	//podController := &MockPodController{}
	manager := NewNetworkPolicyManager(nil, podController)

	return manager
	//dnpc := controllers.NewDefaultNetworkPolicyController(dnpcMock)
}

func TestNotNil(t *testing.T) {
	manager := Init(nil)
	assert.NotNil(t, manager)
}

func CommonTest(t *testing.T, err error, generatedFirewall *k8sfirewall.Firewall) {
	assert.Nil(t, err)

	if err != nil {
		t.Errorf("error is not nil %s", err)
	}

	assert.Equal(t, false, generatedFirewall.Interactive)
	assert.Equal(t, 2, len(generatedFirewall.Chain))
}

func TestNoSpec(t *testing.T) {

	/*	When Spec is null, nothing should be allowed, both in ingress and egress */

	ingress := &networking_v1.NetworkPolicy{}
	manager := Init(nil)

	result, err := manager.ParseDefaultPolicy(ingress)

	CommonTest(t, err, result)

	assert.Len(t, result.Chain, 2)
	for i, chain := range result.Chain {
		if chain.Name != "ingress" && chain.Name != "egress" {
			t.Error(i, "element is not ingress nor egress:", chain.Name)
		}
		assert.Equal(t, "drop", chain.Default_)
		assert.Len(t, chain.Rule, 0)
	}
}

func TestIngressLenIsZero(t *testing.T) {

	/* When Ingress array is empty, nothing should be accessed in ingress */

	policy := &networking_v1.NetworkPolicy{
		Spec: networking_v1.NetworkPolicySpec{
			Ingress: []networking_v1.NetworkPolicyIngressRule{},
		},
	}
	manager := Init(nil)
	result, err := manager.ParseDefaultPolicy(policy)
	CommonTest(t, err, result)

	assert.Equal(t, []k8sfirewall.Chain{
		k8sfirewall.Chain{
			Default_: "drop",
			Name:     "ingress",
		},
		k8sfirewall.Chain{
			Default_: "drop",
			Name:     "egress",
		},
	}, result.Chain)
}

func TestFromIsEmpty(t *testing.T) {

	/* When From is empty, everything should be accessed in ingress */

	policy := &networking_v1.NetworkPolicy{
		Spec: networking_v1.NetworkPolicySpec{
			Ingress: []networking_v1.NetworkPolicyIngressRule{
				networking_v1.NetworkPolicyIngressRule{
					From: nil,
				},
			},
		},
	}
	manager := Init(nil)
	result, err := manager.ParseDefaultPolicy(policy)
	CommonTest(t, err, result)

	assert.Equal(t, []k8sfirewall.Chain{
		k8sfirewall.Chain{
			Default_: "forward",
			Name:     "ingress",
		},
		k8sfirewall.Chain{
			Default_: "drop",
			Name:     "egress",
		},
	}, result.Chain)
}

func TestSingleRuleIPBlockNoExceptions(t *testing.T) {

	policy := &networking_v1.NetworkPolicy{
		Spec: networking_v1.NetworkPolicySpec{
			Ingress: []networking_v1.NetworkPolicyIngressRule{
				networking_v1.NetworkPolicyIngressRule{
					From: []networking_v1.NetworkPolicyPeer{
						networking_v1.NetworkPolicyPeer{
							IPBlock: &networking_v1.IPBlock{
								CIDR: "192.168.1.20/32",
							},
						},
					},
				},
			},
		},
	}
	manager := Init(nil)
	result, err := manager.ParseDefaultPolicy(policy)
	CommonTest(t, err, result)

	for _, chain := range result.Chain {
		if chain.Name == "ingress" {
			for _, rule := range chain.Rule {
				assert.Equal(t, rule.Action, "forward")
				assert.Equal(t, rule.Src, "192.168.1.20/32")

				//	Assert equal does not like it when you give 0, because it thinks
				//	it is int and we want int32...
				var sport int32 = 0
				assert.Equal(t, rule.Sport, sport)
			}
		}
	}
}

func TestSingleRuleIPBlockMultipleExceptions(t *testing.T) {

	policy := &networking_v1.NetworkPolicy{
		Spec: networking_v1.NetworkPolicySpec{
			Ingress: []networking_v1.NetworkPolicyIngressRule{
				networking_v1.NetworkPolicyIngressRule{
					From: []networking_v1.NetworkPolicyPeer{
						networking_v1.NetworkPolicyPeer{
							IPBlock: &networking_v1.IPBlock{
								CIDR: "192.168.0.0/16",
								Except: []string{
									"192.168.2.0/24",
									"192.168.5.0/24",
									"192.168.100.20/32",
								},
							},
						},
					},
				},
			},
		},
	}
	manager := Init(nil)
	result, err := manager.ParseDefaultPolicy(policy)
	CommonTest(t, err, result)
	var zero int32 = 0

	for _, chain := range result.Chain {
		if chain.Name == "ingress" {
			assert.Len(t, chain.Rule, 4)

			for _, rule := range chain.Rule {

				switch rule.Src {
				case "192.168.0.0/16":
					assert.Equal(t, rule.Action, "forward")
					assert.Equal(t, rule.Sport, zero)

				case "192.168.2.0/24":
				case "192.168.5.0/24":
				case "192.168.100.20/32":
					assert.Equal(t, rule.Action, "drop")
				default:
					assert.Fail(t, "ip is not correct")
				}
			}
		}
	}
}

func TestSingleRuleIPBlockMultipleExceptionsWithProtocolNoPort(t *testing.T) {

	tcp := core_v1.ProtocolTCP
	/*zeroPort := &intstr.IntOrString{
		IntVal: 0,
	}*/
	policy := &networking_v1.NetworkPolicy{
		Spec: networking_v1.NetworkPolicySpec{
			Ingress: []networking_v1.NetworkPolicyIngressRule{
				networking_v1.NetworkPolicyIngressRule{
					From: []networking_v1.NetworkPolicyPeer{
						networking_v1.NetworkPolicyPeer{
							IPBlock: &networking_v1.IPBlock{
								CIDR: "192.168.0.0/16",
								Except: []string{
									"192.168.2.0/24",
									"192.168.5.0/24",
									"192.168.100.20/32",
								},
							},
						},
					},
					Ports: []networking_v1.NetworkPolicyPort{
						networking_v1.NetworkPolicyPort{
							Protocol: &tcp,
							//Port:     zeroPort,
						},
					},
				},
			},
		},
	}
	manager := Init(nil)
	result, err := manager.ParseDefaultPolicy(policy)
	CommonTest(t, err, result)

	for _, chain := range result.Chain {
		if chain.Name == "ingress" {
			assert.Len(t, chain.Rule, 4)

			for _, rule := range chain.Rule {

				switch rule.Src {
				case "192.168.0.0/16":
					assert.Equal(t, rule.Action, "forward")
					assert.Zero(t, rule.Sport, 0)
					assert.Equal(t, rule.L4proto, "TCP")

				case "192.168.2.0/24":
				case "192.168.5.0/24":
				case "192.168.100.20/32":
					assert.Equal(t, rule.Action, "drop")
					assert.Zero(t, rule.Sport, 0)
					assert.Equal(t, rule.L4proto, "TCP")
				default:
					assert.Fail(t, "ip is not correct")
				}
			}
		}
	}
}

func TestSingleRuleIPBlockMultipleExceptionsWithProtocolAndPort(t *testing.T) {

	tcp := core_v1.ProtocolTCP
	port := &intstr.IntOrString{
		IntVal: 6895,
	}
	policy := &networking_v1.NetworkPolicy{
		Spec: networking_v1.NetworkPolicySpec{
			Ingress: []networking_v1.NetworkPolicyIngressRule{
				networking_v1.NetworkPolicyIngressRule{
					From: []networking_v1.NetworkPolicyPeer{
						networking_v1.NetworkPolicyPeer{
							IPBlock: &networking_v1.IPBlock{
								CIDR: "192.168.0.0/16",
								Except: []string{
									"192.168.2.0/24",
									"192.168.5.0/24",
									"192.168.100.20/32",
								},
							},
						},
					},
					Ports: []networking_v1.NetworkPolicyPort{
						networking_v1.NetworkPolicyPort{
							Protocol: &tcp,
							Port:     port,
						},
					},
				},
			},
		},
	}
	manager := Init(nil)
	result, err := manager.ParseDefaultPolicy(policy)
	CommonTest(t, err, result)

	for _, chain := range result.Chain {
		if chain.Name == "ingress" {
			assert.Len(t, chain.Rule, 4)

			for _, rule := range chain.Rule {

				switch rule.Src {
				case "192.168.0.0/16":
					assert.Equal(t, rule.Action, "forward")
					assert.Equal(t, rule.Sport, port.IntVal)
					assert.Equal(t, rule.L4proto, "TCP")

				case "192.168.2.0/24":
				case "192.168.5.0/24":
				case "192.168.100.20/32":
					assert.Equal(t, rule.Action, "drop")
					assert.Equal(t, rule.Sport, port.IntVal)
					assert.Equal(t, rule.L4proto, "TCP")
				default:
					assert.Fail(t, "ip is not correct")
				}
			}
		}
	}
}

func TestSingleRuleIPBlockMultipleExceptionsWithMultipleProtocolAndPort(t *testing.T) {

	tcp := core_v1.ProtocolTCP
	udp := core_v1.ProtocolUDP
	port := &intstr.IntOrString{
		IntVal: 6895,
	}
	policy := &networking_v1.NetworkPolicy{
		Spec: networking_v1.NetworkPolicySpec{
			Ingress: []networking_v1.NetworkPolicyIngressRule{
				networking_v1.NetworkPolicyIngressRule{
					From: []networking_v1.NetworkPolicyPeer{
						networking_v1.NetworkPolicyPeer{
							IPBlock: &networking_v1.IPBlock{
								CIDR: "192.168.0.0/16",
								Except: []string{
									"192.168.2.0/24",
									"192.168.5.0/24",
									"192.168.100.20/32",
								},
							},
						},
					},
					Ports: []networking_v1.NetworkPolicyPort{
						networking_v1.NetworkPolicyPort{
							Protocol: &tcp,
							Port:     port,
						},
						networking_v1.NetworkPolicyPort{
							Protocol: &udp,
							Port:     port,
						},
					},
				},
			},
		},
	}
	manager := Init(nil)
	result, err := manager.ParseDefaultPolicy(policy)
	CommonTest(t, err, result)

	for _, chain := range result.Chain {
		if chain.Name == "ingress" {
			assert.Len(t, chain.Rule, 8)

			allValues := make(map[string]int)
			for _, rule := range chain.Rule {

				switch rule.Src {
				case "192.168.0.0/16":
					assert.Equal(t, rule.Action, "forward")
					assert.Equal(t, rule.Sport, port.IntVal)

				case "192.168.2.0/24":
				case "192.168.5.0/24":
				case "192.168.100.20/32":
					assert.Equal(t, rule.Action, "drop")
					assert.Equal(t, rule.Sport, port.IntVal)
					if rule.L4proto != "TCP" && rule.L4proto != "UDP" {
						assert.Fail(t, "protocol is invalid")
					}
					allValues[rule.Src]++
				default:
					assert.Fail(t, "ip is not correct")
				}
			}

			for _, occurrences := range allValues {
				assert.Equal(t, occurrences, 2)
			}
		}
	}
}

func TestUnsupportedProtocolWithSupported(t *testing.T) {

	tcp := core_v1.ProtocolSCTP
	udp := core_v1.ProtocolUDP
	port := &intstr.IntOrString{
		IntVal: 6895,
	}
	policy := &networking_v1.NetworkPolicy{
		Spec: networking_v1.NetworkPolicySpec{
			Ingress: []networking_v1.NetworkPolicyIngressRule{
				networking_v1.NetworkPolicyIngressRule{
					From: []networking_v1.NetworkPolicyPeer{
						networking_v1.NetworkPolicyPeer{
							IPBlock: &networking_v1.IPBlock{
								CIDR: "192.168.0.0/16",
								Except: []string{
									"192.168.2.0/24",
									"192.168.5.0/24",
									"192.168.100.20/32",
								},
							},
						},
					},
					Ports: []networking_v1.NetworkPolicyPort{
						networking_v1.NetworkPolicyPort{
							Protocol: &tcp,
							Port:     port,
						},
						networking_v1.NetworkPolicyPort{
							Protocol: &udp,
							Port:     port,
						},
					},
				},
			},
		},
	}
	manager := Init(nil)
	result, err := manager.ParseDefaultPolicy(policy)
	CommonTest(t, err, result)

	for _, chain := range result.Chain {
		if chain.Name == "ingress" {
			assert.Len(t, chain.Rule, 4)

			for _, rule := range chain.Rule {

				switch rule.Src {
				case "192.168.0.0/16":
					assert.Equal(t, rule.Action, "forward")
					assert.Equal(t, rule.Sport, port.IntVal)
					assert.Equal(t, rule.L4proto, "UDP")

				case "192.168.2.0/24":
				case "192.168.5.0/24":
				case "192.168.100.20/32":
					assert.Equal(t, rule.Action, "drop")
					assert.Equal(t, rule.Sport, port.IntVal)
					assert.Equal(t, rule.L4proto, "UDP")
				default:
					assert.Fail(t, "ip is not correct")
				}
			}
		}
	}
}

func TestOnlyUnsupportedProtocol(t *testing.T) {

	//	if the policy only consists of unsupported protocol, then it is better
	//	not to generate rules at all, instead of creating wrong rules!
	tcp := core_v1.ProtocolSCTP
	port := &intstr.IntOrString{
		IntVal: 6895,
	}
	policy := &networking_v1.NetworkPolicy{
		Spec: networking_v1.NetworkPolicySpec{
			Ingress: []networking_v1.NetworkPolicyIngressRule{
				networking_v1.NetworkPolicyIngressRule{
					From: []networking_v1.NetworkPolicyPeer{
						networking_v1.NetworkPolicyPeer{
							IPBlock: &networking_v1.IPBlock{
								CIDR: "192.168.0.0/16",
								Except: []string{
									"192.168.2.0/24",
									"192.168.5.0/24",
									"192.168.100.20/32",
								},
							},
						},
					},
					Ports: []networking_v1.NetworkPolicyPort{
						networking_v1.NetworkPolicyPort{
							Protocol: &tcp,
							Port:     port,
						},
					},
				},
			},
		},
	}
	manager := Init(nil)
	result, err := manager.ParseDefaultPolicy(policy)
	CommonTest(t, err, result)

	for _, chain := range result.Chain {
		if chain.Name == "ingress" {
			assert.Len(t, chain.Rule, 0)
		}
	}
}

func TestProtocolIsNil(t *testing.T) {

	//	if the policy only consists of unsupported protocol, then it is better
	//	not to generate rules at all, instead of creating wrong rules!
	port := &intstr.IntOrString{
		IntVal: 6895,
	}
	policy := &networking_v1.NetworkPolicy{
		Spec: networking_v1.NetworkPolicySpec{
			Ingress: []networking_v1.NetworkPolicyIngressRule{
				networking_v1.NetworkPolicyIngressRule{
					From: []networking_v1.NetworkPolicyPeer{
						networking_v1.NetworkPolicyPeer{
							IPBlock: &networking_v1.IPBlock{
								CIDR: "192.168.0.0/16",
								Except: []string{
									"192.168.2.0/24",
									"192.168.5.0/24",
									"192.168.100.20/32",
								},
							},
						},
					},
					Ports: []networking_v1.NetworkPolicyPort{
						networking_v1.NetworkPolicyPort{
							Protocol: nil,
							Port:     port,
						},
					},
				},
			},
		},
	}
	manager := Init(nil)
	result, err := manager.ParseDefaultPolicy(policy)
	CommonTest(t, err, result)

	for _, chain := range result.Chain {
		if chain.Name == "ingress" {
			assert.Len(t, chain.Rule, 8)

			allValues := make(map[string]int)
			for _, rule := range chain.Rule {

				switch rule.Src {
				case "192.168.0.0/16":
					assert.Equal(t, rule.Action, "forward")
					assert.Equal(t, rule.Sport, port.IntVal)

				case "192.168.2.0/24":
				case "192.168.5.0/24":
				case "192.168.100.20/32":
					assert.Equal(t, rule.Action, "drop")
					assert.Equal(t, rule.Sport, port.IntVal)
					if rule.L4proto != "TCP" && rule.L4proto != "UDP" {
						assert.Fail(t, "protocol is invalid")
					}
					allValues[rule.Src]++
				default:
					assert.Fail(t, "ip is not correct")
				}
			}

			for _, occurrences := range allValues {
				assert.Equal(t, occurrences, 2)
			}
		}
	}
}

func TestPodSelectorWithProtocol(t *testing.T) {
	tcp := core_v1.ProtocolTCP
	port := &intstr.IntOrString{
		IntVal: 6895,
	}
	labels := map[string]string{
		"v":     "1.2",
		"cache": "redis",
	}
	policy := &networking_v1.NetworkPolicy{
		Spec: networking_v1.NetworkPolicySpec{
			Ingress: []networking_v1.NetworkPolicyIngressRule{
				networking_v1.NetworkPolicyIngressRule{
					From: []networking_v1.NetworkPolicyPeer{
						networking_v1.NetworkPolicyPeer{
							PodSelector: &meta_v1.LabelSelector{
								MatchLabels: labels,
							},
						},
					},
					Ports: []networking_v1.NetworkPolicyPort{
						networking_v1.NetworkPolicyPort{
							Protocol: &tcp,
							Port:     port,
						},
					},
				},
			},
		},
	}

	podsFound := []polycube_pod.Pod{
		polycube_pod.Pod{
			Pod: core_v1.Pod{
				Status: core_v1.PodStatus{
					PodIP: "172.10.30.30",
				},
			},
			Veth: "veth123",
		},
	}

	testObj := new(MockPodController)
	testObj.On("GetPods", mock.Anything).Return(podsFound, nil)

	manager := Init(testObj)

	result, err := manager.ParseDefaultPolicy(policy)
	CommonTest(t, err, result)

	for _, chain := range result.Chain {
		if chain.Name == "ingress" {
			assert.Len(t, chain.Rule, 1)

			for _, rule := range chain.Rule {
				assert.Equal(t, rule.Action, "forward")
				assert.Equal(t, rule.Src, "172.10.30.30")
				assert.Equal(t, rule.L4proto, "TCP")
				assert.Equal(t, rule.Sport, port.IntVal)
			}
		}
	}
}

func TestPodSelectorNoPodsFound(t *testing.T) {
	tcp := core_v1.ProtocolTCP
	port := &intstr.IntOrString{
		IntVal: 6895,
	}
	labels := map[string]string{
		"v":     "1.2",
		"cache": "redis",
	}
	policy := &networking_v1.NetworkPolicy{
		Spec: networking_v1.NetworkPolicySpec{
			Ingress: []networking_v1.NetworkPolicyIngressRule{
				networking_v1.NetworkPolicyIngressRule{
					From: []networking_v1.NetworkPolicyPeer{
						networking_v1.NetworkPolicyPeer{
							PodSelector: &meta_v1.LabelSelector{
								MatchLabels: labels,
							},
						},
					},
					Ports: []networking_v1.NetworkPolicyPort{
						networking_v1.NetworkPolicyPort{
							Protocol: &tcp,
							Port:     port,
						},
					},
				},
			},
		},
	}

	podsFound := []polycube_pod.Pod{}

	testObj := new(MockPodController)
	testObj.On("GetPods", mock.Anything).Return(podsFound, nil)

	manager := Init(testObj)

	result, err := manager.ParseDefaultPolicy(policy)
	CommonTest(t, err, result)

	for _, chain := range result.Chain {
		if chain.Name == "ingress" {
			assert.Len(t, chain.Rule, 0)
		}
	}
}

func TestPodSelectorMultiplePodsFound(t *testing.T) {
	tcp := core_v1.ProtocolTCP
	port := &intstr.IntOrString{
		IntVal: 6895,
	}
	labels := map[string]string{
		"v":     "1.2",
		"cache": "redis",
	}
	policy := &networking_v1.NetworkPolicy{
		Spec: networking_v1.NetworkPolicySpec{
			Ingress: []networking_v1.NetworkPolicyIngressRule{
				networking_v1.NetworkPolicyIngressRule{
					From: []networking_v1.NetworkPolicyPeer{
						networking_v1.NetworkPolicyPeer{
							PodSelector: &meta_v1.LabelSelector{
								MatchLabels: labels,
							},
						},
					},
					Ports: []networking_v1.NetworkPolicyPort{
						networking_v1.NetworkPolicyPort{
							Protocol: &tcp,
							Port:     port,
						},
					},
				},
			},
		},
	}

	podsFound := []polycube_pod.Pod{
		polycube_pod.Pod{
			Pod: core_v1.Pod{
				Status: core_v1.PodStatus{
					PodIP: "172.10.30.30",
				},
			},
			Veth: "veth123",
		},
		polycube_pod.Pod{
			Pod: core_v1.Pod{
				Status: core_v1.PodStatus{
					PodIP: "172.25.153.77",
				},
			},
			Veth: "veth312",
		},
	}

	testObj := new(MockPodController)
	testObj.On("GetPods", mock.Anything).Return(podsFound, nil)

	manager := Init(testObj)

	result, err := manager.ParseDefaultPolicy(policy)
	CommonTest(t, err, result)

	for _, chain := range result.Chain {
		if chain.Name == "ingress" {
			assert.Len(t, chain.Rule, 2)

			var found int
			for _, rule := range chain.Rule {

				switch rule.Src {
				case "172.25.153.77", "172.10.30.30":
					found++
					assert.Equal(t, rule.Sport, port.IntVal)
				default:
					assert.Fail(t, "unrecognized ip")
				}
			}

			assert.Equal(t, 2, found)
		}
	}
}

func TestPodMatchExpressionReturnsNoRules(t *testing.T) {
	tcp := core_v1.ProtocolTCP
	port := &intstr.IntOrString{
		IntVal: 6895,
	}
	expression := []meta_v1.LabelSelectorRequirement{
		meta_v1.LabelSelectorRequirement{
			Key:      "cache",
			Operator: "In",
			Values:   []string{"redis", "memcached"},
		},
	}

	policy := &networking_v1.NetworkPolicy{
		Spec: networking_v1.NetworkPolicySpec{
			Ingress: []networking_v1.NetworkPolicyIngressRule{
				networking_v1.NetworkPolicyIngressRule{
					From: []networking_v1.NetworkPolicyPeer{
						networking_v1.NetworkPolicyPeer{
							PodSelector: &meta_v1.LabelSelector{
								MatchExpressions: expression,
							},
						},
					},
					Ports: []networking_v1.NetworkPolicyPort{
						networking_v1.NetworkPolicyPort{
							Protocol: &tcp,
							Port:     port,
						},
					},
				},
			},
		},
	}

	podsFound := []polycube_pod.Pod{
		polycube_pod.Pod{
			Pod: core_v1.Pod{
				Status: core_v1.PodStatus{
					PodIP: "172.10.30.30",
				},
			},
			Veth: "veth123",
		},
		polycube_pod.Pod{
			Pod: core_v1.Pod{
				Status: core_v1.PodStatus{
					PodIP: "172.25.153.77",
				},
			},
			Veth: "veth312",
		},
	}

	testObj := new(MockPodController)
	testObj.On("GetPods", mock.Anything).Return(podsFound, nil)

	manager := Init(testObj)

	result, err := manager.ParseDefaultPolicy(policy)
	CommonTest(t, err, result)

	for _, chain := range result.Chain {
		if chain.Name == "ingress" {
			assert.Len(t, chain.Rule, 0)
		}
	}
}

func TestEmptyPodSelectorSelectsAllPodsInSameNamespace(t *testing.T) {
	tcp := core_v1.ProtocolTCP
	port := &intstr.IntOrString{
		IntVal: 6895,
	}
	labels := map[string]string{}
	policy := &networking_v1.NetworkPolicy{
		ObjectMeta: meta_v1.ObjectMeta{
			Namespace: "Same",
		},
		Spec: networking_v1.NetworkPolicySpec{
			Ingress: []networking_v1.NetworkPolicyIngressRule{
				networking_v1.NetworkPolicyIngressRule{
					From: []networking_v1.NetworkPolicyPeer{
						networking_v1.NetworkPolicyPeer{
							PodSelector: &meta_v1.LabelSelector{
								MatchLabels: labels,
							},
						},
					},
					Ports: []networking_v1.NetworkPolicyPort{
						networking_v1.NetworkPolicyPort{
							Protocol: &tcp,
							Port:     port,
						},
					},
				},
			},
		},
	}

	podsFound := []polycube_pod.Pod{
		polycube_pod.Pod{
			Pod: core_v1.Pod{
				ObjectMeta: meta_v1.ObjectMeta{
					Namespace: "Same",
				},
				Status: core_v1.PodStatus{
					PodIP: "172.10.30.30",
				},
			},
			Veth: "veth123",
		},
		polycube_pod.Pod{
			Pod: core_v1.Pod{
				ObjectMeta: meta_v1.ObjectMeta{
					Namespace: "Same",
				},
				Status: core_v1.PodStatus{
					PodIP: "172.25.153.77",
				},
			},
			Veth: "veth312",
		},
	}

	testObj := new(MockPodController)
	testObj.On("GetPods", podquery.Query{
		Pod: []podquery.QueryObject{
			podquery.QueryObject{
				By:   "name",
				Name: "*",
			},
		},
		Namespace: []podquery.QueryObject{
			podquery.QueryObject{
				By:   "name",
				Name: "Same",
			},
		},
	}).Return(podsFound, nil)

	manager := Init(testObj)

	result, err := manager.ParseDefaultPolicy(policy)
	CommonTest(t, err, result)

	for _, chain := range result.Chain {
		if chain.Name == "ingress" {
			assert.Len(t, chain.Rule, 2)

			var found int
			for _, rule := range chain.Rule {

				switch rule.Src {
				case "172.25.153.77", "172.10.30.30":
					found++
					assert.Equal(t, rule.Sport, port.IntVal)
					assert.Equal(t, rule.Action, "forward")
				default:
					assert.Fail(t, "unrecognized ip")
				}
			}

			assert.Equal(t, 2, found)
		}
	}
}

func TestNilLabelSelectorBlocksAllPodsInSameNamespace(t *testing.T) {
	tcp := core_v1.ProtocolTCP
	port := &intstr.IntOrString{
		IntVal: 6895,
	}

	policy := &networking_v1.NetworkPolicy{
		ObjectMeta: meta_v1.ObjectMeta{
			Namespace: "Same",
		},
		Spec: networking_v1.NetworkPolicySpec{
			Ingress: []networking_v1.NetworkPolicyIngressRule{
				networking_v1.NetworkPolicyIngressRule{
					From: []networking_v1.NetworkPolicyPeer{
						networking_v1.NetworkPolicyPeer{
							PodSelector: &meta_v1.LabelSelector{
								MatchLabels: nil,
							},
						},
					},
					Ports: []networking_v1.NetworkPolicyPort{
						networking_v1.NetworkPolicyPort{
							Protocol: &tcp,
							Port:     port,
						},
					},
				},
			},
		},
	}

	podsFound := []polycube_pod.Pod{
		polycube_pod.Pod{
			Pod: core_v1.Pod{
				ObjectMeta: meta_v1.ObjectMeta{
					Namespace: "Same",
				},
				Status: core_v1.PodStatus{
					PodIP: "172.10.30.30",
				},
			},
			Veth: "veth123",
		},
		polycube_pod.Pod{
			Pod: core_v1.Pod{
				ObjectMeta: meta_v1.ObjectMeta{
					Namespace: "Same",
				},
				Status: core_v1.PodStatus{
					PodIP: "172.25.153.77",
				},
			},
			Veth: "veth312",
		},
	}

	testObj := new(MockPodController)
	testObj.On("GetPods", podquery.Query{
		Pod: []podquery.QueryObject{
			podquery.QueryObject{
				By:   "name",
				Name: "*",
			},
		},
		Namespace: []podquery.QueryObject{
			podquery.QueryObject{
				By:   "name",
				Name: "Same",
			},
		},
	}).Return(podsFound, nil)

	manager := Init(testObj)

	result, err := manager.ParseDefaultPolicy(policy)
	CommonTest(t, err, result)

	for _, chain := range result.Chain {
		if chain.Name == "ingress" {
			assert.Len(t, chain.Rule, 2)

			var found int
			for _, rule := range chain.Rule {

				switch rule.Src {
				case "172.25.153.77", "172.10.30.30":
					found++
					assert.Equal(t, rule.Sport, port.IntVal)
					assert.Equal(t, rule.Action, "drop")
				default:
					assert.Fail(t, "unrecognized ip")
				}
			}

			assert.Equal(t, 2, found)
		}
	}
}

func TestSelectAllFromSpecificNamespace(t *testing.T) {
	tcp := core_v1.ProtocolTCP
	port := &intstr.IntOrString{
		IntVal: 6895,
	}
	labels := map[string]string{
		"environment": "production",
	}
	policy := &networking_v1.NetworkPolicy{
		ObjectMeta: meta_v1.ObjectMeta{
			Namespace: "Same",
		},
		Spec: networking_v1.NetworkPolicySpec{
			Ingress: []networking_v1.NetworkPolicyIngressRule{
				networking_v1.NetworkPolicyIngressRule{
					From: []networking_v1.NetworkPolicyPeer{
						networking_v1.NetworkPolicyPeer{
							NamespaceSelector: &meta_v1.LabelSelector{
								MatchLabels: labels,
							},
						},
					},
					Ports: []networking_v1.NetworkPolicyPort{
						networking_v1.NetworkPolicyPort{
							Protocol: &tcp,
							Port:     port,
						},
					},
				},
			},
		},
	}

	podsFound := []polycube_pod.Pod{
		polycube_pod.Pod{
			Pod: core_v1.Pod{
				ObjectMeta: meta_v1.ObjectMeta{
					Namespace: "Same",
				},
				Status: core_v1.PodStatus{
					PodIP: "172.10.30.30",
				},
			},
			Veth: "veth123",
		},
		polycube_pod.Pod{
			Pod: core_v1.Pod{
				ObjectMeta: meta_v1.ObjectMeta{
					Namespace: "Same",
				},
				Status: core_v1.PodStatus{
					PodIP: "172.25.153.77",
				},
			},
			Veth: "veth312",
		},
	}

	testObj := new(MockPodController)
	testObj.On("GetPods", podquery.Query{
		Pod: []podquery.QueryObject{
			podquery.QueryObject{
				By:   "name",
				Name: "*",
			},
		},
		Namespace: []podquery.QueryObject{
			podquery.QueryObject{
				By:     "labels",
				Labels: labels,
			},
		},
	}).Return(podsFound, nil)

	manager := Init(testObj)

	result, err := manager.ParseDefaultPolicy(policy)
	CommonTest(t, err, result)

	for _, chain := range result.Chain {
		if chain.Name == "ingress" {
			assert.Len(t, chain.Rule, 2)

			var found int
			for _, rule := range chain.Rule {

				switch rule.Src {
				case "172.10.30.30", "172.25.153.77":
					found++
					assert.Equal(t, rule.Sport, port.IntVal)
					assert.Equal(t, rule.Action, "forward")
				default:
					assert.Fail(t, "unrecognized ip")
				}
			}

			assert.Equal(t, 2, found)
		}
	}
}

func TestBlockAllFromSpecificNamespace(t *testing.T) {
	tcp := core_v1.ProtocolTCP
	port := &intstr.IntOrString{
		IntVal: 6895,
	}
	labels := map[string]string{
		"environment": "beta",
	}
	policy := &networking_v1.NetworkPolicy{
		ObjectMeta: meta_v1.ObjectMeta{
			Namespace: "Same",
		},
		Spec: networking_v1.NetworkPolicySpec{
			Ingress: []networking_v1.NetworkPolicyIngressRule{
				networking_v1.NetworkPolicyIngressRule{
					From: []networking_v1.NetworkPolicyPeer{
						networking_v1.NetworkPolicyPeer{
							PodSelector: &meta_v1.LabelSelector{
								MatchLabels: nil,
							},
							NamespaceSelector: &meta_v1.LabelSelector{
								MatchLabels: labels,
							},
						},
					},
					Ports: []networking_v1.NetworkPolicyPort{
						networking_v1.NetworkPolicyPort{
							Protocol: &tcp,
							Port:     port,
						},
					},
				},
			},
		},
	}

	podsFound := []polycube_pod.Pod{
		polycube_pod.Pod{
			Pod: core_v1.Pod{
				ObjectMeta: meta_v1.ObjectMeta{
					Namespace: "Same",
				},
				Status: core_v1.PodStatus{
					PodIP: "172.10.30.30",
				},
			},
			Veth: "veth123",
		},
		polycube_pod.Pod{
			Pod: core_v1.Pod{
				ObjectMeta: meta_v1.ObjectMeta{
					Namespace: "Same",
				},
				Status: core_v1.PodStatus{
					PodIP: "172.25.153.77",
				},
			},
			Veth: "veth312",
		},
	}

	testObj := new(MockPodController)
	testObj.On("GetPods", podquery.Query{
		Pod: []podquery.QueryObject{
			podquery.QueryObject{
				By:   "name",
				Name: "*",
			},
		},
		Namespace: []podquery.QueryObject{
			podquery.QueryObject{
				By:     "labels",
				Labels: labels,
			},
		},
	}).Return(podsFound, nil)

	manager := Init(testObj)

	result, err := manager.ParseDefaultPolicy(policy)
	CommonTest(t, err, result)

	for _, chain := range result.Chain {
		if chain.Name == "ingress" {
			assert.Len(t, chain.Rule, 2)

			var found int
			for _, rule := range chain.Rule {

				switch rule.Src {
				case "172.10.30.30", "172.25.153.77":
					found++
					assert.Equal(t, rule.Sport, port.IntVal)
					assert.Equal(t, rule.Action, "drop")
				default:
					assert.Fail(t, "unrecognized ip")
				}
			}

			assert.Equal(t, 2, found)
		}
	}
}

func TestComplexPolicy(t *testing.T) {
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
			Namespace: "Production",
		},
		Spec: networking_v1.NetworkPolicySpec{
			Ingress: []networking_v1.NetworkPolicyIngressRule{

				/* First rule
				Block the following if it comes with TCP6895 Or UDP6895
					- what comes from 192.168.2.0/24
					- what comes from 192.168.5.0/24
					- what comes from 192.168.100.20/32
					- All pods that belong to all namespaces

				Allow the following if it comes with TCP6895 & UDP6895
					- what comes from 192.168.0.0/16 (see exceptions above)
				*/
				networking_v1.NetworkPolicyIngressRule{
					From: []networking_v1.NetworkPolicyPeer{
						networking_v1.NetworkPolicyPeer{
							IPBlock: &networking_v1.IPBlock{
								CIDR: "192.168.0.0/16",
								Except: []string{
									"192.168.2.0/24",
									"192.168.5.0/24",
									"192.168.100.20/32",
								},
							},
							PodSelector: &meta_v1.LabelSelector{
								MatchLabels: nil,
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

				/*	Second Rule
					Block the following if it comes with TCP8080 Or TCP80:
						- what comes from 192.169.2.0/24
						- what comes from 192.169.5.0/24
						- what comes from 192.169.100.20/32

					Allow the following if it comes with TCP8080 Or TCP80:
						- what comes from 192.169.0.0/16 (see exceptions above)
						- all pods with labels podLabels that belong to certain namespace (namespaceLabels)
				*/
				networking_v1.NetworkPolicyIngressRule{
					From: []networking_v1.NetworkPolicyPeer{
						networking_v1.NetworkPolicyPeer{
							IPBlock: &networking_v1.IPBlock{
								CIDR: "192.169.0.0/16",
								Except: []string{
									"192.169.2.0/24",
									"192.169.5.0/24",
									"192.169.100.20/32",
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

				/*	Third rule
					Allow everything that comes from 5000
				*/
				networking_v1.NetworkPolicyIngressRule{
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

	//	pods find for the first rule
	firstPodsFound := []polycube_pod.Pod{
		polycube_pod.Pod{
			Pod: core_v1.Pod{
				ObjectMeta: meta_v1.ObjectMeta{
					Namespace: "Production",
				},
				Status: core_v1.PodStatus{
					PodIP: "172.20.20.20",
				},
			},
			Veth: "veth123",
		},
		polycube_pod.Pod{
			Pod: core_v1.Pod{
				ObjectMeta: meta_v1.ObjectMeta{
					Namespace: "Production",
				},
				Status: core_v1.PodStatus{
					PodIP: "172.30.30.30",
				},
			},
			Veth: "veth312",
		},
		polycube_pod.Pod{
			Pod: core_v1.Pod{
				ObjectMeta: meta_v1.ObjectMeta{
					Namespace: "Beta",
				},
				Status: core_v1.PodStatus{
					PodIP: "172.40.40.40",
				},
			},
			Veth: "veth512",
		},
	}

	//	Pods found for the second rule
	secondPodsFound := []polycube_pod.Pod{
		polycube_pod.Pod{
			Pod: core_v1.Pod{
				ObjectMeta: meta_v1.ObjectMeta{
					Namespace: "Production",
				},
				Status: core_v1.PodStatus{
					PodIP: "172.20.20.20",
				},
			},
			Veth: "veth123",
		},
		polycube_pod.Pod{
			Pod: core_v1.Pod{
				ObjectMeta: meta_v1.ObjectMeta{
					Namespace: "Production",
				},
				Status: core_v1.PodStatus{
					PodIP: "172.30.30.30",
				},
			},
			Veth: "veth312",
		},
	}

	testObj := new(MockPodController)
	testObj.On("GetPods", podquery.Query{
		Pod: []podquery.QueryObject{
			podquery.QueryObject{
				By:   "name",
				Name: "*",
			},
		},
		Namespace: []podquery.QueryObject{
			podquery.QueryObject{
				By:   "name",
				Name: "*",
			},
		},
	}).Return(firstPodsFound, nil)
	testObj.On("GetPods", podquery.Query{
		Pod: []podquery.QueryObject{
			podquery.QueryObject{
				By:     "labels",
				Labels: podLabels,
			},
		},
		Namespace: []podquery.QueryObject{
			podquery.QueryObject{
				By:     "labels",
				Labels: namespaceLabels,
			},
		},
	}).Return(secondPodsFound, nil)

	manager := Init(testObj)

	result, err := manager.ParseDefaultPolicy(policy)
	CommonTest(t, err, result)

	for _, chain := range result.Chain {
		if chain.Name == "ingress" {
			assert.Len(t, chain.Rule, 14+12+2)

			for _, rule := range chain.Rule {

				switch rule.Src {
				case "172.20.20.20", "172.30.30.30", "172.40.40.40", "192.168.2.0/24", "192.168.5.0/24", "192.168.100.20/32":
					switch rule.Sport {
					case port1.IntVal:
						assert.Equal(t, rule.Action, "drop")
						if rule.L4proto != "TCP" && rule.L4proto != "UDP" {
							assert.Fail(t, "first rules: protocol unrecognized")
						}
					case port2.IntVal, port3.IntVal:
						assert.Equal(t, rule.Action, "forward")
						assert.Equal(t, rule.L4proto, "TCP")
					}
				case "192.168.0.0/16":
					switch rule.Sport {
					case port1.IntVal:
						if rule.L4proto != "TCP" && rule.L4proto != "UDP" {
							assert.Fail(t, "first rules: protocol unrecognized")
						}
					case port2.IntVal, port3.IntVal:
						assert.Equal(t, rule.L4proto, "TCP")
					}
					assert.Equal(t, rule.Action, "forward")
				case "192.169.2.0/24", "192.169.5.0/24", "192.169.100.20/32":
					assert.Equal(t, rule.L4proto, "TCP")
					if rule.Sport != port2.IntVal && rule.Sport != port3.IntVal {
						assert.Fail(t, "second rule, port unrecognized", rule.Sport)
						assert.Equal(t, rule.Action, "drop")
					}
				case "192.169.0.0/16":
					assert.Equal(t, rule.L4proto, "TCP")
					if rule.Sport != port2.IntVal && rule.Sport != port3.IntVal {
						assert.Fail(t, "second rule, port unrecognized", rule.Sport)
						assert.Equal(t, rule.Action, "forward")
					}
				case "0":
					if rule.L4proto != "TCP" && rule.L4proto != "UDP" {
						assert.Fail(t, "case 0: protocol unrecognized")
					}
					assert.Equal(t, rule.Sport, port4.IntVal)
					assert.Equal(t, rule.Action, "forward")
				default:
					assert.Fail(t, "first rule: unrecognized", rule.Src)
				}
			}
		}
	}
}
