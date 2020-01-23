package controllers

import (
	"sync"

	"github.com/segmentio/ksuid"
)

// EventDispatcher dispatches the event to all subscribers
type EventDispatcher struct {
	name        string
	subscribers map[string]subscriber
	lock        sync.Mutex
}

// EventDispatchersContainer contains the three basic EventDispatcher-s:
// New, Update, Delete
type EventDispatchersContainer struct {
	new    *EventDispatcher
	update *EventDispatcher
	delete *EventDispatcher
}

type subscriber func(interface{}, interface{})

// NewEventDispatcher starts a new event dispatcher
func NewEventDispatcher() *EventDispatcher {
	return &EventDispatcher{
		subscribers: make(map[string]subscriber),
	}
}

// Dispatch will dispatch the event to the list of subscribers
func (d *EventDispatcher) Dispatch(current, prev interface{}) {
	d.lock.Lock()
	defer d.lock.Unlock()

	// Are there any subscribers?
	if len(d.subscribers) == 0 {
		return
	}

	// Loop through all of the subscribers
	for _, s := range d.subscribers {
		// The controller will make it go
		// go s(current)
		s(current, prev)
	}
}

// Add will add a new subscriber
func (d *EventDispatcher) Add(s subscriber) string {
	d.lock.Lock()
	defer d.lock.Unlock()

	id := ksuid.New().String()
	d.subscribers[id] = s

	return id
}

// Remove will remove a subscriber
func (d *EventDispatcher) Remove(i string) {
	d.lock.Lock()
	defer d.lock.Unlock()

	if _, exists := d.subscribers[i]; exists {
		delete(d.subscribers, i)
	}
}

// CleanUp will remove all subscribers at once
func (d *EventDispatcher) CleanUp() {
	d.lock.Lock()
	defer d.lock.Unlock()

	// The suggested way to clean up a map, is to create a new empty one
	// The garbage collector will take care of the rest
	d.subscribers = make(map[string]subscriber)
}
