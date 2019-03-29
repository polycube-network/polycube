package controllers

import (
	"testing"
)

func TestGetNsUnrecognizedQuery(t *testing.T) {
	/*controller := &PcnNamespaceController{
		namespaces: map[string]nsStore{},
	}
	_, err := controller.GetNamespaces(pcn_types.PodQueryObject{
		By: "dont-know-lol",
	})
	assert.NotNil(t, err)*/
}

func TestGetNsByName(t *testing.T) {
	/*controller := &PcnNamespaceController{
		namespaces: map[string]nsStore{},
	}

	productionLabels := map[string]string{
		"type": "production",
	}
	betaLabels := map[string]string{
		"type": "beta",
	}

	//	Namespaces
	Production := core_v1.Namespace{
		ObjectMeta: meta_v1.ObjectMeta{
			Name:   "Production",
			Labels: productionLabels,
		},
	}
	controller.addNewNamespace(&Production)
	Beta := core_v1.Namespace{
		ObjectMeta: meta_v1.ObjectMeta{
			Name:   "Beta",
			Labels: betaLabels,
		},
	}
	controller.addNewNamespace(&Beta)

	//	Get all
	allNs := []core_v1.Namespace{Production, Beta}
	result, err := controller.GetNamespaces(pcn_types.PodQueryObject{
		By:   "name",
		Name: "*",
	})
	assert.Nil(t, err)
	assert.ElementsMatch(t, allNs, result)

	//	Only get production
	expectedResult := []core_v1.Namespace{Production}
	result, err = controller.GetNamespaces(pcn_types.PodQueryObject{
		By:   "name",
		Name: Production.Name,
	})
	assert.Nil(t, err)
	assert.ElementsMatch(t, expectedResult, result)

	//	Something that does not exist
	result, err = controller.GetNamespaces(pcn_types.PodQueryObject{
		By:   "name",
		Name: "Staging",
	})
	assert.Nil(t, err)
	assert.Empty(t, result)

	//	Not even a name
	result, err = controller.GetNamespaces(pcn_types.PodQueryObject{
		By:   "name",
		Name: "",
	})
	assert.NotNil(t, err)*/
}

func TestGetNsByLabels(t *testing.T) {
	/*controller := &PcnNamespaceController{
		namespaces: map[string]nsStore{},
	}

	productionLabels := map[string]string{
		"type": "production",
	}
	betaLabels := map[string]string{
		"type": "beta",
	}

	//	Namespaces
	Production := core_v1.Namespace{
		ObjectMeta: meta_v1.ObjectMeta{
			Name:   "Production",
			Labels: productionLabels,
		},
	}
	Beta := core_v1.Namespace{
		ObjectMeta: meta_v1.ObjectMeta{
			Name:   "Beta",
			Labels: betaLabels,
		},
	}
	Beta1 := core_v1.Namespace{
		ObjectMeta: meta_v1.ObjectMeta{
			Name:   "Beta1",
			Labels: betaLabels,
		},
	}
	NoLabels := core_v1.Namespace{
		ObjectMeta: meta_v1.ObjectMeta{
			Name: "NoLabels",
		},
	}
	controller.addNewNamespace(&Production)
	controller.addNewNamespace(&Beta)
	controller.addNewNamespace(&Beta1)
	controller.addNewNamespace(&NoLabels)
	assert.Len(t, controller.namespaces, 4)

	//	Find Beta
	expectedResult := []core_v1.Namespace{Beta, Beta1}
	result, err := controller.GetNamespaces(pcn_types.PodQueryObject{
		By:     "labels",
		Labels: betaLabels,
	})
	assert.Nil(t, err)
	assert.ElementsMatch(t, result, expectedResult)

	//	Find No labels
	expectedResult = []core_v1.Namespace{NoLabels}
	result, err = controller.GetNamespaces(pcn_types.PodQueryObject{
		By:     "labels",
		Labels: map[string]string{},
	})
	assert.Nil(t, err)
	assert.ElementsMatch(t, result, expectedResult)

	//	Find something which does not exist
	result, err = controller.GetNamespaces(pcn_types.PodQueryObject{
		By: "labels",
		Labels: map[string]string{
			"type": "Staging",
		},
	})
	assert.Nil(t, err)
	assert.Empty(t, result)*/
}
