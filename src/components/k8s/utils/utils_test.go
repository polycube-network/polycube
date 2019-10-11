package utils

import (
	"net"
	"testing"

	pcn_types "github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/types"
	"github.com/stretchr/testify/assert"
)

func TestBuildObjectKey(t *testing.T) {
	peerType := "pod"
	name := "my-pod"
	labels := map[string]string{
		"applies-to": "me",
		"not-to":     "you",
	}
	assert := assert.New(t)

	// --- everything-nil
	t.Run("everything-nil", func(t *testing.T) {
		result := BuildObjectKey(nil, peerType)

		assert.Equal(peerType+"Name:", result)
	})

	// --- name
	t.Run("name", func(t *testing.T) {
		result := BuildObjectKey(&pcn_types.ObjectQuery{
			By:   "name",
			Name: name,
		}, peerType)

		assert.Equal(peerType+"Name:"+name, result)
	})

	// --- labels
	t.Run("labels", func(t *testing.T) {
		result := BuildObjectKey(&pcn_types.ObjectQuery{
			By:     "labels",
			Labels: labels,
		}, peerType)

		l := ImplodeLabels(labels, ",", true)
		assert.Equal(peerType+"Labels:"+l, result)
	})

	// --- names-precedence
	t.Run("names-precedence", func(t *testing.T) {
		result := BuildObjectKey(&pcn_types.ObjectQuery{
			Name:   name,
			Labels: labels,
		}, peerType)

		assert.Equal(peerType+"Name:"+name, result)
	})
}

func TestBuildQuery(t *testing.T) {
	name := "my-pod"
	labels := map[string]string{
		"applies-to": "me",
		"not-to":     "you",
	}
	assert := assert.New(t)

	// --- everything-nil
	t.Run("everything-nil", func(t *testing.T) {
		result := BuildQuery("", nil)

		assert.Nil(result)
	})

	// --- name
	t.Run("name", func(t *testing.T) {
		result := BuildQuery(name, nil)

		assert.Equal("name", result.By)
		assert.Equal(name, result.Name)
	})

	// --- labels
	t.Run("labels", func(t *testing.T) {
		result := BuildQuery("", labels)

		assert.Equal("labels", result.By)
		assert.Equal(labels, result.Labels)
	})

	// --- name-precedence
	t.Run("name-precedence", func(t *testing.T) {
		result := BuildQuery(name, labels)

		assert.Equal("name", result.By)
		assert.Equal(name, result.Name)
	})
}

func TestAreLabelsContained(t *testing.T) {
	assert := assert.New(t)
	haystack := map[string]string{
		"applies-to": "me",
		"not-to":     "you",
	}

	// --- less
	t.Run("less", func(t *testing.T) {
		needle := map[string]string{
			"applies-to": "me",
		}

		result := AreLabelsContained(needle, haystack)
		assert.True(result)
	})

	// --- exact
	t.Run("exact", func(t *testing.T) {
		needle := map[string]string{
			"applies-to": "me",
			"not-to":     "you",
		}

		result := AreLabelsContained(needle, haystack)
		assert.True(result)
	})

	// --- exceeding
	t.Run("exceeding", func(t *testing.T) {
		needle := map[string]string{
			"applies-to": "me",
			"not-to":     "you",
			"exceeding":  "true",
		}

		result := AreLabelsContained(needle, haystack)
		assert.False(result)
	})

	// --- wrong-key
	t.Run("wrong-key", func(t *testing.T) {
		needle := map[string]string{
			"applies-to":  "me",
			"but-also-to": "you",
		}

		result := AreLabelsContained(needle, haystack)
		assert.False(result)
	})

	// --- wrong-vals
	t.Run("wrong-vals", func(t *testing.T) {
		needle := map[string]string{
			"applies-to": "you",
		}

		result := AreLabelsContained(needle, haystack)
		assert.False(result)
	})

	// --- nil-needle
	t.Run("nil-needle", func(t *testing.T) {
		result := AreLabelsContained(nil, nil)
		assert.True(result)
	})

	// --- empty-both
	t.Run("empty-both", func(t *testing.T) {
		result := AreLabelsContained(map[string]string{}, nil)
		assert.True(result)
	})

	// --- empty-needle
	t.Run("empty-needle", func(t *testing.T) {
		result := AreLabelsContained(nil, haystack)
		assert.False(result)
	})

	// --- empty-haystack
	t.Run("empty-haystack", func(t *testing.T) {
		result := AreLabelsContained(map[string]string{
			"just-for": "test",
		}, nil)
		assert.False(result)
	})
}

func TestImplodeLabels(t *testing.T) {
	assert := assert.New(t)
	labels := map[string]string{
		"not-to":     "you",
		"applies-to": "me",
	}

	// --- values
	t.Run("values", func(t *testing.T) {
		result := ImplodeLabels(labels, "!", true)
		assert.Equal("applies-to=me!not-to=you", result)
	})
}

func TestGetPodVirtualIP(t *testing.T) {
	assert := assert.New(t)
	ip := "192.168.133.78"

	// --- 16
	t.Run("16", func(t *testing.T) {
		ipRange := "10.10.0.0/16"
		_, v, _ := net.ParseCIDR(ipRange)

		SetVPodsRange(v)
		result := GetPodVirtualIP(ip)
		assert.Equal("10.10.133.78", result)
	})

	// --- 24
	t.Run("16", func(t *testing.T) {
		ipRange := "10.10.0.0/24"
		_, v, _ := net.ParseCIDR(ipRange)

		SetVPodsRange(v)
		result := GetPodVirtualIP(ip)
		assert.Equal("10.10.0.78", result)
	})

	// --- 23
	t.Run("16", func(t *testing.T) {
		ipRange := "10.10.0.0/23"
		_, v, _ := net.ParseCIDR(ipRange)

		SetVPodsRange(v)
		result := GetPodVirtualIP(ip)
		assert.Equal("10.10.1.78", result)
	})
}
