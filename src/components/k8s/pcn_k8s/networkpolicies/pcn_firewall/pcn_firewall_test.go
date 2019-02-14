package pcnfirewall

import (
	"errors"
	"net/http"
	"testing"

	"github.com/stretchr/testify/mock"

	"github.com/stretchr/testify/assert"

	k8sfirewall "github.com/SunSince90/polycube/src/components/k8s/utils/k8sfirewall"
)

func TestNewFirewall(t *testing.T) {
	api := new(MockFirewallAPI)
	//	Creation failed
	nameFailCase := "to-be-failed"
	api.On("CreateFirewallByID", nil, nameFailCase, k8sfirewall.Firewall{
		Name:     nameFailCase,
		Type_:    "TC",
		Loglevel: "DEBUG",
	}).Return(&http.Response{}, errors.New("fake-error"))
	//	Interactive failed
	interactiveFail := "to-be-failed-interactive"
	api.On("CreateFirewallByID", nil, interactiveFail, k8sfirewall.Firewall{
		Name:     interactiveFail,
		Type_:    "TC",
		Loglevel: "DEBUG",
	}).Return(&http.Response{}, nil)
	api.On("UpdateFirewallInteractiveByID", nil, interactiveFail, false).Return(&http.Response{}, errors.New("fake-error"))
	//	Success
	//	Interactive failed

	api.On("CreateFirewallByID", nil, FirewallName, k8sfirewall.Firewall{
		Name:     FirewallName,
		Type_:    "TC",
		Loglevel: "DEBUG",
	}).Return(&http.Response{}, nil)
	api.On("UpdateFirewallInteractiveByID", nil, FirewallName, false).Return(&http.Response{}, nil)

	fw := newFirewall(nameFailCase, api)
	assert.Nil(t, fw)

	fw = newFirewall(interactiveFail, api)
	assert.NotNil(t, fw)

	fw = newFirewall(FirewallName, api)
	assert.NotNil(t, fw)
}

func TestInjectRules(t *testing.T) {
	rules := []k8sfirewall.ChainRule{
		k8sfirewall.ChainRule{
			//	... not relevant
		},
		k8sfirewall.ChainRule{
			//	... not relevant
		},
		k8sfirewall.ChainRule{
			//	... not relevant
		},
	}
	errChain := "error-chain"
	OkFirewall.firewall.Name = errChain
	MockAPI.On("ReadFirewallChainByID", nil, errChain, "ingress").Return(k8sfirewall.Chain{}, &http.Response{}, errors.New("fake-error"))
	result, err := OkFirewall.injectRules("ingress", rules)
	assert.NotNil(t, err)
	assert.Empty(t, result)

	//	Empty Chain
	empty := "empty"
	emptyChain := k8sfirewall.Chain{
		//	... not relevant
		Rule: []k8sfirewall.ChainRule{},
	}
	MockAPI.On("ReadFirewallChainByID", nil, empty, "ingress").Return(emptyChain, &http.Response{}, nil)
	MockAPI.On("CreateFirewallChainRuleListByID", nil, empty, "ingress", rules).Return(&http.Response{}, nil)
	OkFirewall.firewall.Name = empty

	result, err = OkFirewall.injectRules("ingress", rules)
	assert.Nil(t, err)
	assert.Len(t, result, 3)
	for i := 0; i < len(result); i++ {
		assert.Equal(t, 1+int32(i), result[i].Id)
	}

	//	Not Empty Chain
	lastID := int32(102)
	notEmpty := "not-empty"
	notEmptyChain := k8sfirewall.Chain{
		//	...
		Rule: []k8sfirewall.ChainRule{
			k8sfirewall.ChainRule{
				Id: 3,
				//	... not relevant
			},
			k8sfirewall.ChainRule{
				Id: 5,
				//	... not relevant
			},
			k8sfirewall.ChainRule{
				Id: 7,
				//	... not relevant
			},
			k8sfirewall.ChainRule{
				Id: 94,
				//	... not relevant
			},
			k8sfirewall.ChainRule{
				Id: lastID,
				//	... not relevant
			},
		},
	}
	MockAPI.On("ReadFirewallChainByID", nil, notEmpty, "ingress").Return(notEmptyChain, &http.Response{}, nil)
	MockAPI.On("CreateFirewallChainRuleListByID", nil, notEmpty, "ingress", rules).Return(&http.Response{}, nil)
	OkFirewall.firewall.Name = notEmpty

	result, err = OkFirewall.injectRules("ingress", rules)
	assert.Nil(t, err)
	assert.Len(t, result, 3)
	for i := 0; i < len(result); i++ {
		assert.Equal(t, lastID+int32(i)+1, result[i].Id)
	}

	//	Error in injecting rules
	injectError := "inject-error"
	MockAPI.On("ReadFirewallChainByID", nil, injectError, "ingress").Return(notEmptyChain, &http.Response{}, nil)
	MockAPI.On("CreateFirewallChainRuleListByID", nil, injectError, "ingress", rules).Return(&http.Response{}, errors.New("fake-error"))
	OkFirewall.firewall.Name = injectError

	result, err = OkFirewall.injectRules("ingress", rules)
	assert.NotNil(t, err)
	assert.Empty(t, result)
}

