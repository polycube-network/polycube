package parsers

import (
	"testing"

	"k8s.io/apimachinery/pkg/util/intstr"

	v1beta "github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/pkg/apis/polycube.network/v1beta"
	pcn_types "github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/types"
	"github.com/stretchr/testify/assert"
	core_v1 "k8s.io/api/core/v1"
)

func TestParsePcnPorts(t *testing.T) {
	assert := assert.New(t)

	// -- default
	t.Run("default", func(t *testing.T) {
		p := []v1beta.PolycubeNetworkPolicyProtocolContainer{
			v1beta.PolycubeNetworkPolicyProtocolContainer{
				Ports: v1beta.PolycubeNetworkPolicyPorts{
					Source:      50,
					Destination: 22,
				},
				Protocol: v1beta.TCP,
			},
		}

		result := parsePcnPorts(p)
		assert.Len(result, 1)

		expectedResult := pcn_types.ProtoPort{
			SPort:    50,
			DPort:    22,
			Protocol: "tcp",
		}
		assert.Equal(expectedResult, result[0])
	})

	// -- empty-port
	t.Run("empty-port", func(t *testing.T) {
		p := []v1beta.PolycubeNetworkPolicyProtocolContainer{
			v1beta.PolycubeNetworkPolicyProtocolContainer{
				Ports: v1beta.PolycubeNetworkPolicyPorts{
					Destination: 22,
				},
				Protocol: v1beta.TCP,
			},
		}

		result := parsePcnPorts(p)
		assert.Len(result, 1)

		expectedResult := pcn_types.ProtoPort{
			DPort:    22,
			Protocol: "tcp",
		}
		assert.Equal(expectedResult, result[0])
	})

	// -- empty-protocol
	t.Run("empty-protocol", func(t *testing.T) {
		p := []v1beta.PolycubeNetworkPolicyProtocolContainer{
			v1beta.PolycubeNetworkPolicyProtocolContainer{
				Ports: v1beta.PolycubeNetworkPolicyPorts{
					Destination: 22,
				},
			},
		}

		result := parsePcnPorts(p)
		assert.Len(result, 1)

		expectedResult := pcn_types.ProtoPort{
			DPort: 22,
		}
		assert.Equal(expectedResult, result[0])
	})
}

func TestGetPeersFromPodSelector(t *testing.T) {
	assert := assert.New(t)
	ns := "ns"

	// --any-peer
	t.Run("any-peer", func(t *testing.T) {
		trueVal := true
		p := v1beta.PolycubeNetworkPolicyPeer{
			Any: &trueVal,
		}

		result := getPeersFromPcnPodSelector(p, ns)
		assert.Len(result, 1)
		assert.NotNil(result[0].Namespace)
		assert.Nil(result[0].Peer)
	})

	// --any-namespace
	t.Run("any-namespace", func(t *testing.T) {
		trueVal := true
		p := v1beta.PolycubeNetworkPolicyPeer{
			Any: &trueVal,
			OnNamespace: &v1beta.PolycubeNetworkPolicyNamespaceSelector{
				Any: &trueVal,
			},
		}

		result := getPeersFromPcnPodSelector(p, ns)
		assert.Len(result, 1)
		assert.Nil(result[0].Namespace)
		assert.Nil(result[0].Peer)
	})

	// --multiple-ns
	t.Run("multiple-ns", func(t *testing.T) {
		trueVal := true
		p := v1beta.PolycubeNetworkPolicyPeer{
			Any: &trueVal,
			OnNamespace: &v1beta.PolycubeNetworkPolicyNamespaceSelector{
				WithNames: []string{
					"my-ns",
					"our-ns",
				},
			},
		}

		result := getPeersFromPcnPodSelector(p, ns)
		assert.Len(result, 2)

		for _, r := range result {
			assert.NotNil(r.Namespace)
			if r.Namespace.Name != "my-ns" && r.Namespace.Name != "our-ns" {
				assert.Fail("Namespace failed.", r.Namespace.Name)
			}
		}
	})

	// --with-labels
	t.Run("with-labels", func(t *testing.T) {
		trueVal := true
		labels := map[string]string{
			"applies-to": "me",
		}
		p := v1beta.PolycubeNetworkPolicyPeer{
			Any: &trueVal,
			OnNamespace: &v1beta.PolycubeNetworkPolicyNamespaceSelector{
				WithLabels: labels,
			},
		}

		result := getPeersFromPcnPodSelector(p, ns)
		assert.Len(result, 1)

		assert.NotNil(result[0].Namespace)
		assert.Equal(labels, result[0].Namespace.Labels)
	})
}

func TestParsePcnIngress(t *testing.T) {
	assert := assert.New(t)

	// --drop-all
	t.Run("drop-all", func(t *testing.T) {
		trueVal := true
		p := v1beta.PolycubeNetworkPolicy{
			Spec: v1beta.PolycubeNetworkPolicySpec{
				IngressRules: v1beta.PolycubeNetworkPolicyIngressRuleContainer{
					DropAll: &trueVal,
				},
			},
		}

		result := ParsePcnIngress(&p, nil)
		assert.Len(result, 1)
		assert.Equal("drop-all", result[0].Action)
	})

	// --allow-all
	t.Run("allow-all", func(t *testing.T) {
		trueVal := true
		p := v1beta.PolycubeNetworkPolicy{
			Spec: v1beta.PolycubeNetworkPolicySpec{
				IngressRules: v1beta.PolycubeNetworkPolicyIngressRuleContainer{
					AllowAll: &trueVal,
				},
			},
		}

		result := ParsePcnIngress(&p, nil)
		assert.Len(result, 1)
		assert.Equal("forward-all", result[0].Action)
	})

	// --with-service
	t.Run("with-service", func(t *testing.T) {
		trueVal := true
		p := v1beta.PolycubeNetworkPolicy{
			Spec: v1beta.PolycubeNetworkPolicySpec{
				IngressRules: v1beta.PolycubeNetworkPolicyIngressRuleContainer{
					AllowAll: &trueVal,
				},
			},
		}
		servLabels := map[string]string{
			"applies-to": "me",
		}
		serv := core_v1.Service{
			Spec: core_v1.ServiceSpec{
				Selector: servLabels,
				Ports: []core_v1.ServicePort{
					core_v1.ServicePort{
						TargetPort: intstr.IntOrString{
							IntVal: int32(5050),
						},
					},
				},
			},
		}

		result := ParsePcnIngress(&p, &serv)
		assert.Len(result, 1)
		assert.True(result[0].Subject.IsService)
	})
}
