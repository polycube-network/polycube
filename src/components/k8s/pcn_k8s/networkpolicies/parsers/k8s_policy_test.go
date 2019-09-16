package parsers

import (
	"testing"

	meta_v1 "k8s.io/apimachinery/pkg/apis/meta/v1"

	pcn_types "github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/types"
	"github.com/stretchr/testify/assert"
	core_v1 "k8s.io/api/core/v1"
	networking_v1 "k8s.io/api/networking/v1"
	"k8s.io/apimachinery/pkg/util/intstr"
)

func TestProtocolAndPort(t *testing.T) {
	// -- Unsupported protocol
	SCTP := core_v1.ProtocolSCTP
	port := networking_v1.NetworkPolicyPort{
		Protocol: &SCTP,
		Port: &intstr.IntOrString{
			Type:   intstr.Int,
			IntVal: int32(5050),
		},
	}

	supported, pr, po := parseProtocolAndPort(port)
	assert.False(t, supported)
	assert.Empty(t, pr)
	assert.Zero(t, po)

	// -- TCP
	TCP := core_v1.ProtocolTCP
	port = networking_v1.NetworkPolicyPort{
		Protocol: &TCP,
		Port: &intstr.IntOrString{
			Type:   intstr.Int,
			IntVal: int32(5050),
		},
	}

	supported, pr, po = parseProtocolAndPort(port)
	assert.True(t, supported)
	assert.Equal(t, pr, "TCP")
	assert.Equal(t, po, port.Port.IntVal)

	// -- UDP
	UDP := core_v1.ProtocolUDP
	port = networking_v1.NetworkPolicyPort{
		Protocol: &UDP,
		Port: &intstr.IntOrString{
			Type:   intstr.Int,
			IntVal: int32(5151),
		},
	}

	supported, pr, po = parseProtocolAndPort(port)
	assert.True(t, supported)
	assert.Equal(t, pr, "UDP")
	assert.Equal(t, po, port.Port.IntVal)
}

func TestParseK8sPorts(t *testing.T) {
	assert := assert.New(t)

	// -- Protocol is nil
	t.Run("nil-protocol", func(t *testing.T) {
		port := networking_v1.NetworkPolicyPort{
			Port: &intstr.IntOrString{
				Type:   intstr.Int,
				IntVal: int32(5050),
			},
		}
		result := parseK8sPorts([]networking_v1.NetworkPolicyPort{
			port,
		}, pcn_types.PolicyIncoming)

		assert.Len(result, 1)
		r := result[0]
		assert.Zero(r.SPort)
		assert.Equal(r.DPort, port.Port.IntVal)
		assert.Empty(r.Protocol)
	})

	// -- TCP
	t.Run("tcp", func(t *testing.T) {
		TCP := core_v1.ProtocolTCP
		port := networking_v1.NetworkPolicyPort{
			Protocol: &TCP,
		}
		result := parseK8sPorts([]networking_v1.NetworkPolicyPort{
			port,
		}, pcn_types.PolicyOutgoing)

		assert.Len(result, 1)
		r := result[0]
		assert.Zero(r.DPort)
		assert.Zero(r.SPort)
		assert.Equal(r.Protocol, "TCP")
	})

	// -- Only Unsupported protocols
	t.Run("only-unsupported", func(t *testing.T) {
		SCTP := core_v1.ProtocolSCTP
		port := networking_v1.NetworkPolicyPort{
			Protocol: &SCTP,
		}
		result := parseK8sPorts([]networking_v1.NetworkPolicyPort{
			port,
		}, pcn_types.PolicyOutgoing)

		assert.Empty(result)
	})

	// -- Test directions
	t.Run("test-dirs", func(t *testing.T) {
		p := int32(5050)
		port := networking_v1.NetworkPolicyPort{
			Port: &intstr.IntOrString{
				Type:   intstr.Int,
				IntVal: p,
			},
		}

		// -- Incoming
		result := parseK8sPorts([]networking_v1.NetworkPolicyPort{
			port,
		}, pcn_types.PolicyIncoming)

		r := result[0]
		assert.Zero(r.SPort)
		assert.Equal(r.DPort, p)

		// -- Outgoing
		result = parseK8sPorts([]networking_v1.NetworkPolicyPort{
			port,
		}, pcn_types.PolicyOutgoing)

		r = result[0]
		assert.Zero(r.DPort)
		assert.Equal(r.SPort, p)
	})
}

