package networkpolicies

import (
	"testing"

	pcn_fw "github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/networkpolicies/pcn_firewall"

	pcn_types "github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/types"
	"github.com/stretchr/testify/assert"
)

func TestCheckFwManger(t *testing.T) {
	assert := assert.New(t)
	policy := pcn_types.ParsedPolicy{
		Subject: pcn_types.PolicySubject{},
	}

	// --- nil
	t.Run("nil", func(t *testing.T) {
		p := policy
		namespace := "ns"
		p.Subject.Namespace = namespace

		fw := pcn_fw.StartFirewall("name", "ns", map[string]string{
			"applies-to": "me",
		})

		result := checkFwManger(&p, fw)
		assert.True(result)
	})

	// --- different-namespace
	t.Run("different-namespace", func(t *testing.T) {
		p := policy
		p.Subject.Namespace = "not-ns"

		fw := pcn_fw.StartFirewall("name", "ns", map[string]string{
			"applies-to": "me",
		})

		result := checkFwManger(&p, fw)
		assert.False(result)
	})

	// --- not-appropriate-labels
	t.Run("not-appropriate-labels", func(t *testing.T) {
		p := policy
		p.Subject.Namespace = "not-ns"
		p.Subject.Query = &pcn_types.ObjectQuery{
			Labels: map[string]string{
				"applies-to": "you",
			},
		}

		fw := pcn_fw.StartFirewall("name", "ns", map[string]string{
			"applies-to": "me",
		})

		result := checkFwManger(&p, fw)
		assert.False(result)
	})
}