func TestRemoveRules(t *testing.T) {
	//	Successful case
	success := "success"
	OkFirewall.firewall.Name = success
	rules := []k8sfirewall.ChainRule{
		k8sfirewall.ChainRule{
			Id: 1,
		},
		k8sfirewall.ChainRule{
			Id: 2,
		},
		k8sfirewall.ChainRule{
			Id: 3,
		},
	}
	MockAPI.On("DeleteFirewallChainRuleByID", nil, OkFirewall.firewall.Name, "ingress", mock.AnythingOfType("int32")).Return(&http.Response{}, nil)

	result := OkFirewall.RemoveRules("ingress", rules)
	assert.Empty(t, result)

	//	Rule 3 failed
	failed := "failed"
	OkFirewall.firewall.Name = failed
	MockAPI.On("DeleteFirewallChainRuleByID", nil, OkFirewall.firewall.Name, "ingress", int32(1)).Return(&http.Response{}, nil)
	MockAPI.On("DeleteFirewallChainRuleByID", nil, OkFirewall.firewall.Name, "ingress", int32(2)).Return(&http.Response{}, nil)
	MockAPI.On("DeleteFirewallChainRuleByID", nil, OkFirewall.firewall.Name, "ingress", int32(3)).Return(&http.Response{}, errors.New("fake-error"))

	result = OkFirewall.RemoveRules("ingress", rules)
	assert.Len(t, result, 1)

	//	Everything failed
	efailed := "everything-failed"
	OkFirewall.firewall.Name = efailed
	MockAPI.On("DeleteFirewallChainRuleByID", nil, OkFirewall.firewall.Name, "ingress", mock.AnythingOfType("int32")).Return(&http.Response{}, errors.New("fake-error"))

	result = OkFirewall.RemoveRules("ingress", rules)
	assert.Len(t, result, len(rules))
}