func TestGetPeersFromIPBlock(t *testing.T) {
	assert := assert.New(t)
	peer := "10.10.0.0/16"
	exceptions := []string{
		"10.10.1.0/24",
		"10.10.2.0/24",
	}

	// --- No exceptions
	t.Run("no-exceptions", func(t *testing.T) {
		block := networking_v1.IPBlock{
			CIDR: peer,
		}
		exc, cidr := getPeersFromIPBlock(&block)

		assert.Nil(exc)
		assert.Len(cidr.IPBlock, 1)
		assert.Equal(peer, cidr.IPBlock[0])
	})

	// --- With exceptions
	t.Run("with-exceptions", func(t *testing.T) {
		block := networking_v1.IPBlock{
			CIDR:   peer,
			Except: exceptions,
		}
		exc, _ := getPeersFromIPBlock(&block)

		assert.NotNil(exc)
		assert.Equal(exceptions, exc.IPBlock)
	})
}

func TestGetPeersFromK8sSelectors(t *testing.T) {
	assert := assert.New(t)
	podSelector := meta_v1.LabelSelector{
		MatchLabels: map[string]string{
			"applies-to": "me",
			"not-to":     "you",
		},
	}
	nsSelector := meta_v1.LabelSelector{
		MatchLabels: map[string]string{
			"applies-to": "her",
			"not-to":     "him",
		},
	}

	// --- Matchexpressions
	t.Run("expressions", func(t *testing.T) {
		podS := meta_v1.LabelSelector{
			MatchExpressions: []meta_v1.LabelSelectorRequirement{},
		}
		nS := meta_v1.LabelSelector{
			MatchExpressions: []meta_v1.LabelSelectorRequirement{},
		}

		peer, err := getPeersFromK8sSelectors(&podS, nil, "test-ns")
		assert.NotNil(err)
		assert.Nil(peer)

		peer, err = getPeersFromK8sSelectors(&nS, nil, "test-ns")
		assert.NotNil(err)
		assert.Nil(peer)
	})

	// --- podselector-only
	t.Run("podselector-only", func(t *testing.T) {
		peer, err := getPeersFromK8sSelectors(&podSelector, nil, "test-ns")
		assert.Nil(err)
		assert.NotNil(peer)
		assert.NotNil(peer.Peer)
		assert.NotNil(peer.Namespace)
		assert.Equal("test-ns", peer.Namespace.Name)
	})

	// --- nsselector-only
	t.Run("nsselector-only", func(t *testing.T) {
		peer, err := getPeersFromK8sSelectors(nil, &nsSelector, "test-ns")
		assert.Nil(err)
		assert.NotNil(peer)
		assert.Nil(peer.Peer)
		assert.NotNil(peer.Namespace)
	})

	// --- both-selectors
	t.Run("both-selectors", func(t *testing.T) {
		peer, err := getPeersFromK8sSelectors(&podSelector, &nsSelector, "test-ns")
		assert.Nil(err)
		assert.NotNil(peer)
		assert.NotNil(peer.Peer)
		assert.NotNil(peer.Namespace)
	})

	// --- podselector-no-ns
	t.Run("podselector-no-ns", func(t *testing.T) {
		peer, err := getPeersFromK8sSelectors(&podSelector, nil, "")
		assert.NotNil(err)
		assert.Nil(peer)
	})
}

