package controllers

import (
	"testing"

	"github.com/stretchr/testify/mock"

	pcn_types "github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/types"
	"github.com/stretchr/testify/assert"
	core_v1 "k8s.io/api/core/v1"
	meta_v1 "k8s.io/apimachinery/pkg/apis/meta/v1"
)

type MockNamespaceController struct {
	mock.Mock
}

func (m *MockNamespaceController) Run()  {}
func (m *MockNamespaceController) Stop() {}
func (m *MockNamespaceController) Subscribe(event pcn_types.EventType, consumer func(*core_v1.Namespace)) (func(), error) {
	return func() {}, nil
}
func (m *MockNamespaceController) GetNamespaces(query pcn_types.PodQueryObject) ([]core_v1.Namespace, error) {
	args := m.Called(query)
	return args.Get(0).([]core_v1.Namespace), args.Error(1)
}

func TestLabelHash(t *testing.T) {
	controller := &PcnPodController{
		pods: map[string]podStore{},
	}

	//	Empty
	l := map[string]string{}
	result := controller.hashLabels(l)
	assert.Empty(t, result)

	l = map[string]string{
		"type": "production",
	}
	result = controller.hashLabels(l)
	assert.NotEmpty(t, result)
}

func TestGetPodsUnrecognizedQuery(t *testing.T) {
	controller := &PcnPodController{
		pods: map[string]podStore{},
	}
	_, err := controller.GetPods(pcn_types.PodQuery{
		Pod: pcn_types.PodQueryObject{
			By: "boom-boom-pow",
		},
	})
	assert.NotNil(t, err)
}

func TestGetPodsByName(t *testing.T) {
	controller := &PcnPodController{
		pods: map[string]podStore{},
	}
	allPods := []pcn_types.Pod{
		pcn_types.Pod{
			Pod: core_v1.Pod{
				ObjectMeta: meta_v1.ObjectMeta{
					Name:      "pod-1-123",
					Namespace: "Production",
				},
			},
		},
		pcn_types.Pod{
			Pod: core_v1.Pod{
				ObjectMeta: meta_v1.ObjectMeta{
					Name:      "pod-2-456",
					Namespace: "Production",
				},
			},
		},
		pcn_types.Pod{
			Pod: core_v1.Pod{
				ObjectMeta: meta_v1.ObjectMeta{
					Name:      "pod-3-789",
					Namespace: "Beta",
				},
			},
		},
		pcn_types.Pod{
			Pod: core_v1.Pod{
				ObjectMeta: meta_v1.ObjectMeta{
					Name:      "pod-4-123",
					Namespace: "Default",
				},
			},
		},
	}
	for _, p := range allPods {
		controller.addNewPod(&p.Pod)
	}

	//	All Pods
	result, err := controller.GetPods(pcn_types.PodQuery{
		Pod: pcn_types.PodQueryObject{
			By:   "name",
			Name: "*",
		},
	})
	assert.Nil(t, err)
	assert.ElementsMatch(t, allPods, result)

	//	Specific pod
	result, err = controller.GetPods(pcn_types.PodQuery{
		Pod: pcn_types.PodQueryObject{
			By:   "name",
			Name: allPods[2].Pod.Name,
		},
	})
	assert.Nil(t, err)
	assert.ElementsMatch(t, []pcn_types.Pod{allPods[2]}, result)

	//	A pod that doesn't exist
	result, err = controller.GetPods(pcn_types.PodQuery{
		Pod: pcn_types.PodQueryObject{
			By:   "name",
			Name: "pod-that-doesnt-exist",
		},
	})
	assert.Nil(t, err)
	assert.Empty(t, result)
}

