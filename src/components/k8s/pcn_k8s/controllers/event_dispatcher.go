package controllers

import (
	"sync"

	log "github.com/sirupsen/logrus"
)

type EventDispatcher struct {
	name string

	n id

	subscribers map[id]subscriber

	lock sync.RWMutex
}

type subscriber func(interface{})
type id uint

func NewEventDispatcher(name string) *EventDispatcher {

	//	Let them know we're starting
	log.SetLevel(log.DebugLevel)
	var l = log.WithFields(log.Fields{
		"by":     name,
		"method": "NewEventDispatcher()",
	})
	l.Println("%s starting...")

	return &EventDispatcher{
		name:        name,
		n:           0,
		subscribers: make(map[id]subscriber),
	}
}

func (d *EventDispatcher) Dispatch(item interface{}) {

	var l = log.WithFields(log.Fields{
		"by":     d.name,
		"method": "Dispatch()",
	})

	l.Println("Dispatching the event...")

	//	Are there any subscribers?
	if (len(d.subscribers)) < 1 {
		l.Println("There are no subscribers!")
		return
	}

	d.lock.Lock()
	defer d.lock.Unlock()

	//	Loop through all of the subscribers
	for _, s := range d.subscribers {
		//	go s(item)
		s(item)
	}
}

func (d *EventDispatcher) Add(s subscriber) id {

	var l = log.WithFields(log.Fields{
		"by":     d.name,
		"method": "Add()",
	})
	l.Println("Adding a new subscriber")

	d.lock.Lock()
	defer d.lock.Unlock()

	l.Println("Going to add a new subscriber with id %d", d.n+1)

	d.subscribers[d.n+1] = s
	d.n++

	return d.n

}

func (d *EventDispatcher) Remove(i id) {

	var l = log.WithFields(log.Fields{
		"by":     d.name,
		"method": "Remove()",
	})
	l.Println("Going to delete")

	d.lock.Lock()
	defer d.lock.Unlock()

	if _, exists := d.subscribers[i]; exists {
		delete(d.subscribers, i)

		l.Println("subscriber has been deleted")
	}

}

func (d *EventDispatcher) CleanUp() {

	var l = log.WithFields(log.Fields{
		"by":     d.name,
		"method": "CleanUp()",
	})
	l.Println("Going to clean")

	d.lock.Lock()
	defer d.lock.Unlock()

	//	The suggested way to clean up a map, is to create a new empty one
	//	The garbage collector will take care of the rest
	d.subscribers = make(map[id]subscriber)

}