func TestParseK8sIngress(t *testing.T) {
	assert := assert.New(t)

	// --- Not an ingress policy
	t.Run("no-ingress", func(t *testing.T) {
		policy := networking_v1.NetworkPolicy{
			Spec: networking_v1.NetworkPolicySpec{
				Egress: []networking_v1.NetworkPolicyEgressRule{},
				PolicyTypes: []networking_v1.PolicyType{
					"Egress",
				},
			},
		}

		result := ParseK8sIngress(&policy)
		assert.Nil(result)
	})

	// --- empty ingress rules
	t.Run("empty-rules", func(t *testing.T) {
		policy := networking_v1.NetworkPolicy{
			ObjectMeta: meta_v1.ObjectMeta{
				Name: "policy-name",
			},
			Spec: networking_v1.NetworkPolicySpec{
				Ingress: []networking_v1.NetworkPolicyIngressRule{},
			},
		}
		result := ParseK8sIngress(&policy)
		assert.Len(result, 1)
		r := result[0]
		assert.Equal(pcn_types.PolicyIncoming, r.Direction)
		assert.Equal("drop-all", r.Action)
		assert.Empty(r.Templates)
	})

	// --- from is nil
	t.Run("from-nil", func(t *testing.T) {
		policy := networking_v1.NetworkPolicy{
			ObjectMeta: meta_v1.ObjectMeta{
				Name: "policy-name",
			},
			Spec: networking_v1.NetworkPolicySpec{
				Ingress: []networking_v1.NetworkPolicyIngressRule{
					networking_v1.NetworkPolicyIngressRule{},
				},
			},
		}
		result := ParseK8sIngress(&policy)
		assert.Len(result, 1)
		r := result[0]
		assert.Equal(pcn_types.PolicyIncoming, r.Direction)
		assert.Equal("forward-all", r.Action)
		assert.NotEmpty(r.Templates)
		assert.Equal(int32(0), r.Priority)
	})

	// --- IPBlock no exceptions
	t.Run("ipblock-no-exc", func(t *testing.T) {
		cidr := "10.10.0.0/16"
		policy := networking_v1.NetworkPolicy{
			ObjectMeta: meta_v1.ObjectMeta{
				Name: "policy-name",
			},
			Spec: networking_v1.NetworkPolicySpec{
				Ingress: []networking_v1.NetworkPolicyIngressRule{
					networking_v1.NetworkPolicyIngressRule{
						From: []networking_v1.NetworkPolicyPeer{
							networking_v1.NetworkPolicyPeer{
								IPBlock: &networking_v1.IPBlock{
									CIDR: cidr,
								},
							},
						},
					},
				},
			},
		}
		result := ParseK8sIngress(&policy)
		assert.Len(result, 1)
		r := result[0]
		assert.NotEmpty(r.Peer.IPBlock)
		assert.Equal(cidr, r.Peer.IPBlock[0])
		assert.Equal(int32(0), r.Priority)
	})

	// --- IPBlock with exceptions
	t.Run("ipblock-exc", func(t *testing.T) {
		cidr := "10.10.0.0/16"
		exceptions := []string{
			"10.10.1.0/24",
			"10.10.5.0/24",
		}
		policy := networking_v1.NetworkPolicy{
			ObjectMeta: meta_v1.ObjectMeta{
				Name: "policy-name",
			},
			Spec: networking_v1.NetworkPolicySpec{
				Ingress: []networking_v1.NetworkPolicyIngressRule{
					networking_v1.NetworkPolicyIngressRule{
						From: []networking_v1.NetworkPolicyPeer{
							networking_v1.NetworkPolicyPeer{
								IPBlock: &networking_v1.IPBlock{
									CIDR:   cidr,
									Except: exceptions,
								},
							},
						},
					},
				},
			},
		}
		result := ParseK8sIngress(&policy)
		assert.Len(result, 2)
		r := result[0]
		assert.Equal(exceptions, r.Peer.IPBlock)
		assert.NotEqual(result[0].Templates, result[1].Templates)
		assert.Equal(int32(0), r.Priority)
		assert.Equal(int32(1), result[1].Priority)
	})

	// Selectors
	t.Run("selectors", func(t *testing.T) {
		policy := networking_v1.NetworkPolicy{
			ObjectMeta: meta_v1.ObjectMeta{
				Name: "policy-name",
			},
			Spec: networking_v1.NetworkPolicySpec{
				Ingress: []networking_v1.NetworkPolicyIngressRule{
					networking_v1.NetworkPolicyIngressRule{
						From: []networking_v1.NetworkPolicyPeer{
							networking_v1.NetworkPolicyPeer{
								PodSelector: &meta_v1.LabelSelector{
									MatchLabels: map[string]string{
										"applies-to": "me",
									},
								},
							},
						},
					},
				},
			},
		}
		result := ParseK8sIngress(&policy)
		assert.Len(result, 1)
		r := result[0]
		assert.Empty(r.Peer.IPBlock)
		assert.Equal(int32(0), r.Priority)
	})
}

