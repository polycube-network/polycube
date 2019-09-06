package controllers

import (
	"errors"
	"fmt"
	"strings"
	"time"

	pcn_types "github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/types"
	"github.com/polycube-network/polycube/src/components/k8s/utils"
	core_v1 "k8s.io/api/core/v1"
	meta_v1 "k8s.io/apimachinery/pkg/apis/meta/v1"
	"k8s.io/apimachinery/pkg/runtime"
	utilruntime "k8s.io/apimachinery/pkg/util/runtime"
	"k8s.io/apimachinery/pkg/util/wait"
	"k8s.io/apimachinery/pkg/watch"
	"k8s.io/client-go/tools/cache"
	workqueue "k8s.io/client-go/util/workqueue"
)

// NamespaceController is the interface of the Namespace Controller
type NamespaceController interface {
	// Run starts the namespace controller
	Run()
	// Stop will stop the namespace controller
	Stop()
	// Subscribe executes the function consumer when the event event is triggered.
	// It returns an error if the event type does not exist.
	// It returns a function to call when you want to stop tracking that event.
	Subscribe(event pcn_types.EventType, consumer func(*core_v1.Namespace, *core_v1.Namespace)) (func(), error)
	// GetNamespaces gets namespaces according to a specific namespace query
	GetNamespaces(query *pcn_types.ObjectQuery) ([]core_v1.Namespace, error)
}

// PcnNamespaceController is the implementation of the Namespace Controller
type PcnNamespaceController struct {
	queue       workqueue.RateLimitingInterface
	informer    cache.SharedIndexInformer
	dispatchers EventDispatchersContainer
	stopCh      chan struct{}
}

// createNsController returns a new Namespace Controller
func createNsController() NamespaceController {
	//------------------------------------------------
	// Set up the Namespace Controller
	//------------------------------------------------

	informer := cache.NewSharedIndexInformer(&cache.ListWatch{
		ListFunc: func(options meta_v1.ListOptions) (runtime.Object, error) {
			return clientset.CoreV1().Namespaces().List(options)
		},
		WatchFunc: func(options meta_v1.ListOptions) (watch.Interface, error) {
			return clientset.CoreV1().Namespaces().Watch(options)
		},
	},
		&core_v1.Namespace{},
		0, //Skip resync
		cache.Indexers{},
	)

	//------------------------------------------------
	// Set up the queue
	//------------------------------------------------

	// Start the queue
	queue := workqueue.NewRateLimitingQueue(workqueue.DefaultControllerRateLimiter())

	//------------------------------------------------
	// Set up the event handlers
	//------------------------------------------------

	// Whenever something happens to pods, the event is routed by
	// this event handler and routed to the queue. It'll know what to do.
	informer.AddEventHandler(cache.ResourceEventHandlerFuncs{
		AddFunc: func(obj interface{}) {
			event, err := buildEvent(obj, nil, pcn_types.New)
			if err != nil {
				utilruntime.HandleError(err)
				return
			}
			queue.Add(event)
		},
		UpdateFunc: func(old, new interface{}) {
			event, err := buildEvent(new, old, pcn_types.Update)
			if err != nil {
				utilruntime.HandleError(err)
				return
			}

			queue.Add(event)
		},
		DeleteFunc: func(obj interface{}) {
			// NOTE: in case of a delete, we set this object as 'new' here.
			// Later, it will be notified as old because it is going to
			// be retrieved from tombstone (the current state does not
			// exist... because it's dead.)
			event, err := buildEvent(obj, nil, pcn_types.Delete)
			if err != nil {
				utilruntime.HandleError(err)
				return
			}

			queue.Add(event)
		},
	})

	//------------------------------------------------
	// Set up the dispatchers
	//------------------------------------------------

	dispatchers := EventDispatchersContainer{
		new:    NewEventDispatcher(),
		update: NewEventDispatcher(),
		delete: NewEventDispatcher(),
	}

	// Everything set up, return the controller
	return &PcnNamespaceController{
		queue:       queue,
		informer:    informer,
		dispatchers: dispatchers,
		stopCh:      make(chan struct{}),
	}
}