func TestEnforcePolicy(t *testing.T) {
	lastIngressID := int32(5)
	lastEgressID := int32(9)
	iChain := k8sfirewall.Chain{
		Rule: []k8sfirewall.ChainRule{
			k8sfirewall.ChainRule{
				Id: lastIngressID,
			},
		},
	}
	eChain := k8sfirewall.Chain{
		Rule: []k8sfirewall.ChainRule{
			k8sfirewall.ChainRule{
				Id: lastEgressID,
			},
		},
	}
	irules := []k8sfirewall.ChainRule{
		k8sfirewall.ChainRule{},
		k8sfirewall.ChainRule{},
		k8sfirewall.ChainRule{},
	}
	erules := []k8sfirewall.ChainRule{
		k8sfirewall.ChainRule{},
		k8sfirewall.ChainRule{},
		k8sfirewall.ChainRule{},
		k8sfirewall.ChainRule{},
	}

	//	All Successful case
	allSuccess := "all"
	allSuccessPolicy := allSuccess + "-Policy"
	OkFirewall.firewall.Name = allSuccess
	MockAPI.On("ReadFirewallChainByID", nil, OkFirewall.firewall.Name, "ingress").Return(iChain, &http.Response{}, nil)
	MockAPI.On("ReadFirewallChainByID", nil, OkFirewall.firewall.Name, "egress").Return(eChain, &http.Response{}, nil)
	MockAPI.On("CreateFirewallChainRuleListByID", nil, OkFirewall.firewall.Name, "ingress", irules).Return(&http.Response{}, nil)
	MockAPI.On("CreateFirewallChainRuleListByID", nil, OkFirewall.firewall.Name, "egress", erules).Return(&http.Response{}, nil)

	ierr, eerr := OkFirewall.EnforcePolicy(allSuccessPolicy, irules, erules)
	assert.Nil(t, ierr)
	assert.Nil(t, eerr)
	assert.True(t, OkFirewall.ImplementsPolicy(allSuccessPolicy))
	assert.Len(t, OkFirewall.rules[allSuccessPolicy].ingress, len(irules))
	assert.Len(t, OkFirewall.rules[allSuccessPolicy].egress, len(erules))

	//	Error in egress
	egressError := "egress-error"
	egressErrorPolicy := egressError + "-Policy"
	OkFirewall.firewall.Name = egressError
	MockAPI.On("ReadFirewallChainByID", nil, OkFirewall.firewall.Name, "ingress").Return(iChain, &http.Response{}, nil)
	MockAPI.On("ReadFirewallChainByID", nil, OkFirewall.firewall.Name, "egress").Return(eChain, &http.Response{}, nil)
	MockAPI.On("CreateFirewallChainRuleListByID", nil, OkFirewall.firewall.Name, "ingress", irules).Return(&http.Response{}, nil)
	MockAPI.On("CreateFirewallChainRuleListByID", nil, OkFirewall.firewall.Name, "egress", erules).Return(&http.Response{}, errors.New("fake-error"))

	ierr, eerr = OkFirewall.EnforcePolicy(egressErrorPolicy, irules, erules)
	assert.Nil(t, ierr)
	assert.NotNil(t, eerr)
	assert.True(t, OkFirewall.ImplementsPolicy(egressErrorPolicy))
	assert.Len(t, OkFirewall.rules[egressErrorPolicy].ingress, len(irules))
	assert.Len(t, OkFirewall.rules[egressErrorPolicy].egress, 0)

	//	No ingress
	noIngress := "no-ingress"
	noIngressPolicy := noIngress + "-Policy"
	OkFirewall.firewall.Name = noIngress
	MockAPI.On("ReadFirewallChainByID", nil, OkFirewall.firewall.Name, "egress").Return(eChain, &http.Response{}, nil)
	MockAPI.On("CreateFirewallChainRuleListByID", nil, OkFirewall.firewall.Name, "egress", erules).Return(&http.Response{}, nil)

	ierr, eerr = OkFirewall.EnforcePolicy(noIngressPolicy, []k8sfirewall.ChainRule{}, erules)
	assert.Nil(t, ierr)
	assert.Nil(t, eerr)
	assert.True(t, OkFirewall.ImplementsPolicy(noIngressPolicy))
	assert.Len(t, OkFirewall.rules[noIngressPolicy].egress, len(erules))
	assert.Len(t, OkFirewall.rules[noIngressPolicy].ingress, 0)

	//	Adds to something already existing
	existings := "no-ingress"
	existingsPolicy := existings + "-Policy"
	OkFirewall.firewall.Name = existings
	OkFirewall.rules[existingsPolicy] = &rulesContainer{
		ingress: irules,
		egress:  erules,
	}
	newIrules := []k8sfirewall.ChainRule{
		k8sfirewall.ChainRule{},
		k8sfirewall.ChainRule{},
		k8sfirewall.ChainRule{},
	}
	newErules := []k8sfirewall.ChainRule{
		k8sfirewall.ChainRule{},
		k8sfirewall.ChainRule{},
		k8sfirewall.ChainRule{},
		k8sfirewall.ChainRule{},
		k8sfirewall.ChainRule{},
	}
	MockAPI.On("ReadFirewallChainByID", nil, OkFirewall.firewall.Name, "ingress").Return(iChain, &http.Response{}, nil)
	MockAPI.On("ReadFirewallChainByID", nil, OkFirewall.firewall.Name, "egress").Return(eChain, &http.Response{}, nil)
	MockAPI.On("CreateFirewallChainRuleListByID", nil, OkFirewall.firewall.Name, "ingress", newIrules).Return(&http.Response{}, nil)
	MockAPI.On("CreateFirewallChainRuleListByID", nil, OkFirewall.firewall.Name, "egress", newErules).Return(&http.Response{}, nil)

	ierr, eerr = OkFirewall.EnforcePolicy(existingsPolicy, newIrules, newErules)
	assert.Nil(t, ierr)
	assert.Nil(t, eerr)
	assert.True(t, OkFirewall.ImplementsPolicy(existingsPolicy))
	assert.Len(t, OkFirewall.rules[existingsPolicy].egress, len(erules)+len(newErules))
	assert.Len(t, OkFirewall.rules[existingsPolicy].ingress, len(irules)+len(newIrules))
}