func TestParseK8sEgress(t *testing.T) {
	policyTypes := []networking_v1.PolicyType{
		"Egress",
	}
	assert := assert.New(t)

	// --- Not an egress policy
	t.Run("no-egress", func(t *testing.T) {
		policy := networking_v1.NetworkPolicy{
			Spec: networking_v1.NetworkPolicySpec{
				Ingress: []networking_v1.NetworkPolicyIngressRule{},
			},
		}

		result := ParseK8sEgress(&policy)
		assert.Nil(result)
	})

	// --- empty ingress rules
	t.Run("empty-rules", func(t *testing.T) {
		policy := networking_v1.NetworkPolicy{
			ObjectMeta: meta_v1.ObjectMeta{
				Name: "policy-name",
			},
			Spec: networking_v1.NetworkPolicySpec{
				Egress:      []networking_v1.NetworkPolicyEgressRule{},
				PolicyTypes: policyTypes,
			},
		}
		result := ParseK8sEgress(&policy)
		assert.Len(result, 1)
		r := result[0]
		assert.Equal(pcn_types.PolicyOutgoing, r.Direction)
		assert.Equal("drop-all", r.Action)
	})

	// --- to is nil
	t.Run("to-nil", func(t *testing.T) {
		policy := networking_v1.NetworkPolicy{
			ObjectMeta: meta_v1.ObjectMeta{
				Name: "policy-name",
			},
			Spec: networking_v1.NetworkPolicySpec{
				Egress: []networking_v1.NetworkPolicyEgressRule{
					networking_v1.NetworkPolicyEgressRule{},
				},
				PolicyTypes: policyTypes,
			},
		}
		result := ParseK8sEgress(&policy)
		assert.Len(result, 1)
		r := result[0]
		assert.Equal(pcn_types.PolicyOutgoing, r.Direction)
		assert.Equal("forward-all", r.Action)
		assert.NotEmpty(r.Templates)
		assert.Equal(int32(0), r.Priority)
	})

	// --- IPBlock no exceptions
	t.Run("ipblock-no-exc", func(t *testing.T) {
		cidr := "10.10.0.0/16"
		policy := networking_v1.NetworkPolicy{
			ObjectMeta: meta_v1.ObjectMeta{
				Name: "policy-name",
			},
			Spec: networking_v1.NetworkPolicySpec{
				PolicyTypes: policyTypes,
				Egress: []networking_v1.NetworkPolicyEgressRule{
					networking_v1.NetworkPolicyEgressRule{
						To: []networking_v1.NetworkPolicyPeer{
							networking_v1.NetworkPolicyPeer{
								IPBlock: &networking_v1.IPBlock{
									CIDR: cidr,
								},
							},
						},
					},
				},
			},
		}
		result := ParseK8sEgress(&policy)
		assert.Len(result, 1)
		r := result[0]
		assert.NotEmpty(r.Peer.IPBlock)
		assert.Equal(cidr, r.Peer.IPBlock[0])
		assert.Equal(int32(0), r.Priority)
	})

	// --- IPBlock with exceptions
	t.Run("ipblock-exc", func(t *testing.T) {
		cidr := "10.10.0.0/16"
		exceptions := []string{
			"10.10.1.0/24",
			"10.10.5.0/24",
		}
		policy := networking_v1.NetworkPolicy{
			ObjectMeta: meta_v1.ObjectMeta{
				Name: "policy-name",
			},
			Spec: networking_v1.NetworkPolicySpec{
				PolicyTypes: policyTypes,
				Egress: []networking_v1.NetworkPolicyEgressRule{
					networking_v1.NetworkPolicyEgressRule{
						To: []networking_v1.NetworkPolicyPeer{
							networking_v1.NetworkPolicyPeer{
								IPBlock: &networking_v1.IPBlock{
									CIDR:   cidr,
									Except: exceptions,
								},
							},
						},
					},
				},
			},
		}
		result := ParseK8sEgress(&policy)
		assert.Len(result, 2)
		r := result[0]
		assert.Equal(exceptions, r.Peer.IPBlock)
		assert.NotEqual(result[0].Templates, result[1].Templates)
		assert.Equal(int32(0), r.Priority)
		assert.Equal(int32(1), result[1].Priority)
	})

	// Selectors
	t.Run("selectors", func(t *testing.T) {
		policy := networking_v1.NetworkPolicy{
			ObjectMeta: meta_v1.ObjectMeta{
				Name: "policy-name",
			},
			Spec: networking_v1.NetworkPolicySpec{
				PolicyTypes: policyTypes,
				Egress: []networking_v1.NetworkPolicyEgressRule{
					networking_v1.NetworkPolicyEgressRule{
						To: []networking_v1.NetworkPolicyPeer{
							networking_v1.NetworkPolicyPeer{
								PodSelector: &meta_v1.LabelSelector{
									MatchLabels: map[string]string{
										"applies-to": "me",
									},
								},
							},
						},
					},
				},
			},
		}
		result := ParseK8sEgress(&policy)
		assert.Len(result, 1)
		r := result[0]
		assert.Empty(r.Peer.IPBlock)
		assert.Equal(int32(0), r.Priority)
	})
}