// Run starts the namespace controller
func (n *PcnNamespaceController) Run() {
	// Don't let panics crash the process
	defer utilruntime.HandleCrash()

	// Let's go!
	go n.informer.Run(n.stopCh)

	// Make sure the cache is populated
	if !cache.WaitForCacheSync(n.stopCh, n.informer.HasSynced) {
		utilruntime.HandleError(fmt.Errorf("Timed out waiting for caches to sync"))
		return
	}

	logger.Infoln("Started...")

	// Work *until* something bad happens.
	// If that's the case, wait one second and then re-work again.
	// Well, except when someone tells us to stop... in that case, just stop, man
	wait.Until(n.work, time.Second, n.stopCh)
}

func (n *PcnNamespaceController) work() {
	for {
		// Get the item's key from the queue
		_event, quit := n.queue.Get()

		if quit {
			logger.Infoln("Quit requested... worker going to exit.")
			return
		}

		event, ok := _event.(pcn_types.Event)
		if ok {
			// Get keys
			key := event.NewKey
			if len(key) == 0 {
				key = event.OldKey
			}
			err := n.process(event)

			// No errors?
			if err == nil {
				// Then reset the ratelimit counters
				n.queue.Forget(_event)
			} else if n.queue.NumRequeues(_event) < maxRetries {
				// Tried less than the maximum retries?
				logger.Warningf("Error processing item with key %s (will retry): %v", key, err)
				n.queue.AddRateLimited(_event)
			} else {
				// Too many retries?
				logger.Errorf("Error processing %s (giving up): %v", key, err)
				n.queue.Forget(_event)
				utilruntime.HandleError(err)
			}
		} else {
			// Don't process something which is not valid.
			n.queue.Forget(_event)
			utilruntime.HandleError(fmt.Errorf("Error when trying to parse event %#v from the queue", _event))
		}
	}
}

// process will process the event and dispatch the namespace event
func (n *PcnNamespaceController) process(event pcn_types.Event) error {
	var ns *core_v1.Namespace
	var prev *core_v1.Namespace
	var err error
	defer n.queue.Done(event)

	//-------------------------------------
	//	Retrieve the namespace from cache
	//-------------------------------------

	if event.NewObject != nil {
		ns, err = n.retrieveNsFromCache(event.NewObject, event.NewKey)
		if err != nil {
			return err
		}
	}
	if event.OldObject != nil {
		_prev, ok := event.OldObject.(*core_v1.Namespace)
		if !ok {
			logger.Errorln("could not get previous state")
			return fmt.Errorf("could not get previous state")
		}
		prev = _prev
	}

	//-------------------------------------
	// Dispatch the event
	//-------------------------------------

	switch event.Type {
	case pcn_types.New:
		n.dispatchers.new.Dispatch(ns, nil)
	case pcn_types.Update:
		n.dispatchers.update.Dispatch(ns, prev)
	case pcn_types.Delete:
		n.dispatchers.delete.Dispatch(nil, ns)
	}

	return nil
}

// retrieveNsFromCache retrieves the namespace from the cache.
// It tries to recover it from the tombstone if deleted.
func (n *PcnNamespaceController) retrieveNsFromCache(obj interface{}, key string) (*core_v1.Namespace, error) {
	// Get the namespace by querying the key that kubernetes has assigned to it
	_ns, _, err := n.informer.GetIndexer().GetByKey(key)

	// Errors?
	if err != nil {
		logger.Errorf("An error occurred: cannot find cache element with key %s from store %v", key, err)
		return nil, fmt.Errorf("An error occurred: cannot find cache element with key %s from ", key)
	}

	// Get the namespace or try to recover it.
	ns, ok := _ns.(*core_v1.Namespace)
	if !ok {
		ns, ok = obj.(*core_v1.Namespace)
		if !ok {
			tombstone, ok := obj.(cache.DeletedFinalStateUnknown)
			if !ok {
				logger.Errorln("error decoding object, invalid type")
				utilruntime.HandleError(fmt.Errorf("error decoding object, invalid type"))
				return nil, fmt.Errorf("error decoding object, invalid type")
			}
			ns, ok = tombstone.Obj.(*core_v1.Namespace)
			if !ok {
				logger.Errorln("error decoding object tombstone, invalid type")
				utilruntime.HandleError(fmt.Errorf("error decoding object tombstone, invalid type"))
				return nil, fmt.Errorf("error decoding object tombstone, invalid type")
			}
			logger.Infof("Recovered deleted object '%s' from tombstone", ns.GetName())
		}
	}

	return ns, nil
}

