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

//--------------------------------------
//	Namespaces
//--------------------------------------

//	-- The Labels
var ProductionNsLabels = map[string]string{
	"app":  "myapp",
	"type": "production",
}
var BetaNsLabels = map[string]string{
	"app":  "myapp",
	"type": "beta",
}

//	-- The namespaces
var ProductionNs = core_v1.Namespace{
	ObjectMeta: meta_v1.ObjectMeta{
		Name:   ProductionNamespace,
		Labels: ProductionNsLabels,
	},
}
var BetaNs = core_v1.Namespace{
	ObjectMeta: meta_v1.ObjectMeta{
		Name:   BetaNamespace,
		Labels: BetaNsLabels,
	},
}

//--------------------------------------
//	Pods
//--------------------------------------

//	-- The labels
var LabelsPodsInProduction = map[string]string{
	"app":     "myapp",
	"version": "2.3",
}
var LabelsPodsInBeta = map[string]string{
	"app":     "myapp",
	"version": "3.0.0b",
}

//	-- Pods
var PodsInProduction = []core_v1.Pod{
	core_v1.Pod{
		ObjectMeta: meta_v1.ObjectMeta{
			Namespace: ProductionNamespace,
			UID:       "PRODUCTION-POD-UID-1",
			Labels:    LabelsPodsInProduction,
		},
		Status: core_v1.PodStatus{
			PodIP: "172.10.10.10",
		},
	},
	core_v1.Pod{
		ObjectMeta: meta_v1.ObjectMeta{
			Namespace: ProductionNamespace,
			UID:       "PRODUCTION-POD-UID-2",
			Labels:    LabelsPodsInProduction,
		},
		Status: core_v1.PodStatus{
			PodIP: "172.20.20.20",
		},
	},
}

var PodsInBeta = []core_v1.Pod{
	core_v1.Pod{
		ObjectMeta: meta_v1.ObjectMeta{
			Namespace: ProductionNamespace,
			UID:       "BETA-POD-UID-1",
			Labels:    LabelsPodsInBeta,
		},
		Status: core_v1.PodStatus{
			PodIP: "172.90.90.90",
		},
	},
}

//--------------------------------------
//	Mocked Structures
//--------------------------------------

//	Mock the pod controller
type MockPodController struct {
	mock.Mock
}

func (m *MockPodController) Run()  {}
func (m *MockPodController) Stop() {}
func (m *MockPodController) Subscribe(pcn_types.EventType, pcn_types.ObjectQuery, pcn_types.ObjectQuery, core_v1.PodPhase, func(*core_v1.Pod)) (func(), error) {
	return func() {}, nil
}
func (m *MockPodController) GetPods(pod pcn_types.ObjectQuery, ns pcn_types.ObjectQuery) ([]core_v1.Pod, error) {
	args := m.Called(pod, ns)
	return args.Get(0).([]core_v1.Pod), args.Error(1)
}
