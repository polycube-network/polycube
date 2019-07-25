package controllers

import (
	pcn_types "github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/types"
	"k8s.io/client-go/tools/cache"
)

// buildEvent builds the event
func buildEvent(new, old interface{}, eventType pcn_types.EventType) (pcn_types.Event, error) {
	event := pcn_types.Event{
		Type: eventType,
	}

	// The new object
	if new != nil {
		key, err := cache.MetaNamespaceKeyFunc(new)
		if err != nil {
			return pcn_types.Event{}, err
		}
		namespace, _, err := cache.SplitMetaNamespaceKey(key)
		if err != nil {
			return pcn_types.Event{}, err
		}

		event.NewKey = key
		event.Namespace = namespace
		event.NewObject = new
	}

	// The old object
	if old != nil {
		key, err := cache.MetaNamespaceKeyFunc(old)
		if err != nil {
			return pcn_types.Event{}, err
		}
		namespace, _, err := cache.SplitMetaNamespaceKey(key)
		if err != nil {
			return pcn_types.Event{}, err
		}

		event.OldKey = key
		event.Namespace = namespace
		event.OldObject = old
	}

	return event, nil
}