// Stop will stop the namespace controller
func (n *PcnNamespaceController) Stop() {
	// Make them know that exit has been requested
	close(n.stopCh)

	// Shutdown the queue, making the worker unblock
	n.queue.ShutDown()

	// Clean up the dispatchers
	n.dispatchers.new.CleanUp()
	n.dispatchers.update.CleanUp()
	n.dispatchers.delete.CleanUp()

	logger.Infoln("Stopped.")
}

// Subscribe executes the function consumer when the event event is triggered.
// It returns an error if the event type does not exist.
// It returns a function to call when you want to stop tracking that event.
func (n *PcnNamespaceController) Subscribe(event pcn_types.EventType, consumer func(*core_v1.Namespace, *core_v1.Namespace)) (func(), error) {

	// Prepare the function to be executed
	consumerFunc := (func(current, prev interface{}) {
		var currentState *core_v1.Namespace
		var prevState *core_v1.Namespace

		// First, cast the items, so that the consumer will receive
		// exactly what it wants...
		if current != nil {
			currentState = current.(*core_v1.Namespace)
		}
		if prev != nil {
			prevState = prev.(*core_v1.Namespace)
		}

		// Then, execute the consumer in a separate thread.
		// NOTE: this step can also be done in the event dispatcher, but I want it to make them oblivious of the type they're handling.
		// This way, the event dispatcher is as general as possible (also, it is not their concern to cast objects.)
		go consumer(currentState, prevState)
	})

	// What event are you subscribing to?
	switch event {

	//-------------------------------------
	// New event
	//-------------------------------------

	case pcn_types.New:
		id := n.dispatchers.new.Add(consumerFunc)

		return func() {
			n.dispatchers.new.Remove(id)
		}, nil

	//-------------------------------------
	// Update event
	//-------------------------------------

	case pcn_types.Update:
		id := n.dispatchers.update.Add(consumerFunc)

		return func() {
			n.dispatchers.update.Remove(id)
		}, nil

	//-------------------------------------
	// Delete Event
	//-------------------------------------

	case pcn_types.Delete:
		id := n.dispatchers.delete.Add(consumerFunc)

		return func() {
			n.dispatchers.delete.Remove(id)
		}, nil

	//-------------------------------------
	// Undefined event
	//-------------------------------------

	default:
		return nil, fmt.Errorf("Undefined event type")
	}
}

// GetNamespaces gets namespaces according to a specific namespace query
func (n *PcnNamespaceController) GetNamespaces(query *pcn_types.ObjectQuery) ([]core_v1.Namespace, error) {
	nsInterface := clientset.CoreV1().Namespaces()

	//-------------------------------------
	// All namespaces
	//-------------------------------------

	if query == nil {
		listOptions := meta_v1.ListOptions{}
		lister, err := nsInterface.List(listOptions)
		return lister.Items, err
	}

	// BRIEF: from now on, this function has been split into mini functions
	// to keep it more organized.

	//-------------------------------------
	// Find by name
	//-------------------------------------

	byName := func(name string) ([]core_v1.Namespace, error) {
		if len(name) == 0 {
			return []core_v1.Namespace{}, errors.New("Namespace name not provided")
		}

		lister, err := nsInterface.List(meta_v1.ListOptions{
			FieldSelector: "metadata.name=" + name,
		})
		return lister.Items, err
	}

	//-------------------------------------
	// Find by labels
	//-------------------------------------

	byLabels := func(labels map[string]string) ([]core_v1.Namespace, error) {
		if labels == nil {
			return []core_v1.Namespace{}, errors.New("Namespace labels is nil")
		}

		lister, err := nsInterface.List(meta_v1.ListOptions{
			LabelSelector: utils.ImplodeLabels(labels, ",", false),
		})

		return lister.Items, err
	}

	//-------------------------------------
	// Main entry point
	//-------------------------------------

	// Get the appropriate function
	switch strings.ToLower(query.By) {
	case "name":
		return byName(query.Name)
	case "labels":
		return byLabels(query.Labels)
	default:
		return []core_v1.Namespace{}, errors.New("Unrecognized namespace query")
	}
}
