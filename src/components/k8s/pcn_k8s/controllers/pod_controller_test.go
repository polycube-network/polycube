package controllers

import (
	"testing"
	"time"

	meta_v1 "k8s.io/apimachinery/pkg/apis/meta/v1"

	"github.com/stretchr/testify/assert"

	"github.com/stretchr/testify/mock"

	pcn_types "github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/types"
	core_v1 "k8s.io/api/core/v1"
)

type MockNamespaceController struct {
	mock.Mock
}

func (m *MockNamespaceController) Run()  {}
func (m *MockNamespaceController) Stop() {}
func (m *MockNamespaceController) Subscribe(event pcn_types.EventType, consumer func(*core_v1.Namespace)) (func(), error) {
	return func() {}, nil
}
func (m *MockNamespaceController) GetNamespaces(query pcn_types.ObjectQuery) ([]core_v1.Namespace, error) {
	args := m.Called(query)
	return args.Get(0).([]core_v1.Namespace), args.Error(1)
}

func TestGetPodsUnrecognizedQuery(t *testing.T) {
	/*controller := &PcnPodController{
		pods: map[string]podStore{},
	}
	_, err := controller.GetPods(pcn_types.PodQuery{
		Pod: pcn_types.PodQueryObject{
			By: "boom-boom-pow",
		},
	})
	assert.NotNil(t, err)*/
}

func TestGetPodsByName(t *testing.T) {
	/*controller := &PcnPodController{
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
	assert.Empty(t, result)*/
}

func TestGetPodsByLabels(t *testing.T) {
	/*ns := new(MockNamespaceController)
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
	assert.ElementsMatch(t, []pcn_types.Pod{allPods[3]}, result)*/
}

func TestPodMeetsCriteria(t *testing.T) {
	nsController := new(MockNamespaceController)

	//	namespace 1
	nsOk := map[string]string{
		"should": "succed",
	}
	productionNs := "production"
	nsController.On("GetNamespaces", pcn_types.ObjectQuery{
		Labels: nsOk,
	}).Return([]core_v1.Namespace{
		core_v1.Namespace{
			ObjectMeta: meta_v1.ObjectMeta{
				Name: productionNs,
			},
		},
	}, nil)

	//	Namespace 2
	nsKo := map[string]string{
		"should": "fail",
	}
	betaNs := "beta"
	nsController.On("GetNamespaces", pcn_types.ObjectQuery{
		Labels: nsKo,
	}).Return([]core_v1.Namespace{
		core_v1.Namespace{
			ObjectMeta: meta_v1.ObjectMeta{
				Name: betaNs,
			},
		},
	}, nil)
	controller := &PcnPodController{
		pods:         map[string]*pcn_types.Pod{},
		nsController: nsController,
	}
	testPod := core_v1.Pod{
		Status: core_v1.PodStatus{
			Phase: core_v1.PodRunning,
		},
		ObjectMeta: meta_v1.ObjectMeta{
			DeletionTimestamp: &meta_v1.Time{
				time.Now(),
			},
		},
	}

	//	Case 1: pod is nil
	result := controller.podMeetsCriteria(nil, pcn_types.ObjectQuery{}, pcn_types.ObjectQuery{}, pcn_types.PodAnyPhase)
	assert.False(t, result)

	//	Case 2: Any Phase
	phase := pcn_types.PodAnyPhase
	result = controller.podMeetsCriteria(&testPod, pcn_types.ObjectQuery{}, pcn_types.ObjectQuery{}, phase)
	assert.True(t, result)

	//	Case 3: Pod is terminating & I want terminating pods
	phase = pcn_types.PodTerminating
	result = controller.podMeetsCriteria(&testPod, pcn_types.ObjectQuery{}, pcn_types.ObjectQuery{}, phase)
	assert.True(t, result)

	//	Case 4: Pod is terminating & I don't want terminating pods
	phase = pcn_types.PodRunning
	result = controller.podMeetsCriteria(&testPod, pcn_types.ObjectQuery{}, pcn_types.ObjectQuery{}, phase)
	assert.False(t, result)

	//	Case 4: Pod is not terminating & I want Terminating pods
	phase = pcn_types.PodTerminating
	testPod.ObjectMeta.DeletionTimestamp = nil
	result = controller.podMeetsCriteria(&testPod, pcn_types.ObjectQuery{}, pcn_types.ObjectQuery{}, phase)
	assert.False(t, result)

	//	Case 5: Pod is not in the phase I want
	phase = core_v1.PodRunning
	testPod.Status.Phase = core_v1.PodFailed
	result = controller.podMeetsCriteria(&testPod, pcn_types.ObjectQuery{}, pcn_types.ObjectQuery{}, phase)
	assert.False(t, result)

	//	Case 5a (name): Pod is not in the namespace I want
	testPod.Status.Phase = core_v1.PodRunning
	testPod.Namespace = productionNs
	ns := pcn_types.ObjectQuery{
		Name: betaNs,
	}
	result = controller.podMeetsCriteria(&testPod, pcn_types.ObjectQuery{}, ns, phase)
	assert.False(t, result)

	//	Case 5b (labels): Pod is not in the namespace I want
	testPod.Status.Phase = core_v1.PodRunning
	ns = pcn_types.ObjectQuery{
		Labels: nsKo,
	}
	result = controller.podMeetsCriteria(&testPod, pcn_types.ObjectQuery{}, ns, phase)
	assert.False(t, result)

	//	Case 6a (name): Pod is in the namespace I want
	ns = pcn_types.ObjectQuery{
		Name: productionNs,
	}
	result = controller.podMeetsCriteria(&testPod, pcn_types.ObjectQuery{}, ns, phase)
	assert.True(t, result)

	//	Case 6 (labels): Pod is in the namespace I want
	ns = pcn_types.ObjectQuery{
		Labels: nsOk,
	}
	result = controller.podMeetsCriteria(&testPod, pcn_types.ObjectQuery{}, ns, phase)
	assert.True(t, result)

	//	Case 7: Pod doesn't have labels I want
	podLabels := map[string]string{
		"pod": "ok",
	}
	testPod.Labels = podLabels

	podQuery := pcn_types.ObjectQuery{
		Labels: map[string]string{
			"pod": "ko",
		},
	}
	result = controller.podMeetsCriteria(&testPod, podQuery, ns, phase)
	assert.False(t, result)

	//	Case 8: Pod has labels I want
	podQuery.Labels = podLabels
	result = controller.podMeetsCriteria(&testPod, podQuery, ns, phase)
	assert.True(t, result)
}