func TestCeasePolicy(t *testing.T) {
	//	Successful case
	allSuccess := "all-success"
	allSuccessPolicy := allSuccess + "-Policy"
	OkFirewall.firewall.Name = allSuccess
	deployedRules := &rulesContainer{
		ingress: []k8sfirewall.ChainRule{
			k8sfirewall.ChainRule{
				Id: 1,
			},
			k8sfirewall.ChainRule{
				Id: 2,
			},
			k8sfirewall.ChainRule{
				Id: 3,
			},
		},
		egress: []k8sfirewall.ChainRule{
			k8sfirewall.ChainRule{
				Id: 1,
			},
			k8sfirewall.ChainRule{
				Id: 2,
			},
			k8sfirewall.ChainRule{
				Id: 3,
			},
			k8sfirewall.ChainRule{
				Id: 4,
			},
		},
	}
	OkFirewall.rules[allSuccessPolicy] = deployedRules
	MockAPI.On("DeleteFirewallChainRuleByID", nil, OkFirewall.firewall.Name, "ingress", mock.AnythingOfType("int32")).Return(&http.Response{}, nil)
	MockAPI.On("DeleteFirewallChainRuleByID", nil, OkFirewall.firewall.Name, "egress", mock.AnythingOfType("int32")).Return(&http.Response{}, nil)

	OkFirewall.CeasePolicy(allSuccessPolicy)
	_, exists := OkFirewall.rules[allSuccessPolicy]
	assert.False(t, exists)

	//	Something in egress did not remove properly
	someFail := "some-fail"
	someFailPolicy := someFail + "-Policy"
	OkFirewall.firewall.Name = someFail
	OkFirewall.rules[someFailPolicy] = deployedRules
	MockAPI.On("DeleteFirewallChainRuleByID", nil, OkFirewall.firewall.Name, "ingress", mock.AnythingOfType("int32")).Return(&http.Response{}, nil)
	MockAPI.On("DeleteFirewallChainRuleByID", nil, OkFirewall.firewall.Name, "egress", int32(1)).Return(&http.Response{}, nil)
	MockAPI.On("DeleteFirewallChainRuleByID", nil, OkFirewall.firewall.Name, "egress", int32(2)).Return(&http.Response{}, nil)
	MockAPI.On("DeleteFirewallChainRuleByID", nil, OkFirewall.firewall.Name, "egress", int32(3)).Return(&http.Response{}, nil)
	MockAPI.On("DeleteFirewallChainRuleByID", nil, OkFirewall.firewall.Name, "egress", int32(4)).Return(&http.Response{}, errors.New("fake-error"))

	OkFirewall.CeasePolicy(someFailPolicy)
	stillThere, exists := OkFirewall.rules[someFailPolicy]
	assert.True(t, exists)
	assert.Empty(t, stillThere.ingress)
	assert.Len(t, stillThere.egress, 1)
}

func TestDestroyFirewall(t *testing.T) {
	//	Successful case
	allSuccess := "all-success"
	OkFirewall.firewall.Name = allSuccess
	MockAPI.On("DeleteFirewallByID", nil, OkFirewall.firewall.Name).Return(&http.Response{}, nil)

	result := OkFirewall.Destroy()
	assert.Nil(t, result)

	//	Failed
	failed := "failed"
	OkFirewall.firewall.Name = failed
	MockAPI.On("DeleteFirewallByID", nil, OkFirewall.firewall.Name).Return(&http.Response{}, errors.New("fake-error"))

	result = OkFirewall.Destroy()
	assert.NotNil(t, result)
}
