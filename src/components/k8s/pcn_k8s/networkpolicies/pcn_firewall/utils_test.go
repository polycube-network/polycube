package pcnfirewall

import (
	"testing"
	"time"

	pcn_types "github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/types"
	"github.com/stretchr/testify/assert"
	meta_v1 "k8s.io/apimachinery/pkg/apis/meta/v1"
)

func TestCalculatePolicyOffset(t *testing.T) {
	assert := assert.New(t)

	// --- first-place
	t.Run("first-place", func(t *testing.T) {
		base := []policyPriority{
			policyPriority{
				parentPriority: 5,
			},
			policyPriority{
				parentPriority: 6,
			},
		}

		first := pcn_types.ParsedPolicy{
			ParentPolicy: pcn_types.ParentPolicy{
				Priority: 3,
			},
		}

		offset := calculatePolicyOffset(first, base)
		assert.Zero(offset)
	})

	// --- middle-place
	t.Run("middle-place", func(t *testing.T) {
		base := []policyPriority{
			policyPriority{
				parentPriority: 5,
			},
			policyPriority{
				parentPriority: 7,
			},
		}

		p := pcn_types.ParsedPolicy{
			ParentPolicy: pcn_types.ParentPolicy{
				Priority: 6,
			},
		}

		offset := calculatePolicyOffset(p, base)
		assert.Equal(1, offset)
	})

	// --- last-place
	t.Run("last-place", func(t *testing.T) {
		base := []policyPriority{
			policyPriority{
				parentPriority: 5,
			},
			policyPriority{
				parentPriority: 7,
			},
		}

		p := pcn_types.ParsedPolicy{
			ParentPolicy: pcn_types.ParentPolicy{
				Priority: 9,
			},
		}

		offset := calculatePolicyOffset(p, base)
		assert.Equal(2, offset)
	})

	// --- same-priority
	t.Run("same-priority", func(t *testing.T) {
		first := "2019-10-10T12:00:00.0Z"
		second := "2019-10-10T13:00:00.0Z"
		ft, _ := time.Parse(time.RFC3339, first)
		st, _ := time.Parse(time.RFC3339, second)
		base := []policyPriority{
			policyPriority{
				parentPriority: 5,
			},
			policyPriority{
				parentPriority: 7,
				timestamp:      ft,
			},
		}

		p := pcn_types.ParsedPolicy{
			ParentPolicy: pcn_types.ParentPolicy{
				Priority: 7,
			},
			CreationTime: meta_v1.Time{
				Time: st,
			},
		}

		offset := calculatePolicyOffset(p, base)
		assert.Equal(1, offset)
	})

	// --- same-date
	t.Run("same-date", func(t *testing.T) {
		first := "2019-10-10T12:00:00.0Z"
		ft, _ := time.Parse(time.RFC3339, first)
		base := []policyPriority{
			policyPriority{
				parentPriority: 5,
			},
			policyPriority{
				priority:       6,
				parentPriority: 7,
				timestamp:      ft,
			},
		}

		p := pcn_types.ParsedPolicy{
			Priority: 5,
			ParentPolicy: pcn_types.ParentPolicy{
				Priority: 7,
			},
			CreationTime: meta_v1.Time{
				Time: ft,
			},
		}

		offset := calculatePolicyOffset(p, base)
		assert.Equal(1, offset)
	})

}
