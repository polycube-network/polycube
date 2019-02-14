package pcnfirewall

import (
	"context"
	"net/http"

	k8sfirewall "github.com/SunSince90/polycube/src/components/k8s/utils/k8sfirewall"
	"github.com/stretchr/testify/mock"
)

const (
	FirewallName = "fw-POD-UUID-1"
)

var MockAPI = new(MockFirewallAPI)

var OkFirewall = DeployedFirewall{
	fwAPI:    MockAPI,
	firewall: &k8sfirewall.Firewall{
		//	Name: the test case you are trying
	},
	rules: map[string]*rulesContainer{},
}

//	Mock the pod controller
type MockFirewallAPI struct {
	mock.Mock
}

func (m *MockFirewallAPI) CreateFirewallByID(ctx context.Context, name string, firewall k8sfirewall.Firewall) (*http.Response, error) {
	args := m.Called(ctx, name, firewall)
	return args.Get(0).(*http.Response), args.Error(1)
}
func (m *MockFirewallAPI) CreateFirewallChainRuleListByID(ctx context.Context, name string, chainName string, rule []k8sfirewall.ChainRule) (*http.Response, error) {
	args := m.Called(ctx, name, chainName, rule)
	return args.Get(0).(*http.Response), args.Error(1)
}
func (m *MockFirewallAPI) DeleteFirewallChainRuleByID(ctx context.Context, name string, chainName string, id int32) (*http.Response, error) {
	args := m.Called(ctx, name, chainName, id)
	return args.Get(0).(*http.Response), args.Error(1)
}
func (m *MockFirewallAPI) ReadFirewallChainByID(ctx context.Context, name string, chainName string) (k8sfirewall.Chain, *http.Response, error) {
	args := m.Called(ctx, name, chainName)
	return args.Get(0).(k8sfirewall.Chain), args.Get(1).(*http.Response), args.Error(2)
}
func (m *MockFirewallAPI) UpdateFirewallInteractiveByID(ctx context.Context, name string, interactive bool) (*http.Response, error) {
	args := m.Called(ctx, name, interactive)
	return args.Get(0).(*http.Response), args.Error(1)
}
func (m *MockFirewallAPI) DeleteFirewallByID(ctx context.Context, name string) (*http.Response, error) {
	args := m.Called(ctx, name)
	return args.Get(0).(*http.Response), args.Error(1)
}