func TestParseK8sPolicyTypes(t *testing.T) {
	assert := assert.New(t)

	// -- Nil
	t.Run("nil", func(t *testing.T) {
		in, eg, policyType := ParseK8sPolicyTypes(nil)

		assert.Nil(eg)
		assert.Nil(in)
		assert.Equal("ingress", policyType)
	})

	// -- Ingress only
	t.Run("ingress-only", func(t *testing.T) {
		spec := networking_v1.NetworkPolicySpec{
			Ingress: []networking_v1.NetworkPolicyIngressRule{},
		}

		in, eg, policyType := ParseK8sPolicyTypes(&spec)

		assert.Nil(eg)
		assert.NotNil(in)
		assert.Equal("ingress", policyType)
	})

	// -- Egress only, correctly formed
	t.Run("egress-only-ok", func(t *testing.T) {
		spec := networking_v1.NetworkPolicySpec{
			Egress: []networking_v1.NetworkPolicyEgressRule{},
			PolicyTypes: []networking_v1.PolicyType{
				"Egress",
			},
		}

		in, eg, policyType := ParseK8sPolicyTypes(&spec)

		assert.NotNil(eg)
		assert.Nil(in)
		assert.Equal("egress", policyType)
	})

	// -- Egress only, incorrectly formed
	t.Run("egress-only-ko", func(t *testing.T) {
		spec := networking_v1.NetworkPolicySpec{
			Egress: []networking_v1.NetworkPolicyEgressRule{},
		}

		in, eg, policyType := ParseK8sPolicyTypes(&spec)

		assert.Nil(eg)
		assert.Nil(in)
		assert.Equal("ingress", policyType)
	})

	// -- Both
	t.Run("both", func(t *testing.T) {
		spec := networking_v1.NetworkPolicySpec{
			Egress:  []networking_v1.NetworkPolicyEgressRule{},
			Ingress: []networking_v1.NetworkPolicyIngressRule{},
			PolicyTypes: []networking_v1.PolicyType{
				"Ingress",
				"Egress",
			},
		}

		in, eg, policyType := ParseK8sPolicyTypes(&spec)

		assert.NotNil(eg)
		assert.NotNil(in)
		assert.Empty(policyType)
	})
}
