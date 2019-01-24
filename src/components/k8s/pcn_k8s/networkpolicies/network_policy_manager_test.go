package networkpolicies

import (
	"testing"

	"github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/controllers"
	k8sfirewall "github.com/SunSince90/polycube/src/components/k8s/utils/k8sfirewall"
	"github.com/stretchr/testify/assert"
	core_v1 "k8s.io/api/core/v1"
	networking_v1 "k8s.io/api/networking/v1"
	"k8s.io/apimachinery/pkg/util/intstr"
)

type DnpcMock struct {
	controllers.DefaultNetworkPolicyController
}

func Init() *NetworkPolicyManager {

	//dnpc := &dnpcMock{}
	manager := NewNetworkPolicyManager(nil)

	return manager
	//dnpc := controllers.NewDefaultNetworkPolicyController(dnpcMock)

}

func TestNotNil(t *testing.T) {
	manager := Init()
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
	manager := Init()

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
	manager := Init()
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
	manager := Init()
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
	manager := Init()
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
	manager := Init()
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
	manager := Init()
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
	manager := Init()
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
	manager := Init()
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
	manager := Init()
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
	manager := Init()
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
	manager := Init()
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
