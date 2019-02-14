package networkpolicies

import (
	pcn_types "github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/types"
	"github.com/stretchr/testify/mock"
	core_v1 "k8s.io/api/core/v1"
	meta_v1 "k8s.io/apimachinery/pkg/apis/meta/v1"
)

const (
	ProductionNamespace   = "Production"
	BetaNamespace         = "Beta"
	StagingNamespace      = "Staging"
	TestDefaultPolicyName = "Test Default Policy"
)

var ProductionPods = []pcn_types.Pod{
	pcn_types.Pod{
		Pod: core_v1.Pod{
			ObjectMeta: meta_v1.ObjectMeta{
				Namespace: ProductionNamespace,
				UID:       "PRODUCTION-POD-UID-1",
			},
			Status: core_v1.PodStatus{
				PodIP: "172.20.20.20",
			},
		},
		Veth: "veth123",
	},
}

var BetaPods = []pcn_types.Pod{
	pcn_types.Pod{
		Pod: core_v1.Pod{
			ObjectMeta: meta_v1.ObjectMeta{
				Namespace: BetaNamespace,
				UID:       "BETA-POD-UID-1",
			},
			Status: core_v1.PodStatus{
				PodIP: "172.40.40.40",
			},
		},
		Veth: "veth512",
	},
}

var AppPodSelector = meta_v1.LabelSelector{
	MatchLabels: map[string]string{
		"type":    "app",
		"version": "2.5",
	},
}

var GlobalQuery1 = pcn_types.Query{
	Pod: []pcn_types.QueryObject{
		pcn_types.QueryObject{
			By:     "labels",
			Labels: AppPodSelector.MatchLabels,
		},
	},
	Namespace: []pcn_types.QueryObject{
		pcn_types.QueryObject{
			By:   "name",
			Name: ProductionNamespace,
		},
	},
}

var GlobalPodsFound1 = []pcn_types.Pod{
	pcn_types.Pod{
		Pod: core_v1.Pod{
			ObjectMeta: meta_v1.ObjectMeta{
				Namespace: ProductionNamespace,
			},
			Status: core_v1.PodStatus{
				PodIP: "172.100.10.10",
			},
		},
		Veth: "veth123",
	},
	pcn_types.Pod{
		Pod: core_v1.Pod{
			ObjectMeta: meta_v1.ObjectMeta{
				Namespace: ProductionNamespace,
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
func (m *MockPodController) Subscribe(event pcn_types.EventType, consumer func(*pcn_types.Pod)) (func(), error) {
	return func() {}, nil
}
func (m *MockPodController) GetPodsByName(name string, ns string) ([]pcn_types.Pod, error) {
	args := m.Called(name, ns)
	return args.Get(0).([]pcn_types.Pod), args.Error(1)
}
func (m *MockPodController) AsyncGetPodsByName(channel chan<- []pcn_types.Pod, name string, ns string) ([]pcn_types.Pod, error) {
	args := m.Called(channel, name, ns)
	return args.Get(0).([]pcn_types.Pod), args.Error(1)
}
func (m *MockPodController) GetPodsByLabels(labels map[string]string, ns string) ([]pcn_types.Pod, error) {
	args := m.Called(labels, ns)
	return args.Get(0).([]pcn_types.Pod), args.Error(1)
}
func (m *MockPodController) AsyncGetPodsByLabels(channel chan<- []pcn_types.Pod, labels map[string]string, ns string) ([]pcn_types.Pod, error) {
	args := m.Called(channel, labels, ns)
	return args.Get(0).([]pcn_types.Pod), args.Error(1)
}
func (m *MockPodController) GetPods(query pcn_types.Query) ([]pcn_types.Pod, error) {
	args := m.Called(query)
	return args.Get(0).([]pcn_types.Pod), args.Error(1)
}
func (m *MockPodController) AsyncGetPods(channel chan<- []pcn_types.Pod, query pcn_types.Query) ([]pcn_types.Pod, error) {
	args := m.Called(query)
	return args.Get(0).([]pcn_types.Pod), args.Error(1)
}