//	NOT USED, SO THEY JUST RETURN EMPTY VALUES
func (m *MockFirewallAPI) CreateFirewallChainAppendByID(ctx context.Context, name string, chainName string, append k8sfirewall.ChainAppendInput) (k8sfirewall.ChainAppendOutput, *http.Response, error) {
	return k8sfirewall.ChainAppendOutput{}, nil, nil
}
func (m *MockFirewallAPI) CreateFirewallChainApplyRulesByID(ctx context.Context, name string, chainName string) (k8sfirewall.ChainApplyRulesOutput, *http.Response, error) {
	return k8sfirewall.ChainApplyRulesOutput{}, nil, nil
}
func (m *MockFirewallAPI) CreateFirewallChainByID(ctx context.Context, name string, chainName string, chain k8sfirewall.Chain) (*http.Response, error) {
	return nil, nil
}
func (m *MockFirewallAPI) CreateFirewallChainListByID(ctx context.Context, name string, chain []k8sfirewall.Chain) (*http.Response, error) {
	return nil, nil
}
func (m *MockFirewallAPI) CreateFirewallChainResetCountersByID(ctx context.Context, name string, chainName string) (k8sfirewall.ChainResetCountersOutput, *http.Response, error) {
	return k8sfirewall.ChainResetCountersOutput{}, nil, nil
}
func (m *MockFirewallAPI) CreateFirewallChainRuleByID(ctx context.Context, name string, chainName string, id int32, rule k8sfirewall.ChainRule) (*http.Response, error) {
	return nil, nil
}
func (m *MockFirewallAPI) CreateFirewallPortsByID(ctx context.Context, name string, portsName string, ports k8sfirewall.Ports) (*http.Response, error) {
	return nil, nil
}
func (m *MockFirewallAPI) CreateFirewallPortsListByID(ctx context.Context, name string, ports []k8sfirewall.Ports) (*http.Response, error) {
	return nil, nil
}
func (m *MockFirewallAPI) DeleteFirewallChainByID(ctx context.Context, name string, chainName string) (*http.Response, error) {
	return nil, nil
}
func (m *MockFirewallAPI) DeleteFirewallChainListByID(ctx context.Context, name string) (*http.Response, error) {
	return nil, nil
}
func (m *MockFirewallAPI) DeleteFirewallChainRuleListByID(ctx context.Context, name string, chainName string) (*http.Response, error) {
	return nil, nil
}
func (m *MockFirewallAPI) DeleteFirewallPortsByID(ctx context.Context, name string, portsName string) (*http.Response, error) {
	return nil, nil
}
func (m *MockFirewallAPI) DeleteFirewallPortsListByID(ctx context.Context, name string) (*http.Response, error) {
	return nil, nil
}
func (m *MockFirewallAPI) ReadFirewallAcceptEstablishedByID(ctx context.Context, name string) (string, *http.Response, error) {
	return "", nil, nil
}
func (m *MockFirewallAPI) ReadFirewallByID(ctx context.Context, name string) (k8sfirewall.Firewall, *http.Response, error) {
	return k8sfirewall.Firewall{}, nil, nil
}
func (m *MockFirewallAPI) ReadFirewallChainDefaultByID(ctx context.Context, name string, chainName string) (string, *http.Response, error) {
	return "", nil, nil
}
func (m *MockFirewallAPI) ReadFirewallChainListByID(ctx context.Context, name string) ([]k8sfirewall.Chain, *http.Response, error) {
	return []k8sfirewall.Chain{}, nil, nil
}
func (m *MockFirewallAPI) ReadFirewallChainRuleActionByID(ctx context.Context, name string, chainName string, id int32) (string, *http.Response, error) {
	return "", nil, nil
}
func (m *MockFirewallAPI) ReadFirewallChainRuleByID(ctx context.Context, name string, chainName string, id int32) (k8sfirewall.ChainRule, *http.Response, error) {
	return k8sfirewall.ChainRule{}, nil, nil
}
func (m *MockFirewallAPI) ReadFirewallChainRuleConntrackByID(ctx context.Context, name string, chainName string, id int32) (string, *http.Response, error) {
	return "", nil, nil
}
func (m *MockFirewallAPI) ReadFirewallChainRuleDescriptionByID(ctx context.Context, name string, chainName string, id int32) (string, *http.Response, error) {
	return "", nil, nil
}
func (m *MockFirewallAPI) ReadFirewallChainRuleDportByID(ctx context.Context, name string, chainName string, id int32) (int32, *http.Response, error) {
	return 0, nil, nil
}
func (m *MockFirewallAPI) ReadFirewallChainRuleDstByID(ctx context.Context, name string, chainName string, id int32) (string, *http.Response, error) {
	return "", nil, nil
}
func (m *MockFirewallAPI) ReadFirewallChainRuleL4protoByID(ctx context.Context, name string, chainName string, id int32) (string, *http.Response, error) {
	return "", nil, nil
}
func (m *MockFirewallAPI) ReadFirewallChainRuleListByID(ctx context.Context, name string, chainName string) ([]k8sfirewall.ChainRule, *http.Response, error) {
	return []k8sfirewall.ChainRule{}, nil, nil
}
func (m *MockFirewallAPI) ReadFirewallChainRuleSportByID(ctx context.Context, name string, chainName string, id int32) (int32, *http.Response, error) {
	return 0, nil, nil
}
func (m *MockFirewallAPI) ReadFirewallChainRuleSrcByID(ctx context.Context, name string, chainName string, id int32) (string, *http.Response, error) {
	return "", nil, nil
}
func (m *MockFirewallAPI) ReadFirewallChainRuleTcpflagsByID(ctx context.Context, name string, chainName string, id int32) (string, *http.Response, error) {
	return "", nil, nil
}
func (m *MockFirewallAPI) ReadFirewallChainStatsActionByID(ctx context.Context, name string, chainName string, id int32) (string, *http.Response, error) {
	return "", nil, nil
}
func (m *MockFirewallAPI) ReadFirewallChainStatsByID(ctx context.Context, name string, chainName string, id int32) (k8sfirewall.ChainStats, *http.Response, error) {
	return k8sfirewall.ChainStats{}, nil, nil
}
func (m *MockFirewallAPI) ReadFirewallChainStatsBytesByID(ctx context.Context, name string, chainName string, id int32) (int32, *http.Response, error) {
	return 0, nil, nil
}
func (m *MockFirewallAPI) ReadFirewallChainStatsConntrackByID(ctx context.Context, name string, chainName string, id int32) (string, *http.Response, error) {
	return "", nil, nil
}
func (m *MockFirewallAPI) ReadFirewallChainStatsDescriptionByID(ctx context.Context, name string, chainName string, id int32) (string, *http.Response, error) {
	return "", nil, nil
}
func (m *MockFirewallAPI) ReadFirewallChainStatsDportByID(ctx context.Context, name string, chainName string, id int32) (int32, *http.Response, error) {
	return 0, nil, nil
}
func (m *MockFirewallAPI) ReadFirewallChainStatsDstByID(ctx context.Context, name string, chainName string, id int32) (string, *http.Response, error) {
	return "", nil, nil
}
func (m *MockFirewallAPI) ReadFirewallChainStatsL4protoByID(ctx context.Context, name string, chainName string, id int32) (string, *http.Response, error) {
	return "", nil, nil
}
func (m *MockFirewallAPI) ReadFirewallChainStatsListByID(ctx context.Context, name string, chainName string) ([]k8sfirewall.ChainStats, *http.Response, error) {
	return []k8sfirewall.ChainStats{}, nil, nil
}
func (m *MockFirewallAPI) ReadFirewallChainStatsPktsByID(ctx context.Context, name string, chainName string, id int32) (int32, *http.Response, error) {
	return 0, nil, nil
}
func (m *MockFirewallAPI) ReadFirewallChainStatsSportByID(ctx context.Context, name string, chainName string, id int32) (int32, *http.Response, error) {
	return 0, nil, nil
}
func (m *MockFirewallAPI) ReadFirewallChainStatsSrcByID(ctx context.Context, name string, chainName string, id int32) (string, *http.Response, error) {
	return "", nil, nil
}
func (m *MockFirewallAPI) ReadFirewallChainStatsTcpflagsByID(ctx context.Context, name string, chainName string, id int32) (string, *http.Response, error) {
	return "", nil, nil
}
func (m *MockFirewallAPI) ReadFirewallConntrackByID(ctx context.Context, name string) (string, *http.Response, error) {
	return "", nil, nil
}
func (m *MockFirewallAPI) ReadFirewallEgressPortByID(ctx context.Context, name string) (string, *http.Response, error) {
	return "", nil, nil
}
func (m *MockFirewallAPI) ReadFirewallIngressPortByID(ctx context.Context, name string) (string, *http.Response, error) {
	return "", nil, nil
}
func (m *MockFirewallAPI) ReadFirewallInteractiveByID(ctx context.Context, name string) (bool, *http.Response, error) {
	return false, nil, nil
}
func (m *MockFirewallAPI) ReadFirewallListByID(ctx context.Context) ([]k8sfirewall.Firewall, *http.Response, error) {
	return []k8sfirewall.Firewall{}, nil, nil
}
func (m *MockFirewallAPI) ReadFirewallLoglevelByID(ctx context.Context, name string) (string, *http.Response, error) {
	return "", nil, nil
}
func (m *MockFirewallAPI) ReadFirewallPortsByID(ctx context.Context, name string, portsName string) (k8sfirewall.Ports, *http.Response, error) {
	return k8sfirewall.Ports{}, nil, nil
}
func (m *MockFirewallAPI) ReadFirewallPortsListByID(ctx context.Context, name string) ([]k8sfirewall.Ports, *http.Response, error) {
	return []k8sfirewall.Ports{}, nil, nil
}
func (m *MockFirewallAPI) ReadFirewallPortsPeerByID(ctx context.Context, name string, portsName string) (string, *http.Response, error) {
	return "", nil, nil
}
func (m *MockFirewallAPI) ReadFirewallPortsStatusByID(ctx context.Context, name string, portsName string) (string, *http.Response, error) {
	return "", nil, nil
}
func (m *MockFirewallAPI) ReadFirewallPortsUuidByID(ctx context.Context, name string, portsName string) (string, *http.Response, error) {
	return "", nil, nil
}
func (m *MockFirewallAPI) ReadFirewallSessionTableByID(ctx context.Context, name string, src string, dst string, l4proto string, sport int32, dport int32) (k8sfirewall.SessionTable, *http.Response, error) {
	return k8sfirewall.SessionTable{}, nil, nil
}
func (m *MockFirewallAPI) ReadFirewallSessionTableEtaByID(ctx context.Context, name string, src string, dst string, l4proto string, sport int32, dport int32) (int32, *http.Response, error) {
	return 0, nil, nil
}
func (m *MockFirewallAPI) ReadFirewallSessionTableListByID(ctx context.Context, name string) ([]k8sfirewall.SessionTable, *http.Response, error) {
	return []k8sfirewall.SessionTable{}, nil, nil
}
func (m *MockFirewallAPI) ReadFirewallSessionTableStateByID(ctx context.Context, name string, src string, dst string, l4proto string, sport int32, dport int32) (string, *http.Response, error) {
	return "", nil, nil
}
func (m *MockFirewallAPI) ReadFirewallTypeByID(ctx context.Context, name string) (string, *http.Response, error) {
	return "", nil, nil
}
func (m *MockFirewallAPI) ReadFirewallUuidByID(ctx context.Context, name string) (string, *http.Response, error) {
	return "", nil, nil
}
func (m *MockFirewallAPI) ReplaceFirewallByID(ctx context.Context, name string, firewall k8sfirewall.Firewall) (*http.Response, error) {
	return nil, nil
}
func (m *MockFirewallAPI) ReplaceFirewallChainByID(ctx context.Context, name string, chainName string, chain k8sfirewall.Chain) (*http.Response, error) {
	return nil, nil
}
func (m *MockFirewallAPI) ReplaceFirewallChainListByID(ctx context.Context, name string, chain []k8sfirewall.Chain) (*http.Response, error) {
	return nil, nil
}
func (m *MockFirewallAPI) ReplaceFirewallChainRuleByID(ctx context.Context, name string, chainName string, id int32, rule k8sfirewall.ChainRule) (*http.Response, error) {
	return nil, nil
}
func (m *MockFirewallAPI) ReplaceFirewallChainRuleListByID(ctx context.Context, name string, chainName string, rule []k8sfirewall.ChainRule) (*http.Response, error) {
	return nil, nil
}
func (m *MockFirewallAPI) ReplaceFirewallPortsByID(ctx context.Context, name string, portsName string, ports k8sfirewall.Ports) (*http.Response, error) {
	return nil, nil
}
func (m *MockFirewallAPI) ReplaceFirewallPortsListByID(ctx context.Context, name string, ports []k8sfirewall.Ports) (*http.Response, error) {
	return nil, nil
}
func (m *MockFirewallAPI) UpdateFirewallAcceptEstablishedByID(ctx context.Context, name string, acceptEstablished string) (*http.Response, error) {
	return nil, nil
}
func (m *MockFirewallAPI) UpdateFirewallByID(ctx context.Context, name string, firewall k8sfirewall.Firewall) (*http.Response, error) {
	return nil, nil
}
func (m *MockFirewallAPI) UpdateFirewallChainByID(ctx context.Context, name string, chainName string, chain k8sfirewall.Chain) (*http.Response, error) {
	return nil, nil
}
func (m *MockFirewallAPI) UpdateFirewallChainDefaultByID(ctx context.Context, name string, chainName string, default_ string) (*http.Response, error) {
	return nil, nil
}
func (m *MockFirewallAPI) UpdateFirewallChainListByID(ctx context.Context, name string, chain []k8sfirewall.Chain) (*http.Response, error) {
	return nil, nil
}
func (m *MockFirewallAPI) UpdateFirewallChainRuleByID(ctx context.Context, name string, chainName string, id int32, rule k8sfirewall.ChainRule) (*http.Response, error) {
	return nil, nil
}
func (m *MockFirewallAPI) UpdateFirewallChainRuleListByID(ctx context.Context, name string, chainName string, rule []k8sfirewall.ChainRule) (*http.Response, error) {
	return nil, nil
}
func (m *MockFirewallAPI) UpdateFirewallConntrackByID(ctx context.Context, name string, conntrack string) (*http.Response, error) {
	return nil, nil
}
func (m *MockFirewallAPI) UpdateFirewallEgressPortByID(ctx context.Context, name string, egressPort string) (*http.Response, error) {
	return nil, nil
}
func (m *MockFirewallAPI) UpdateFirewallIngressPortByID(ctx context.Context, name string, ingressPort string) (*http.Response, error) {
	return nil, nil
}
func (m *MockFirewallAPI) UpdateFirewallListByID(ctx context.Context, firewall []k8sfirewall.Firewall) (*http.Response, error) {
	return nil, nil
}
func (m *MockFirewallAPI) UpdateFirewallLoglevelByID(ctx context.Context, name string, loglevel string) (*http.Response, error) {
	return nil, nil
}
func (m *MockFirewallAPI) UpdateFirewallPortsByID(ctx context.Context, name string, portsName string, ports k8sfirewall.Ports) (*http.Response, error) {
	return nil, nil
}
func (m *MockFirewallAPI) UpdateFirewallPortsListByID(ctx context.Context, name string, ports []k8sfirewall.Ports) (*http.Response, error) {
	return nil, nil
}
func (m *MockFirewallAPI) UpdateFirewallPortsPeerByID(ctx context.Context, name string, portsName string, peer string) (*http.Response, error) {
	return nil, nil
}

/*func (m *MockPodController) Run()  {}
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
}*/