func TestGetPodsByLabels(t *testing.T) {
	ns := new(MockNamespaceController)
	controller := &PcnPodController{
		pods:         map[string]podStore{},
		nsController: ns,
	}
	productionPodsLabels := map[string]string{
		"ns": "production",
	}
	betaPodsLabels := map[string]string{
		"ns": "beta",
	}
	allPods := []pcn_types.Pod{
		pcn_types.Pod{
			Pod: core_v1.Pod{
				ObjectMeta: meta_v1.ObjectMeta{
					Name:      "pod-1-123",
					Namespace: "Production",
					Labels:    productionPodsLabels,
				},
			},
		},
		pcn_types.Pod{
			Pod: core_v1.Pod{
				ObjectMeta: meta_v1.ObjectMeta{
					Name:      "pod-2-456",
					Namespace: "Production",
					Labels:    productionPodsLabels,
				},
			},
		},
		pcn_types.Pod{
			Pod: core_v1.Pod{
				ObjectMeta: meta_v1.ObjectMeta{
					Name:      "pod-3-789",
					Namespace: "Beta",
					Labels:    betaPodsLabels,
				},
			},
		},
		pcn_types.Pod{
			Pod: core_v1.Pod{
				ObjectMeta: meta_v1.ObjectMeta{
					Name:      "pod-4-123",
					Namespace: "Default",
				},
			},
		},
	}
	for _, p := range allPods {
		controller.addNewPod(&p.Pod)
	}

	productionLabels := map[string]string{
		"type": "Production",
	}
	ns.On("GetNamespaces", pcn_types.PodQueryObject{
		By:     "labels",
		Labels: productionLabels,
	}).Return([]core_v1.Namespace{
		core_v1.Namespace{
			ObjectMeta: meta_v1.ObjectMeta{
				Name: "Production",
			},
		},
	}, nil)
	ns.On("GetNamespaces", pcn_types.PodQueryObject{
		By:     "labels",
		Labels: map[string]string{},
	}).Return([]core_v1.Namespace{
		core_v1.Namespace{
			ObjectMeta: meta_v1.ObjectMeta{
				Name: "Default",
			},
		},
	}, nil)
	ns.On("GetNamespaces", pcn_types.PodQueryObject{
		By: "labels",
		Labels: map[string]string{
			"on": "Staging",
		},
	}).Return([]core_v1.Namespace{}, nil)

	//	Production Pods
	result, err := controller.GetPods(pcn_types.PodQuery{
		Pod: pcn_types.PodQueryObject{
			By:     "labels",
			Labels: productionPodsLabels,
		},
		Namespace: pcn_types.PodQueryObject{
			By:     "labels",
			Labels: productionLabels,
		},
	})
	assert.Nil(t, err)
	assert.ElementsMatch(t, []pcn_types.Pod{allPods[0], allPods[1]}, result)

	//	Namespace doesn't exist
	result, err = controller.GetPods(pcn_types.PodQuery{
		Pod: pcn_types.PodQueryObject{
			By:     "labels",
			Labels: productionPodsLabels,
		},
		Namespace: pcn_types.PodQueryObject{
			By: "labels",
			Labels: map[string]string{
				"on": "Staging",
			},
		},
	})
	assert.Nil(t, err)
	assert.Empty(t, result)

	//	A pod that doesn't exist
	result, err = controller.GetPods(pcn_types.PodQuery{
		Pod: pcn_types.PodQueryObject{
			By: "labels",
			Labels: map[string]string{
				"on": "Staging",
			},
		},
		Namespace: pcn_types.PodQueryObject{
			By:     "labels",
			Labels: productionLabels,
		},
	})
	assert.Nil(t, err)
	assert.Empty(t, result)

	//	No labels in ns
	result, err = controller.GetPods(pcn_types.PodQuery{
		Pod: pcn_types.PodQueryObject{
			By:   "name",
			Name: "*",
		},
		Namespace: pcn_types.PodQueryObject{
			By:     "labels",
			Labels: map[string]string{},
		},
	})
	assert.Nil(t, err)
	assert.ElementsMatch(t, []pcn_types.Pod{allPods[3]}, result)
}
