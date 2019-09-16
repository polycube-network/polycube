package parsers

import (
	"testing"
	"time"

	pcn_types "github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/types"
	k8sfirewall "github.com/polycube-network/polycube/src/components/k8s/utils/k8sfirewall"
	"github.com/stretchr/testify/assert"
	networking_v1 "k8s.io/api/networking/v1"
	meta_v1 "k8s.io/apimachinery/pkg/apis/meta/v1"
)

func TestReformatName(t *testing.T) {
	// -- Test
	count := 5
	n := 2
	direction := pcn_types.PolicyIncoming
	name := "allow-api"

	result := reformatPolicyName(name, direction, n, count)
	assert.Equal(t, result, "allow-api#2ip5")
}

func TestSwapPortsDirection(t *testing.T) {
	assert := assert.New(t)

	// --- Both ports
	t.Run("both-ports", func(t *testing.T) {
		ports := pcn_types.ProtoPort{
			Protocol: "TCP",
			SPort:    int32(8080),
			DPort:    int32(5566),
		}
		result := swapPortsDirection(ports)

		assert.Equal(result.Protocol, ports.Protocol)
		assert.Equal(result.DPort, ports.SPort)
		assert.Equal(result.SPort, ports.DPort)
	})
}

func TestInsertPorts(t *testing.T) {
	assert := assert.New(t)
	rules := []k8sfirewall.ChainRule{
		k8sfirewall.ChainRule{
			Conntrack: "new",
			Action:    "forward",
		},
	}

	// --- No ports
	t.Run("no-ports", func(t *testing.T) {
		ports := []pcn_types.ProtoPort{}
		result := insertPorts(rules, ports)

		assert.Len(result, len(rules))
		assert.Equal(result, rules)
	})

	// --- Ports
	t.Run("with-ports", func(t *testing.T) {
		TCP := pcn_types.ProtoPort{
			Protocol: "TCP",
			DPort:    int32(8080),
			SPort:    int32(2789),
		}
		UDP := pcn_types.ProtoPort{
			Protocol: "UDP",
			SPort:    int32(5050),
		}
		ports := []pcn_types.ProtoPort{
			TCP,
			UDP,
		}
		result := insertPorts(rules, ports)

		assert.Len(result, len(ports))

		for _, rule := range result {
			switch rule.L4proto {
			case "UDP":
				if rule.Sport != UDP.SPort {
					assert.Fail("UDP sport is not correct")
				}
				if rule.Dport != UDP.DPort {
					assert.Fail("UDP dport is not correct")
				}
			case "TCP":
				if rule.Sport != TCP.SPort {
					assert.Fail("TCP sport is not correct")
				}
				if rule.Dport != TCP.DPort {
					assert.Fail("TCP dport is not correct")
				}
			default:
				assert.Fail("l4 protocol unrecognized")
			}
		}
	})

	// --- Only Ports
	t.Run("only-ports", func(t *testing.T) {
		double := pcn_types.ProtoPort{
			DPort: int32(8080),
			SPort: int32(2789),
		}
		one := pcn_types.ProtoPort{
			SPort: int32(5050),
		}
		ports := []pcn_types.ProtoPort{
			double,
			one,
		}
		result := insertPorts(rules, ports)

		for _, rule := range result {
			assert.Empty(rule.L4proto)
		}
	})
}

func TestBuildRuleTemplates(t *testing.T) {
	assert := assert.New(t)
	ports := []pcn_types.ProtoPort{
		pcn_types.ProtoPort{
			Protocol: "TCP",
			SPort:    int32(8080),
			DPort:    int32(5566),
		},
	}

	// --- Directions
	t.Run("actions", func(t *testing.T) {
		ing := buildRuleTemplates(pcn_types.PolicyIncoming, pcn_types.ActionForward, ports)
		assert.NotEmpty(ing.Incoming)
		assert.Empty(ing.Outgoing)

		eg := buildRuleTemplates(pcn_types.PolicyOutgoing, pcn_types.ActionForward, ports)
		assert.NotEmpty(eg.Outgoing)
		assert.Empty(eg.Incoming)
	})

	// --- Fields
	t.Run("actions", func(t *testing.T) {
		ing := buildRuleTemplates(pcn_types.PolicyIncoming, pcn_types.ActionForward, ports)

		for _, in := range ing.Incoming {
			assert.Equal(in.Action, pcn_types.ActionForward)
			assert.Equal(in.Conntrack, pcn_types.ConnTrackNew)
		}
	})
}

func TestGetBasePolicyFromK8s(t *testing.T) {
	assert := assert.New(t)
	policyName := "policy-name"
	policyNs := "policy-ns"
	now := time.Now()
	policy := networking_v1.NetworkPolicy{
		ObjectMeta: meta_v1.ObjectMeta{
			Name:      policyName,
			Namespace: policyNs,
			CreationTimestamp: meta_v1.Time{
				Time: now,
			},
		},
	}
	parent := pcn_types.ParentPolicy{
		Name:     policyName,
		Priority: 5,
		Provider: pcn_types.K8sProvider,
	}

	// --- Base Parst
	t.Run("base-parts", func(t *testing.T) {
		result := getBasePolicyFromK8s(&policy)

		assert.Equal(parent, result.ParentPolicy)
		assert.Equal(result.Subject.Namespace, policyNs)
		assert.False(result.Subject.IsService)
		assert.Equal(int32(5), result.ParentPolicy.Priority)
		assert.Equal(meta_v1.Time{
			Time: now,
		}, result.CreationTime)
	})

	// --- Applies to any
	t.Run("any", func(t *testing.T) {
		result := getBasePolicyFromK8s(&policy)

		assert.Nil(result.Subject.Query)
	})

	// --- Applies to specific
	t.Run("any", func(t *testing.T) {
		p := policy
		labels := map[string]string{
			"applies-to": "me",
			"not-to":     "you",
		}
		p.Spec.PodSelector = meta_v1.LabelSelector{
			MatchLabels: labels,
		}
		result := getBasePolicyFromK8s(&p)

		assert.NotNil(result.Subject.Query)
		assert.Equal(labels, result.Subject.Query.Labels)
	})
}

func TestFillTemplates(t *testing.T) {
	assert := assert.New(t)
	rules := []k8sfirewall.ChainRule{
		k8sfirewall.ChainRule{
			Conntrack: "new",
			L4proto:   "tcp",
			Sport:     int32(8080),
			Dport:     int32(9000),
		},
		k8sfirewall.ChainRule{
			Conntrack: "new",
			L4proto:   "udp",
			Sport:     int32(5750),
		},
	}
	src := "10.10.10.10"
	dst := "11.11.11.11"
	result := fillTemplates(src, dst, rules)

	assert.Len(result, len(rules))
	for _, rule := range result {
		assert.Equal(src, rule.Src)
		assert.Equal(dst, rule.Dst)
	}
}
