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

var GlobalPodSelector1 = meta_v1.LabelSelector{
	MatchLabels: map[string]string{
		"type":    "app",
		"version": "2.5",
	},
}
var GlobalNamespace1 = "Production"
var GlobalName1 = "TestDefaultPolicy"
var GlobalQuery1 = podquery.Query{
	Pod: []podquery.QueryObject{
		podquery.QueryObject{
			By:     "labels",
			Labels: GlobalPodSelector1.MatchLabels,
		},
	},
	Namespace: []podquery.QueryObject{
		podquery.QueryObject{
			By:   "name",
			Name: GlobalNamespace1,
		},
	},
}
var GlobalPodsFound1 = []polycube_pod.Pod{
	polycube_pod.Pod{
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
	polycube_pod.Pod{
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

func CommonTest(t *testing.T, err error, generatedFirewall []k8sfirewall.Firewall) {
	assert.Nil(t, err)

	if err != nil {
		t.Errorf("error is not nil %s", err)
	}

	//assert.Equal(t, false, generatedFirewall.Interactive)
	//assert.Equal(t, 2, len(generatedFirewall.Chain))
}

func TestNoSpec(t *testing.T) {

	/*	When Spec is null, nothing should be allowed, both in ingress and egress */

	ingress := &networking_v1.NetworkPolicy{
		ObjectMeta: meta_v1.ObjectMeta{
			Name:      GlobalName1,
			Namespace: GlobalNamespace1,
		},
		Spec: networking_v1.NetworkPolicySpec{
			PodSelector: GlobalPodSelector1,
		},
	}
	testObj := new(MockPodController)
	testObj.On("GetPods", GlobalQuery1).Return(GlobalPodsFound1, nil)
	manager := Init(testObj)
	result, err := manager.ParseDefaultPolicy(ingress)

	CommonTest(t, err, result)

	assert.Len(t, result, 2)

	for _, fw := range result {
		for i, chain := range fw.Chain {
			if chain.Name != "ingress" && chain.Name != "egress" {
				t.Error(i, "element is not ingress nor egress:", chain.Name)
			}
			assert.Equal(t, "drop", chain.Default_)
			assert.Len(t, chain.Rule, 0)
		}
	}
}

func TestIngressLenIsZero(t *testing.T) {

	/* When Ingress array is empty, nothing is allowed in ingress */

	policy := &networking_v1.NetworkPolicy{
		ObjectMeta: meta_v1.ObjectMeta{
			Name:      GlobalName1,
			Namespace: GlobalNamespace1,
		},
		Spec: networking_v1.NetworkPolicySpec{
			PodSelector: GlobalPodSelector1,
			Ingress:     []networking_v1.NetworkPolicyIngressRule{},
		},
	}
	testObj := new(MockPodController)
	testObj.On("GetPods", GlobalQuery1).Return(GlobalPodsFound1, nil)
	manager := Init(testObj)
	result, err := manager.ParseDefaultPolicy(policy)
	CommonTest(t, err, result)

	assert.Len(t, result, 2)
	for _, fw := range result {
		assert.Len(t, fw.Chain, 1)

		assert.Equal(t, "ingress", fw.Chain[0].Name)
		assert.Equal(t, "drop", fw.Chain[0].Default_)
		assert.Len(t, fw.Chain[0].Rule, 0)
	}
}

func TestIngressLenIsZeroWithPort(t *testing.T) {
	tcp := core_v1.ProtocolTCP
	udp := core_v1.ProtocolUDP

	port1 := &intstr.IntOrString{
		IntVal: 6895,
	}

	policy := &networking_v1.NetworkPolicy{
		ObjectMeta: meta_v1.ObjectMeta{
			Name:      GlobalName1,
			Namespace: GlobalNamespace1,
		},
		Spec: networking_v1.NetworkPolicySpec{
			PodSelector: GlobalPodSelector1,
			Ingress: []networking_v1.NetworkPolicyIngressRule{
				networking_v1.NetworkPolicyIngressRule{
					From: []networking_v1.NetworkPolicyPeer{},
					Ports: []networking_v1.NetworkPolicyPort{
						networking_v1.NetworkPolicyPort{
							Port:     port1,
							Protocol: &tcp,
						},
						networking_v1.NetworkPolicyPort{
							Port:     port1,
							Protocol: &udp,
						},
					},
				},
			},
		},
	}

	testObj := new(MockPodController)
	testObj.On("GetPods", GlobalQuery1).Return(GlobalPodsFound1, nil)
	manager := Init(testObj)
	result, err := manager.ParseDefaultPolicy(policy)
	CommonTest(t, err, result)

	assert.Len(t, result, 2)

	for _, fw := range result {
		assert.Len(t, fw.Chain, 1)
		assert.NotEqual(t, "rgress", fw.Chain[0].Name)
		assert.Len(t, fw.Chain[0].Rule, 0)
		assert.Equal(t, "drop", fw.Chain[0].Default_)

		//	Found out that this test never happens so...
		/*for _, rule := range fw.Chain[0].Rule {
			assert.Equal(t, "drop", rule.Action)
			assert.Equal(t, port1.IntVal, rule.Sport)
			if rule.L4proto != "UDP" && rule.L4proto != "TCP" {
				assert.Fail(t, "incorrect protocol", rule.L4proto)
			}
		}*/
	}
}

func TestFromIsEmpty(t *testing.T) {

	/* When From is empty, everything is allowed in ingress */

	policy := &networking_v1.NetworkPolicy{
		ObjectMeta: meta_v1.ObjectMeta{
			Name:      GlobalName1,
			Namespace: GlobalNamespace1,
		},
		Spec: networking_v1.NetworkPolicySpec{
			PodSelector: GlobalPodSelector1,
			Ingress: []networking_v1.NetworkPolicyIngressRule{
				networking_v1.NetworkPolicyIngressRule{
					From: nil,
				},
			},
		},
	}
	testObj := new(MockPodController)
	testObj.On("GetPods", GlobalQuery1).Return(GlobalPodsFound1, nil)
	manager := Init(testObj)
	result, err := manager.ParseDefaultPolicy(policy)
	CommonTest(t, err, result)

	assert.Len(t, result, 2)
	for _, fw := range result {
		assert.Len(t, fw.Chain, 1)

		assert.Equal(t, "ingress", fw.Chain[0].Name)
		assert.Equal(t, "forward", fw.Chain[0].Default_)
		assert.Len(t, fw.Chain[0].Rule, 0)
	}
}

func TestSingleRuleIPBlockNoExceptions(t *testing.T) {

	policy := &networking_v1.NetworkPolicy{
		ObjectMeta: meta_v1.ObjectMeta{
			Name:      GlobalName1,
			Namespace: GlobalNamespace1,
		},
		Spec: networking_v1.NetworkPolicySpec{
			PodSelector: GlobalPodSelector1,
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
	testObj := new(MockPodController)
	testObj.On("GetPods", GlobalQuery1).Return(GlobalPodsFound1, nil)
	manager := Init(testObj)
	result, err := manager.ParseDefaultPolicy(policy)
	CommonTest(t, err, result)

	assert.Len(t, result, 2)
	for _, fw := range result {
		assert.Len(t, fw.Chain, 1)

		assert.Equal(t, "ingress", fw.Chain[0].Name)
		assert.Equal(t, "drop", fw.Chain[0].Default_)
		assert.Len(t, fw.Chain[0].Rule, 1)

		assert.Equal(t, fw.Chain[0].Rule[0].Action, "forward")
		assert.Equal(t, fw.Chain[0].Rule[0].Src, "192.168.1.20/32")

		if fw.Chain[0].Rule[0].Dst != GlobalPodsFound1[0].Pod.Status.PodIP && fw.Chain[0].Rule[0].Dst != GlobalPodsFound1[1].Pod.Status.PodIP {
			assert.Fail(t, "unrecognized ip:", fw.Chain[0].Rule[0].Dst)
		}
	}
}

func TestSingleRuleIPBlockMultipleExceptions(t *testing.T) {

	policy := &networking_v1.NetworkPolicy{
		ObjectMeta: meta_v1.ObjectMeta{
			Name:      GlobalName1,
			Namespace: GlobalNamespace1,
		},
		Spec: networking_v1.NetworkPolicySpec{
			PodSelector: GlobalPodSelector1,
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
	testObj := new(MockPodController)
	testObj.On("GetPods", GlobalQuery1).Return(GlobalPodsFound1, nil)
	manager := Init(testObj)
	result, err := manager.ParseDefaultPolicy(policy)
	CommonTest(t, err, result)

	assert.Len(t, result, 2)

	for _, fw := range result {
		assert.Len(t, fw.Chain, 1)

		assert.Equal(t, "ingress", fw.Chain[0].Name)
		assert.Equal(t, "drop", fw.Chain[0].Default_)
		assert.Len(t, fw.Chain[0].Rule, 4)

		for _, rule := range fw.Chain[0].Rule {

			switch rule.Src {
			case "192.168.0.0/16":
				assert.Equal(t, rule.Action, "forward")
				assert.Equal(t, int32(0), rule.Sport)

			case "192.168.2.0/24":
			case "192.168.5.0/24":
			case "192.168.100.20/32":
				assert.Equal(t, "drop", rule.Action)
			default:
				assert.Fail(t, "ip is not correct")
			}

			if rule.Dst != GlobalPodsFound1[0].Pod.Status.PodIP && rule.Dst != GlobalPodsFound1[1].Pod.Status.PodIP {
				assert.Fail(t, "unrecognized ip:", fw.Chain[0].Rule[0].Dst)
			}
		}
	}
}

func TestSingleRuleIPBlockMultipleExceptionsWithProtocolNoPort(t *testing.T) {

	tcp := core_v1.ProtocolTCP
	policy := &networking_v1.NetworkPolicy{
		ObjectMeta: meta_v1.ObjectMeta{
			Name:      GlobalName1,
			Namespace: GlobalNamespace1,
		},
		Spec: networking_v1.NetworkPolicySpec{
			PodSelector: GlobalPodSelector1,
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
	testObj := new(MockPodController)
	testObj.On("GetPods", GlobalQuery1).Return(GlobalPodsFound1, nil)
	manager := Init(testObj)
	result, err := manager.ParseDefaultPolicy(policy)
	CommonTest(t, err, result)

	assert.Len(t, result, 2)

	for _, fw := range result {
		assert.Len(t, fw.Chain, 1)

		assert.Equal(t, "ingress", fw.Chain[0].Name)
		assert.Equal(t, "drop", fw.Chain[0].Default_)
		assert.Len(t, fw.Chain[0].Rule, 4)

		for _, rule := range fw.Chain[0].Rule {

			switch rule.Src {
			case "192.168.0.0/16":
				assert.Equal(t, "forward", rule.Action, rule.Action)
				assert.Zero(t, rule.Sport)
				assert.Equal(t, "TCP", rule.L4proto)

			case "192.168.2.0/24":
			case "192.168.5.0/24":
			case "192.168.100.20/32":
				assert.Equal(t, "drop", rule.Action)
				assert.Zero(t, rule.Sport)
				assert.Equal(t, "TCP", rule.L4proto)
			default:
				assert.Fail(t, "ip is not correct")
			}

			if rule.Dst != GlobalPodsFound1[0].Pod.Status.PodIP && rule.Dst != GlobalPodsFound1[1].Pod.Status.PodIP {
				assert.Fail(t, "unrecognized ip:", fw.Chain[0].Rule[0].Dst)
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
		ObjectMeta: meta_v1.ObjectMeta{
			Name:      GlobalName1,
			Namespace: GlobalNamespace1,
		},
		Spec: networking_v1.NetworkPolicySpec{
			PodSelector: GlobalPodSelector1,
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
	testObj := new(MockPodController)
	testObj.On("GetPods", GlobalQuery1).Return(GlobalPodsFound1, nil)
	manager := Init(testObj)
	result, err := manager.ParseDefaultPolicy(policy)
	CommonTest(t, err, result)

	assert.Len(t, result, 2)

	for _, fw := range result {
		assert.Len(t, fw.Chain, 1)

		assert.Equal(t, "ingress", fw.Chain[0].Name)
		assert.Equal(t, "drop", fw.Chain[0].Default_)
		assert.Len(t, fw.Chain[0].Rule, 4)

		for _, rule := range fw.Chain[0].Rule {

			switch rule.Src {
			case "192.168.0.0/16":
				assert.Equal(t, "forward", rule.Action)
				assert.Equal(t, port.IntVal, rule.Sport)
				assert.Equal(t, "TCP", rule.L4proto)

			case "192.168.2.0/24":
			case "192.168.5.0/24":
			case "192.168.100.20/32":
				assert.Equal(t, "drop", rule.Action)
				assert.Equal(t, port.IntVal, rule.Sport)
				assert.Equal(t, "TCP", rule.L4proto)
			default:
				assert.Fail(t, "ip is not correct")
			}

			if rule.Dst != GlobalPodsFound1[0].Pod.Status.PodIP && rule.Dst != GlobalPodsFound1[1].Pod.Status.PodIP {
				assert.Fail(t, "unrecognized ip:", fw.Chain[0].Rule[0].Dst)
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
		ObjectMeta: meta_v1.ObjectMeta{
			Name:      GlobalName1,
			Namespace: GlobalNamespace1,
		},
		Spec: networking_v1.NetworkPolicySpec{
			PodSelector: GlobalPodSelector1,
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
	testObj := new(MockPodController)
	testObj.On("GetPods", GlobalQuery1).Return(GlobalPodsFound1, nil)
	manager := Init(testObj)
	result, err := manager.ParseDefaultPolicy(policy)
	CommonTest(t, err, result)

	assert.Len(t, result, 2)

	for _, fw := range result {
		assert.Len(t, fw.Chain, 1)

		assert.Equal(t, "ingress", fw.Chain[0].Name)
		assert.Equal(t, "drop", fw.Chain[0].Default_)
		assert.Len(t, fw.Chain[0].Rule, 8)

		allValues := make(map[string]int)
		for _, rule := range fw.Chain[0].Rule {

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

			if rule.Dst != GlobalPodsFound1[0].Pod.Status.PodIP && rule.Dst != GlobalPodsFound1[1].Pod.Status.PodIP {
				assert.Fail(t, "unrecognized ip:", fw.Chain[0].Rule[0].Dst)
			}
		}

		for _, occurrences := range allValues {
			assert.Equal(t, occurrences, 2)
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
		ObjectMeta: meta_v1.ObjectMeta{
			Name:      GlobalName1,
			Namespace: GlobalNamespace1,
		},
		Spec: networking_v1.NetworkPolicySpec{
			PodSelector: GlobalPodSelector1,
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
	testObj := new(MockPodController)
	testObj.On("GetPods", GlobalQuery1).Return(GlobalPodsFound1, nil)
	manager := Init(testObj)
	result, err := manager.ParseDefaultPolicy(policy)
	CommonTest(t, err, result)

	assert.Len(t, result, 2)

	for _, fw := range result {
		assert.Len(t, fw.Chain, 1)

		assert.Equal(t, "ingress", fw.Chain[0].Name)
		assert.Equal(t, "drop", fw.Chain[0].Default_)
		assert.Len(t, fw.Chain[0].Rule, 4)

		for _, rule := range fw.Chain[0].Rule {

			switch rule.Src {
			case "192.168.0.0/16":
				assert.Equal(t, "forward", rule.Action)
				assert.Equal(t, port.IntVal, rule.Sport)
				assert.Equal(t, "UDP", rule.L4proto)

			case "192.168.2.0/24":
			case "192.168.5.0/24":
			case "192.168.100.20/32":
				assert.Equal(t, "drop", rule.Action)
				assert.Equal(t, port.IntVal, rule.Sport)
				assert.Equal(t, "UDP", rule.L4proto)
			default:
				assert.Fail(t, "ip is not correct")
			}

			if rule.Dst != GlobalPodsFound1[0].Pod.Status.PodIP && rule.Dst != GlobalPodsFound1[1].Pod.Status.PodIP {
				assert.Fail(t, "unrecognized ip:", fw.Chain[0].Rule[0].Dst)
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
		ObjectMeta: meta_v1.ObjectMeta{
			Name:      GlobalName1,
			Namespace: GlobalNamespace1,
		},
		Spec: networking_v1.NetworkPolicySpec{
			PodSelector: GlobalPodSelector1,
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
	testObj := new(MockPodController)
	testObj.On("GetPods", GlobalQuery1).Return(GlobalPodsFound1, nil)
	manager := Init(testObj)
	result, err := manager.ParseDefaultPolicy(policy)
	CommonTest(t, err, result)

	assert.Len(t, result, 2)

	for _, fw := range result {
		assert.Len(t, fw.Chain, 1)

		assert.Equal(t, "ingress", fw.Chain[0].Name)
		assert.Equal(t, "drop", fw.Chain[0].Default_)
		assert.Len(t, fw.Chain[0].Rule, 0)
	}
}

func TestProtocolIsNil(t *testing.T) {

	port := &intstr.IntOrString{
		IntVal: 6895,
	}
	policy := &networking_v1.NetworkPolicy{
		ObjectMeta: meta_v1.ObjectMeta{
			Name:      GlobalName1,
			Namespace: GlobalNamespace1,
		},
		Spec: networking_v1.NetworkPolicySpec{
			PodSelector: GlobalPodSelector1,
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
	testObj := new(MockPodController)
	testObj.On("GetPods", GlobalQuery1).Return(GlobalPodsFound1, nil)
	manager := Init(testObj)
	result, err := manager.ParseDefaultPolicy(policy)
	CommonTest(t, err, result)

	assert.Len(t, result, 2)

	for _, fw := range result {
		assert.Len(t, fw.Chain, 1)

		assert.Equal(t, "ingress", fw.Chain[0].Name)
		assert.Equal(t, "drop", fw.Chain[0].Default_)
		assert.Len(t, fw.Chain[0].Rule, 8)

		allValues := make(map[string]int)
		for _, rule := range fw.Chain[0].Rule {

			switch rule.Src {
			case "192.168.0.0/16":
				assert.Equal(t, "forward", rule.Action)
				assert.Equal(t, port.IntVal, rule.Sport)

			case "192.168.2.0/24":
			case "192.168.5.0/24":
			case "192.168.100.20/32":
				assert.Equal(t, "drop", rule.Action)
				assert.Equal(t, port.IntVal, rule.Sport)
				if rule.L4proto != "TCP" && rule.L4proto != "UDP" {
					assert.Fail(t, "protocol is invalid", rule.L4proto)
				}
				allValues[rule.Src]++
			default:
				assert.Fail(t, "ip is not correct")
			}

			if rule.Dst != GlobalPodsFound1[0].Pod.Status.PodIP && rule.Dst != GlobalPodsFound1[1].Pod.Status.PodIP {
				assert.Fail(t, "unrecognized ip:", fw.Chain[0].Rule[0].Dst)
			}
		}

		for _, occurrences := range allValues {
			assert.Equal(t, occurrences, 2)
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
		ObjectMeta: meta_v1.ObjectMeta{
			Name:      GlobalName1,
			Namespace: GlobalNamespace1,
		},
		Spec: networking_v1.NetworkPolicySpec{
			PodSelector: GlobalPodSelector1,
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
	testObj.On("GetPods", GlobalQuery1).Return(GlobalPodsFound1, nil)
	testObj.On("GetPods", podquery.Query{
		Pod: []podquery.QueryObject{
			podquery.QueryObject{
				By:     "labels",
				Labels: labels,
			},
		},
		Namespace: []podquery.QueryObject{
			podquery.QueryObject{
				By:   "name",
				Name: GlobalNamespace1,
			},
		},
	}).Return(podsFound, nil)
	manager := Init(testObj)
	result, err := manager.ParseDefaultPolicy(policy)
	CommonTest(t, err, result)

	assert.Len(t, result, 2)

	for _, fw := range result {
		assert.Len(t, fw.Chain, 1)

		assert.Equal(t, "ingress", fw.Chain[0].Name)
		assert.Equal(t, "drop", fw.Chain[0].Default_)
		assert.Len(t, fw.Chain[0].Rule, 1)

		for _, rule := range fw.Chain[0].Rule {
			assert.Equal(t, "forward", rule.Action)
			assert.Equal(t, podsFound[0].Pod.Status.PodIP, rule.Src)
			assert.Equal(t, "TCP", rule.L4proto)
			assert.Equal(t, port.IntVal, rule.Sport)

			if rule.Dst != GlobalPodsFound1[0].Pod.Status.PodIP && rule.Dst != GlobalPodsFound1[1].Pod.Status.PodIP {
				assert.Fail(t, "unrecognized ip:", fw.Chain[0].Rule[0].Dst)
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
		ObjectMeta: meta_v1.ObjectMeta{
			Name:      GlobalName1,
			Namespace: GlobalNamespace1,
		},
		Spec: networking_v1.NetworkPolicySpec{
			PodSelector: GlobalPodSelector1,
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
	testObj.On("GetPods", GlobalQuery1).Return(GlobalPodsFound1, nil)
	testObj.On("GetPods", podquery.Query{
		Pod: []podquery.QueryObject{
			podquery.QueryObject{
				By:     "labels",
				Labels: labels,
			},
		},
		Namespace: []podquery.QueryObject{
			podquery.QueryObject{
				By:   "name",
				Name: GlobalNamespace1,
			},
		},
	}).Return(podsFound, nil)
	manager := Init(testObj)
	result, err := manager.ParseDefaultPolicy(policy)
	CommonTest(t, err, result)

	assert.Len(t, result, 2)

	for _, fw := range result {
		assert.Len(t, fw.Chain, 1)

		assert.Equal(t, "ingress", fw.Chain[0].Name)
		assert.Equal(t, "drop", fw.Chain[0].Default_)
		assert.Len(t, fw.Chain[0].Rule, 0)
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
		ObjectMeta: meta_v1.ObjectMeta{
			Name:      GlobalName1,
			Namespace: GlobalNamespace1,
		},
		Spec: networking_v1.NetworkPolicySpec{
			PodSelector: GlobalPodSelector1,
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
	testObj.On("GetPods", GlobalQuery1).Return(GlobalPodsFound1, nil)
	testObj.On("GetPods", podquery.Query{
		Pod: []podquery.QueryObject{
			podquery.QueryObject{
				By:     "labels",
				Labels: labels,
			},
		},
		Namespace: []podquery.QueryObject{
			podquery.QueryObject{
				By:   "name",
				Name: GlobalNamespace1,
			},
		},
	}).Return(podsFound, nil)
	manager := Init(testObj)
	result, err := manager.ParseDefaultPolicy(policy)
	CommonTest(t, err, result)

	assert.Len(t, result, 2)

	for _, fw := range result {
		assert.Len(t, fw.Chain, 1)
		assert.Equal(t, "ingress", fw.Chain[0].Name)

		var found int
		for _, rule := range fw.Chain[0].Rule {

			switch rule.Src {
			case "172.25.153.77", "172.10.30.30":
				found++
				assert.Equal(t, port.IntVal, rule.Sport)
				assert.Equal(t, "forward", rule.Action)
				assert.Equal(t, "TCP", rule.L4proto)
			default:
				assert.Fail(t, "unrecognized ip")
			}

			if rule.Dst != GlobalPodsFound1[0].Pod.Status.PodIP && rule.Dst != GlobalPodsFound1[1].Pod.Status.PodIP {
				assert.Fail(t, "unrecognized ip:", fw.Chain[0].Rule[0].Dst)
			}
		}

		assert.Equal(t, 2, found)
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
		ObjectMeta: meta_v1.ObjectMeta{
			Name:      GlobalName1,
			Namespace: GlobalNamespace1,
		},
		Spec: networking_v1.NetworkPolicySpec{
			PodSelector: GlobalPodSelector1,
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

	testObj := new(MockPodController)
	testObj.On("GetPods", GlobalQuery1).Return(GlobalPodsFound1, nil)
	manager := Init(testObj)
	result, err := manager.ParseDefaultPolicy(policy)
	CommonTest(t, err, result)

	assert.Len(t, result, 2)

	for _, fw := range result {
		assert.Len(t, fw.Chain, 1)
		assert.Equal(t, "ingress", fw.Chain[0].Name)

		assert.Len(t, fw.Chain[0].Rule, 0)
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
			Name:      GlobalName1,
			Namespace: GlobalNamespace1,
		},
		Spec: networking_v1.NetworkPolicySpec{
			PodSelector: GlobalPodSelector1,
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
					Namespace: GlobalNamespace1,
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
					Namespace: GlobalNamespace1,
				},
				Status: core_v1.PodStatus{
					PodIP: "172.25.153.77",
				},
			},
			Veth: "veth312",
		},
	}

	testObj := new(MockPodController)
	testObj.On("GetPods", GlobalQuery1).Return(GlobalPodsFound1, nil)
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
				Name: GlobalNamespace1,
			},
		},
	}).Return(podsFound, nil)
	manager := Init(testObj)
	result, err := manager.ParseDefaultPolicy(policy)
	CommonTest(t, err, result)

	assert.Len(t, result, 2)

	for _, fw := range result {
		assert.Len(t, fw.Chain, 1)
		assert.Equal(t, "ingress", fw.Chain[0].Name)

		assert.Len(t, fw.Chain[0].Rule, 2)

		var found int
		for _, rule := range fw.Chain[0].Rule {

			switch rule.Src {
			case "172.25.153.77", "172.10.30.30":
				found++
				assert.Equal(t, port.IntVal, rule.Sport)
				assert.Equal(t, "forward", rule.Action)
			default:
				assert.Fail(t, "unrecognized ip")
			}

			if rule.Dst != GlobalPodsFound1[0].Pod.Status.PodIP && rule.Dst != GlobalPodsFound1[1].Pod.Status.PodIP {
				assert.Fail(t, "unrecognized ip:", rule.Dst)
			}
		}

		assert.Equal(t, 2, found)
	}
}

func TestEmptyPodSelectorSelectsAllPodsInSameNamespaceIpsAreDifferent(t *testing.T) {
	tcp := core_v1.ProtocolTCP
	port := &intstr.IntOrString{
		IntVal: 6895,
	}
	labels := map[string]string{}
	policy := &networking_v1.NetworkPolicy{
		ObjectMeta: meta_v1.ObjectMeta{
			Name:      GlobalName1,
			Namespace: GlobalNamespace1,
		},
		Spec: networking_v1.NetworkPolicySpec{
			PodSelector: GlobalPodSelector1,
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

	testObj := new(MockPodController)
	testObj.On("GetPods", GlobalQuery1).Return(GlobalPodsFound1, nil)
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
				Name: GlobalNamespace1,
			},
		},
	}).Return(GlobalPodsFound1, nil)
	manager := Init(testObj)
	result, err := manager.ParseDefaultPolicy(policy)
	CommonTest(t, err, result)

	assert.Len(t, result, 2)

	for _, fw := range result {
		assert.Len(t, fw.Chain, 1)
		assert.Equal(t, "ingress", fw.Chain[0].Name)

		assert.Len(t, fw.Chain[0].Rule, 2)

		var found int
		for _, rule := range fw.Chain[0].Rule {

			switch rule.Src {
			case GlobalPodsFound1[0].Pod.Status.PodIP, GlobalPodsFound1[1].Pod.Status.PodIP:
				found++
				assert.Equal(t, port.IntVal, rule.Sport)
				assert.Equal(t, "forward", rule.Action)
			default:
				assert.Fail(t, "unrecognized ip")
			}

			if rule.Dst != GlobalPodsFound1[0].Pod.Status.PodIP && rule.Dst != GlobalPodsFound1[1].Pod.Status.PodIP {
				assert.Fail(t, "unrecognized ip:", fw.Chain[0].Rule[0].Dst)
			}
			assert.NotEqual(t, rule.Dst, rule.Src)
		}
		assert.Equal(t, 2, found)
	}
}

func TestNilLabelSelectorBlocksAllPodsInSameNamespace(t *testing.T) {
	tcp := core_v1.ProtocolTCP
	port := &intstr.IntOrString{
		IntVal: 6895,
	}

	policy := &networking_v1.NetworkPolicy{
		ObjectMeta: meta_v1.ObjectMeta{
			Name:      GlobalName1,
			Namespace: GlobalNamespace1,
		},
		Spec: networking_v1.NetworkPolicySpec{
			PodSelector: GlobalPodSelector1,
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
					Namespace: GlobalNamespace1,
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
					Namespace: GlobalNamespace1,
				},
				Status: core_v1.PodStatus{
					PodIP: "172.25.153.77",
				},
			},
			Veth: "veth312",
		},
	}
	testObj := new(MockPodController)
	testObj.On("GetPods", GlobalQuery1).Return(GlobalPodsFound1, nil)
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
				Name: GlobalNamespace1,
			},
		},
	}).Return(podsFound, nil)
	manager := Init(testObj)
	result, err := manager.ParseDefaultPolicy(policy)
	CommonTest(t, err, result)

	assert.Len(t, result, 2)

	for _, fw := range result {
		assert.Len(t, fw.Chain, 1)
		assert.Equal(t, "ingress", fw.Chain[0].Name)

		assert.Len(t, fw.Chain[0].Rule, 2)

		var found int
		for _, rule := range fw.Chain[0].Rule {

			switch rule.Src {
			case "172.25.153.77", "172.10.30.30":
				found++
				assert.Equal(t, rule.Sport, port.IntVal)
				assert.Equal(t, rule.Action, "drop")
			default:
				assert.Fail(t, "unrecognized ip")
			}

			if rule.Dst != GlobalPodsFound1[0].Pod.Status.PodIP && rule.Dst != GlobalPodsFound1[1].Pod.Status.PodIP {
				assert.Fail(t, "unrecognized ip:", fw.Chain[0].Rule[0].Dst)
			}
			assert.NotEqual(t, rule.Dst, rule.Src)
		}
		assert.Equal(t, 2, found)
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
			Name:      GlobalName1,
			Namespace: GlobalNamespace1,
		},
		Spec: networking_v1.NetworkPolicySpec{
			PodSelector: GlobalPodSelector1,
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
					Namespace: GlobalNamespace1,
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
					Namespace: GlobalNamespace1,
				},
				Status: core_v1.PodStatus{
					PodIP: "172.25.153.77",
				},
			},
			Veth: "veth312",
		},
	}
	testObj := new(MockPodController)
	testObj.On("GetPods", GlobalQuery1).Return(GlobalPodsFound1, nil)
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

	assert.Len(t, result, 2)

	for _, fw := range result {
		assert.Len(t, fw.Chain, 1)
		assert.Equal(t, "ingress", fw.Chain[0].Name)

		assert.Len(t, fw.Chain[0].Rule, 2)

		var found int
		for _, rule := range fw.Chain[0].Rule {

			switch rule.Src {
			case "172.10.30.30", "172.25.153.77":
				found++
				assert.Equal(t, rule.Sport, port.IntVal)
				assert.Equal(t, rule.Action, "forward")
			default:
				assert.Fail(t, "unrecognized ip")
			}

			if rule.Dst != GlobalPodsFound1[0].Pod.Status.PodIP && rule.Dst != GlobalPodsFound1[1].Pod.Status.PodIP {
				assert.Fail(t, "unrecognized ip:", fw.Chain[0].Rule[0].Dst)
			}
			assert.NotEqual(t, rule.Dst, rule.Src)
		}
		assert.Equal(t, 2, found)
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
			Name:      GlobalName1,
			Namespace: GlobalNamespace1,
		},
		Spec: networking_v1.NetworkPolicySpec{
			PodSelector: GlobalPodSelector1,
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
					Namespace: GlobalNamespace1,
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
					Namespace: GlobalNamespace1,
				},
				Status: core_v1.PodStatus{
					PodIP: "172.25.153.77",
				},
			},
			Veth: "veth312",
		},
	}
	testObj := new(MockPodController)
	testObj.On("GetPods", GlobalQuery1).Return(GlobalPodsFound1, nil)
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

	assert.Len(t, result, 2)

	for _, fw := range result {
		assert.Len(t, fw.Chain, 1)
		assert.Equal(t, "ingress", fw.Chain[0].Name)

		assert.Len(t, fw.Chain[0].Rule, 2)

		var found int
		for _, rule := range fw.Chain[0].Rule {

			switch rule.Src {
			case "172.10.30.30", "172.25.153.77":
				found++
				assert.Equal(t, rule.Sport, port.IntVal)
				assert.Equal(t, rule.Action, "drop")
			default:
				assert.Fail(t, "unrecognized ip")
			}

			if rule.Dst != GlobalPodsFound1[0].Pod.Status.PodIP && rule.Dst != GlobalPodsFound1[1].Pod.Status.PodIP {
				assert.Fail(t, "unrecognized ip:", fw.Chain[0].Rule[0].Dst)
			}
			assert.NotEqual(t, rule.Dst, rule.Src)
		}
		assert.Equal(t, 2, found)
	}
}

func TestIngressComplexPolicy(t *testing.T) {
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
			Name:      GlobalName1,
			Namespace: GlobalNamespace1,
		},
		Spec: networking_v1.NetworkPolicySpec{
			PodSelector: GlobalPodSelector1,
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
	testObj.On("GetPods", GlobalQuery1).Return(GlobalPodsFound1, nil)
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

	assert.Len(t, result, 2)

	for _, fw := range result {
		assert.Len(t, fw.Chain, 1)
		assert.Equal(t, "ingress", fw.Chain[0].Name)
		assert.Len(t, fw.Chain[0].Rule, 14+12+2)
		for _, rule := range fw.Chain[0].Rule {
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
				t.Log(rule)
				assert.Fail(t, "unrecognized ip")
			}

			if rule.Dst != GlobalPodsFound1[0].Pod.Status.PodIP && rule.Dst != GlobalPodsFound1[1].Pod.Status.PodIP {
				assert.Fail(t, "unrecognized ip:", fw.Chain[0].Rule[0].Dst)
			}
			assert.NotEqual(t, rule.Dst, rule.Src)
		}
	}
}

func TestEgressDoesNotExist(t *testing.T) {
	tcp := core_v1.ProtocolTCP
	udp := core_v1.ProtocolUDP

	port1 := &intstr.IntOrString{
		IntVal: 6895,
	}

	podLabels := map[string]string{
		"type":    "app",
		"version": "2.0",
	}

	policy := &networking_v1.NetworkPolicy{
		ObjectMeta: meta_v1.ObjectMeta{
			Name:      GlobalName1,
			Namespace: GlobalNamespace1,
		},
		Spec: networking_v1.NetworkPolicySpec{
			PodSelector: GlobalPodSelector1,

			Ingress: []networking_v1.NetworkPolicyIngressRule{

				networking_v1.NetworkPolicyIngressRule{
					From: []networking_v1.NetworkPolicyPeer{
						networking_v1.NetworkPolicyPeer{

							PodSelector: &meta_v1.LabelSelector{
								MatchLabels: podLabels,
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
		},
	}

	podsFound := []polycube_pod.Pod{
		polycube_pod.Pod{
			Pod: core_v1.Pod{
				ObjectMeta: meta_v1.ObjectMeta{
					Namespace: GlobalNamespace1,
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
					Namespace: GlobalNamespace1,
				},
				Status: core_v1.PodStatus{
					PodIP: "172.30.30.30",
				},
			},
			Veth: "veth312",
		},
	}
	testObj := new(MockPodController)
	testObj.On("GetPods", GlobalQuery1).Return(GlobalPodsFound1, nil)
	testObj.On("GetPods", podquery.Query{
		Pod: []podquery.QueryObject{
			podquery.QueryObject{
				By:     "labels",
				Labels: podLabels,
			},
		},
		Namespace: []podquery.QueryObject{
			podquery.QueryObject{
				By:   "name",
				Name: "Production",
			},
		},
	}).Return(podsFound, nil)
	manager := Init(testObj)
	result, err := manager.ParseDefaultPolicy(policy)
	CommonTest(t, err, result)

	assert.Len(t, result, 2)

	for _, fw := range result {
		assert.Len(t, fw.Chain, 1)
		assert.NotEqual(t, "egress", fw.Chain[0].Name)
		assert.Len(t, fw.Chain[0].Rule, 4)
	}
}

func TestOnlyEgress(t *testing.T) {
	tcp := core_v1.ProtocolTCP
	udp := core_v1.ProtocolUDP

	port1 := &intstr.IntOrString{
		IntVal: 6895,
	}

	podLabels := map[string]string{
		"type":    "app",
		"version": "2.0",
	}

	policy := &networking_v1.NetworkPolicy{
		ObjectMeta: meta_v1.ObjectMeta{
			Name:      GlobalName1,
			Namespace: GlobalNamespace1,
		},
		Spec: networking_v1.NetworkPolicySpec{
			PodSelector: GlobalPodSelector1,
			PolicyTypes: []networking_v1.PolicyType{
				networking_v1.PolicyTypeEgress,
			},
			Egress: []networking_v1.NetworkPolicyEgressRule{

				networking_v1.NetworkPolicyEgressRule{
					To: []networking_v1.NetworkPolicyPeer{
						networking_v1.NetworkPolicyPeer{

							PodSelector: &meta_v1.LabelSelector{
								MatchLabels: podLabels,
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
		},
	}

	podsFound := []polycube_pod.Pod{
		polycube_pod.Pod{
			Pod: core_v1.Pod{
				ObjectMeta: meta_v1.ObjectMeta{
					Namespace: GlobalNamespace1,
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
					Namespace: GlobalNamespace1,
				},
				Status: core_v1.PodStatus{
					PodIP: "172.30.30.30",
				},
			},
			Veth: "veth312",
		},
	}
	testObj := new(MockPodController)
	testObj.On("GetPods", GlobalQuery1).Return(GlobalPodsFound1, nil)
	testObj.On("GetPods", podquery.Query{
		Pod: []podquery.QueryObject{
			podquery.QueryObject{
				By:     "labels",
				Labels: podLabels,
			},
		},
		Namespace: []podquery.QueryObject{
			podquery.QueryObject{
				By:   "name",
				Name: "Production",
			},
		},
	}).Return(podsFound, nil)
	manager := Init(testObj)
	result, err := manager.ParseDefaultPolicy(policy)
	CommonTest(t, err, result)

	assert.Len(t, result, 2)

	for _, fw := range result {
		assert.Len(t, fw.Chain, 1)
		assert.Equal(t, "egress", fw.Chain[0].Name)
		assert.Len(t, fw.Chain[0].Rule, 4)

		for _, rule := range fw.Chain[0].Rule {
			if rule.Dst != "172.30.30.30" && rule.Dst != "172.20.20.20" {
				assert.Fail(t, "unrecognized ip")
			}
			assert.Equal(t, port1.IntVal, rule.Dport)
			if rule.L4proto != "TCP" && rule.L4proto != "UDP" {
				assert.Fail(t, "unrecognized L4Proto")
			}
			assert.Equal(t, "forward", rule.Action)

			if rule.Src != GlobalPodsFound1[0].Pod.Status.PodIP && rule.Src != GlobalPodsFound1[1].Pod.Status.PodIP {
				assert.Fail(t, "unrecognized ip:", fw.Chain[0].Rule[0].Src)
			}
			assert.NotEqual(t, rule.Dst, rule.Src)
		}
	}
}

func TestEgressBlockPodsFromSpecificNamespace(t *testing.T) {
	tcp := core_v1.ProtocolTCP
	udp := core_v1.ProtocolUDP

	port1 := &intstr.IntOrString{
		IntVal: 6895,
	}

	namespaceLabels := map[string]string{
		"environment": "production",
	}

	policy := &networking_v1.NetworkPolicy{
		ObjectMeta: meta_v1.ObjectMeta{
			Name:      GlobalName1,
			Namespace: GlobalNamespace1,
		},
		Spec: networking_v1.NetworkPolicySpec{
			PodSelector: GlobalPodSelector1,
			PolicyTypes: []networking_v1.PolicyType{
				networking_v1.PolicyTypeEgress,
			},
			Egress: []networking_v1.NetworkPolicyEgressRule{

				networking_v1.NetworkPolicyEgressRule{
					To: []networking_v1.NetworkPolicyPeer{
						networking_v1.NetworkPolicyPeer{

							PodSelector: &meta_v1.LabelSelector{
								MatchLabels: nil,
							},
							NamespaceSelector: &meta_v1.LabelSelector{
								MatchLabels: namespaceLabels,
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
		},
	}

	podsFound := []polycube_pod.Pod{
		polycube_pod.Pod{
			Pod: core_v1.Pod{
				ObjectMeta: meta_v1.ObjectMeta{
					Namespace: GlobalNamespace1,
				},
				Status: core_v1.PodStatus{
					PodIP: "172.50.50.50",
				},
			},
			Veth: "veth123",
		},
		polycube_pod.Pod{
			Pod: core_v1.Pod{
				ObjectMeta: meta_v1.ObjectMeta{
					Namespace: GlobalNamespace1,
				},
				Status: core_v1.PodStatus{
					PodIP: "172.60.60.60",
				},
			},
			Veth: "veth312",
		},
		polycube_pod.Pod{
			Pod: core_v1.Pod{
				ObjectMeta: meta_v1.ObjectMeta{
					Namespace: GlobalNamespace1,
				},
				Status: core_v1.PodStatus{
					PodIP: "172.70.70.70",
				},
			},
			Veth: "veth317",
		},
	}
	testObj := new(MockPodController)
	testObj.On("GetPods", GlobalQuery1).Return(GlobalPodsFound1, nil)
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
				Labels: namespaceLabels,
			},
		},
	}).Return(podsFound, nil)
	manager := Init(testObj)
	result, err := manager.ParseDefaultPolicy(policy)
	CommonTest(t, err, result)

	assert.Len(t, result, 2)

	for _, fw := range result {
		assert.Len(t, fw.Chain, 1)
		assert.Equal(t, "egress", fw.Chain[0].Name)
		assert.Len(t, fw.Chain[0].Rule, 6)

		for _, rule := range fw.Chain[0].Rule {
			if rule.Dst != "172.50.50.50" && rule.Dst != "172.60.60.60" && rule.Dst != "172.70.70.70" {
				assert.Fail(t, "unrecognized ip")
			}
			assert.Equal(t, port1.IntVal, rule.Dport)
			if rule.L4proto != "TCP" && rule.L4proto != "UDP" {
				assert.Fail(t, "unrecognized L4Proto")
			}
			assert.Equal(t, "drop", rule.Action)

			if rule.Src != GlobalPodsFound1[0].Pod.Status.PodIP && rule.Src != GlobalPodsFound1[1].Pod.Status.PodIP {
				assert.Fail(t, "unrecognized ip:", fw.Chain[0].Rule[0].Src)
			}
			assert.NotEqual(t, rule.Dst, rule.Src)
		}
	}
}

func TestEgressAllowSameNamespace(t *testing.T) {
	tcp := core_v1.ProtocolTCP
	udp := core_v1.ProtocolUDP

	port1 := &intstr.IntOrString{
		IntVal: 7000,
	}

	policy := &networking_v1.NetworkPolicy{
		ObjectMeta: meta_v1.ObjectMeta{
			Name:      GlobalName1,
			Namespace: GlobalNamespace1,
		},
		Spec: networking_v1.NetworkPolicySpec{
			PodSelector: GlobalPodSelector1,
			PolicyTypes: []networking_v1.PolicyType{
				networking_v1.PolicyTypeEgress,
			},
			Egress: []networking_v1.NetworkPolicyEgressRule{

				networking_v1.NetworkPolicyEgressRule{
					To: []networking_v1.NetworkPolicyPeer{
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
			},
		},
	}

	podsFound := []polycube_pod.Pod{
		polycube_pod.Pod{
			Pod: core_v1.Pod{
				ObjectMeta: meta_v1.ObjectMeta{
					Namespace: GlobalNamespace1,
				},
				Status: core_v1.PodStatus{
					PodIP: "172.50.50.50",
				},
			},
			Veth: "veth123",
		},
		polycube_pod.Pod{
			Pod: core_v1.Pod{
				ObjectMeta: meta_v1.ObjectMeta{
					Namespace: GlobalNamespace1,
				},
				Status: core_v1.PodStatus{
					PodIP: "172.60.60.60",
				},
			},
			Veth: "veth312",
		},
		polycube_pod.Pod{
			Pod: core_v1.Pod{
				ObjectMeta: meta_v1.ObjectMeta{
					Namespace: GlobalNamespace1,
				},
				Status: core_v1.PodStatus{
					PodIP: "172.70.70.70",
				},
			},
			Veth: "veth317",
		},
	}
	testObj := new(MockPodController)
	testObj.On("GetPods", GlobalQuery1).Return(GlobalPodsFound1, nil)
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
				Name: "Production",
			},
		},
	}).Return(podsFound, nil)
	manager := Init(testObj)
	result, err := manager.ParseDefaultPolicy(policy)
	CommonTest(t, err, result)

	assert.Len(t, result, 2)

	for _, fw := range result {
		assert.Len(t, fw.Chain, 1)
		assert.Equal(t, "egress", fw.Chain[0].Name)
		assert.Len(t, fw.Chain[0].Rule, 6)

		for _, rule := range fw.Chain[0].Rule {
			if rule.Dst != "172.50.50.50" && rule.Dst != "172.60.60.60" && rule.Dst != "172.70.70.70" {
				assert.Fail(t, "unrecognized ip")
			}
			assert.Equal(t, port1.IntVal, rule.Dport)
			if rule.L4proto != "TCP" && rule.L4proto != "UDP" {
				assert.Fail(t, "unrecognized L4Proto")
			}
			assert.Equal(t, "forward", rule.Action)

			if rule.Src != GlobalPodsFound1[0].Pod.Status.PodIP && rule.Src != GlobalPodsFound1[1].Pod.Status.PodIP {
				assert.Fail(t, "unrecognized ip:", fw.Chain[0].Rule[0].Src)
			}
			assert.NotEqual(t, rule.Dst, rule.Src)
		}
	}
}

func TestEgressNothingFound(t *testing.T) {
	tcp := core_v1.ProtocolTCP
	udp := core_v1.ProtocolUDP

	port1 := &intstr.IntOrString{
		IntVal: 7000,
	}

	policy := &networking_v1.NetworkPolicy{
		ObjectMeta: meta_v1.ObjectMeta{
			Namespace: GlobalNamespace1,
		},
		Spec: networking_v1.NetworkPolicySpec{
			PodSelector: GlobalPodSelector1,
			PolicyTypes: []networking_v1.PolicyType{
				networking_v1.PolicyTypeEgress,
			},
			Egress: []networking_v1.NetworkPolicyEgressRule{

				networking_v1.NetworkPolicyEgressRule{
					To: []networking_v1.NetworkPolicyPeer{
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
			},
		},
	}

	testObj := new(MockPodController)
	testObj.On("GetPods", GlobalQuery1).Return(GlobalPodsFound1, nil)
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
				Name: "Production",
			},
		},
	}).Return([]polycube_pod.Pod{}, nil)
	manager := Init(testObj)
	result, err := manager.ParseDefaultPolicy(policy)
	CommonTest(t, err, result)

	assert.Len(t, result, 2)
	for _, fw := range result {
		assert.Len(t, fw.Chain, 1)
		assert.Equal(t, "egress", fw.Chain[0].Name)

		assert.Empty(t, fw.Chain[0].Rule)
		assert.Equal(t, "drop", fw.Chain[0].Default_)
	}
}

func TestEgressPolicyTypesEmptyIsIncorrectlyParsed(t *testing.T) {
	tcp := core_v1.ProtocolTCP
	udp := core_v1.ProtocolUDP

	port1 := &intstr.IntOrString{
		IntVal: 7000,
	}

	policy := &networking_v1.NetworkPolicy{
		ObjectMeta: meta_v1.ObjectMeta{
			Namespace: GlobalNamespace1,
		},
		Spec: networking_v1.NetworkPolicySpec{
			PodSelector: GlobalPodSelector1,
			/*PolicyTypes: []networking_v1.PolicyType{
				networking_v1.PolicyTypeEgress,
			},*/
			Egress: []networking_v1.NetworkPolicyEgressRule{

				networking_v1.NetworkPolicyEgressRule{
					To: []networking_v1.NetworkPolicyPeer{
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
			},
		},
	}

	testObj := new(MockPodController)
	testObj.On("GetPods", GlobalQuery1).Return(GlobalPodsFound1, nil)
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
				Name: "Production",
			},
		},
	}).Return([]polycube_pod.Pod{}, nil)
	manager := Init(testObj)
	result, err := manager.ParseDefaultPolicy(policy)
	CommonTest(t, err, result)

	assert.Len(t, result, 2)
	for _, fw := range result {
		assert.Len(t, fw.Chain, 2)

		assert.Equal(t, "drop", fw.Chain[0].Default_)
		assert.Equal(t, "drop", fw.Chain[1].Default_)

		assert.Empty(t, fw.Chain[0].Rule)
		assert.Empty(t, fw.Chain[1].Rule)
	}
}

func TestEmptyEgressBlocksEverything(t *testing.T) {
	policy := &networking_v1.NetworkPolicy{
		ObjectMeta: meta_v1.ObjectMeta{
			Name:      GlobalName1,
			Namespace: GlobalNamespace1,
		},
		Spec: networking_v1.NetworkPolicySpec{
			PodSelector: GlobalPodSelector1,
			PolicyTypes: []networking_v1.PolicyType{
				networking_v1.PolicyTypeEgress,
			},
			Egress: []networking_v1.NetworkPolicyEgressRule{},
		},
	}

	testObj := new(MockPodController)
	testObj.On("GetPods", GlobalQuery1).Return(GlobalPodsFound1, nil)
	manager := Init(testObj)
	result, err := manager.ParseDefaultPolicy(policy)
	CommonTest(t, err, result)

	assert.Len(t, result, 2)

	for _, fw := range result {
		assert.Len(t, fw.Chain, 1)
		assert.NotEqual(t, "ingress", fw.Chain[0].Name)
		assert.Len(t, fw.Chain[0].Rule, 0)
		assert.Equal(t, "drop", fw.Chain[0].Default_)
	}
}

func TestEgressBlockEverythingWithPort(t *testing.T) {
	tcp := core_v1.ProtocolTCP
	udp := core_v1.ProtocolUDP

	port1 := &intstr.IntOrString{
		IntVal: 6895,
	}

	policy := &networking_v1.NetworkPolicy{
		ObjectMeta: meta_v1.ObjectMeta{
			Name:      GlobalName1,
			Namespace: GlobalNamespace1,
		},
		Spec: networking_v1.NetworkPolicySpec{
			PodSelector: GlobalPodSelector1,
			PolicyTypes: []networking_v1.PolicyType{
				networking_v1.PolicyTypeEgress,
			},
			Egress: []networking_v1.NetworkPolicyEgressRule{
				networking_v1.NetworkPolicyEgressRule{
					To: []networking_v1.NetworkPolicyPeer{},
					Ports: []networking_v1.NetworkPolicyPort{
						networking_v1.NetworkPolicyPort{
							Port:     port1,
							Protocol: &tcp,
						},
						networking_v1.NetworkPolicyPort{
							Port:     port1,
							Protocol: &udp,
						},
					},
				},
			},
		},
	}

	testObj := new(MockPodController)
	testObj.On("GetPods", GlobalQuery1).Return(GlobalPodsFound1, nil)
	manager := Init(testObj)
	result, err := manager.ParseDefaultPolicy(policy)
	CommonTest(t, err, result)

	assert.Len(t, result, 2)

	for _, fw := range result {
		assert.Len(t, fw.Chain, 1)
		assert.NotEqual(t, "ingress", fw.Chain[0].Name)
		assert.Len(t, fw.Chain[0].Rule, 0)
		assert.Equal(t, "drop", fw.Chain[0].Default_)

		//	UPDATE: found out that this is never verified so....
		/*for _, rule := range fw.Chain[0].Rule {
			assert.Equal(t, port1.IntVal, rule.Dport)
			assert.Equal(t, "drop", rule.Action)
			if rule.L4proto != "UDP" && rule.L4proto != "TCP" {
				assert.Fail(t, "incorrect protocol", rule.L4proto)
			}
			if rule.Src != GlobalPodsFound1[0].Pod.Status.PodIP && rule.Src != GlobalPodsFound1[1].Pod.Status.PodIP {
				assert.Fail(t, "unrecognized ip:", fw.Chain[0].Rule[0].Src)
			}
			assert.NotEqual(t, rule.Dst, rule.Src)
		}*/
	}
}

func TestIngressEgressComplexPolicy(t *testing.T) {
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
			Name:      GlobalName1,
			Namespace: GlobalNamespace1,
		},
		Spec: networking_v1.NetworkPolicySpec{
			PodSelector: GlobalPodSelector1,
			PolicyTypes: []networking_v1.PolicyType{
				networking_v1.PolicyTypeIngress,
				networking_v1.PolicyTypeEgress,
			},
			Ingress: []networking_v1.NetworkPolicyIngressRule{

				/* Ingress - First rule
				Block the following if it comes with TCP6895 Or UDP6895
					- what comes from 192.168.2.0/24
					- what comes from 192.168.5.0/24
					- what comes from 192.168.100.20/32

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
									"192.168.5.0/24",
									"192.168.100.20/32",
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
				/*	Egress - First Rule
					Block the following if it comes with TCP8080 Or TCP80:
						- what comes from 192.169.2.0/24
						- what comes from 192.169.5.0/24
						- what comes from 192.169.100.20/32

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

				/*	Egress - Second Rule
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

	//	pods find for the first rule
	firstPodsFound := []polycube_pod.Pod{
		polycube_pod.Pod{
			Pod: core_v1.Pod{
				ObjectMeta: meta_v1.ObjectMeta{
					Namespace: GlobalNamespace1,
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
					Namespace: GlobalNamespace1,
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
					Namespace: GlobalNamespace1,
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
					Namespace: GlobalNamespace1,
				},
				Status: core_v1.PodStatus{
					PodIP: "172.30.30.30",
				},
			},
			Veth: "veth312",
		},
	}
	testObj := new(MockPodController)
	testObj.On("GetPods", GlobalQuery1).Return(GlobalPodsFound1, nil)
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

	assert.Len(t, result, 2)

	for _, fw := range result {
		assert.Len(t, fw.Chain, 2)

		for _, chain := range fw.Chain {

			if chain.Name == "ingress" {
				assert.Len(t, chain.Rule, 14)
				assert.Equal(t, "drop", chain.Default_)

				for _, rule := range chain.Rule {
					switch rule.Src {
					case "172.20.20.20", "172.30.30.30", "172.40.40.40", "192.168.0.0/16":
						assert.Equal(t, "forward", rule.Action)
						assert.Equal(t, port1.IntVal, rule.Sport)
						if rule.L4proto != "TCP" && rule.L4proto != "UDP" {
							assert.Fail(t, "ingress: unexpected l4proto", rule.L4proto)
						}
					case "192.168.2.0/24", "192.168.5.0/24", "192.168.100.20/32":
						assert.Equal(t, port1.IntVal, rule.Sport)
						if rule.L4proto != "TCP" && rule.L4proto != "UDP" {
							assert.Fail(t, "ingress: unexpected l4proto", rule.L4proto)
						}
						assert.Equal(t, "drop", rule.Action)
					default:
						assert.Fail(t, "ingress: unrecognized ip", rule.Src)
					}

					if rule.Dst != GlobalPodsFound1[0].Pod.Status.PodIP && rule.Dst != GlobalPodsFound1[1].Pod.Status.PodIP {
						assert.Fail(t, "ingress: unrecognized ip:", fw.Chain[0].Rule[0].Dst)
					}
					assert.NotEqual(t, rule.Dst, rule.Src)
				}
			}

			if chain.Name == "egress" {
				assert.Len(t, chain.Rule, 12+2)
				assert.Equal(t, "drop", chain.Default_)

				for _, rule := range chain.Rule {
					switch rule.Dst {
					case "172.20.20.20", "172.30.30.30", "192.169.0.0/16":
						assert.Equal(t, "forward", rule.Action)
						assert.Equal(t, "TCP", rule.L4proto)
						if rule.Dport != port2.IntVal && rule.Dport != port3.IntVal {
							assert.Fail(t, "egress: unexpected dport", rule.Dport)
						}
					case "192.169.2.0/24", "192.169.5.0/24", "192.169.100.20/32":
						assert.Equal(t, "drop", rule.Action)
						assert.Equal(t, "TCP", rule.L4proto)
						if rule.Dport != port2.IntVal && rule.Dport != port3.IntVal {
							assert.Fail(t, "egress: unexpected dport", rule.Dport)
						}
					case "0":
						assert.Equal(t, "forward", rule.Action)
						assert.Equal(t, port4.IntVal, rule.Dport)
						if rule.L4proto != "TCP" && rule.L4proto != "UDP" {
							assert.Fail(t, "egress: unexpected L4proto", rule.L4proto)
						}
					default:
						assert.Fail(t, "egress: unrecognized ip", rule.Dst)
					}

					if rule.Src != GlobalPodsFound1[0].Pod.Status.PodIP && rule.Src != GlobalPodsFound1[1].Pod.Status.PodIP {
						assert.Fail(t, "egress: unrecognized ip:", fw.Chain[0].Rule[0].Src)
					}
					assert.NotEqual(t, rule.Dst, rule.Src)
				}
			}
		}
	}
}
