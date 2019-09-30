package controllers

import (
	"errors"
	"fmt"
	"strings"
	"time"

	"github.com/polycube-network/polycube/src/components/k8s/utils"

	pcn_types "github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/types"
	core_v1 "k8s.io/api/core/v1"
	meta_v1 "k8s.io/apimachinery/pkg/apis/meta/v1"
	"k8s.io/apimachinery/pkg/runtime"
	utilruntime "k8s.io/apimachinery/pkg/util/runtime"
	"k8s.io/apimachinery/pkg/util/wait"
	"k8s.io/apimachinery/pkg/watch"
	"k8s.io/client-go/tools/cache"
	workqueue "k8s.io/client-go/util/workqueue"
)

// ServiceController is the interface of the service controller
type ServiceController interface {
	// Run starts the service controller
	Run()
	// Stop will stop the service controller
	Stop()
	// Subscribe executes the function consumer when the event event is triggered.
	// It returns an error if the event type does not exist.
	// It returns a function to call when you want to stop tracking that event.
	Subscribe(event pcn_types.EventType, consumer func(*core_v1.Service, *core_v1.Service)) (func(), error)
	// List gets services according to a specific service query and a namespace query
	List(queryServ, queryNs *pcn_types.ObjectQuery) ([]core_v1.Service, error)
}

// PcnServiceController is the implementation of the service controller
type PcnServiceController struct {
	queue       workqueue.RateLimitingInterface
	informer    cache.SharedIndexInformer
	dispatchers EventDispatchersContainer
	stopCh      chan struct{}
}

// createServiceController will start a new service controller
func createServiceController() ServiceController {
	//------------------------------------------------
	// Set up the Service Controller
	//------------------------------------------------

	informer := cache.NewSharedIndexInformer(&cache.ListWatch{
		ListFunc: func(options meta_v1.ListOptions) (runtime.Object, error) {
			return clientset.CoreV1().Services(meta_v1.NamespaceAll).List(options)
		},
		WatchFunc: func(options meta_v1.ListOptions) (watch.Interface, error) {
			return clientset.CoreV1().Services(meta_v1.NamespaceAll).Watch(options)
		},
	},
		&core_v1.Service{},
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

	// Whenever something happens to services, the event is routed by
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
	return &PcnServiceController{
		queue:       queue,
		informer:    informer,
		dispatchers: dispatchers,
		stopCh:      make(chan struct{}),
	}
}

// Run starts the service controller
func (s *PcnServiceController) Run() {
	// Don't let panics crash the process
	defer utilruntime.HandleCrash()

	// Let's go!
	go s.informer.Run(s.stopCh)

	// Make sure the cache is populated
	if !cache.WaitForCacheSync(s.stopCh, s.informer.HasSynced) {
		utilruntime.HandleError(fmt.Errorf("Timed out waiting for caches to sync"))
		return
	}

	logger.Infoln("[Services Controller](Run) Started...")

	// Work *until* something bad happens.
	// If that's the case, wait one second and then re-work again.
	// Well, except when someone tells us to stop... in that case, just stop, man
	wait.Until(s.work, time.Second, s.stopCh)
}

// work gets the item from the queue and attempts to process it
func (s *PcnServiceController) work() {
	f := "[Services Controller](work)"
	for {
		// Get the item's key from the queue
		_event, quit := s.queue.Get()

		if quit {
			logger.Infoln(f + "Quit requested... worker going to exit.")
			return
		}

		event, ok := _event.(pcn_types.Event)
		if ok {
			// Get keys
			key := event.NewKey
			if len(key) == 0 {
				key = event.OldKey
			}
			err := s.process(event)

			// No errors?
			if err == nil {
				// Then reset the ratelimit counters
				s.queue.Forget(_event)
			} else if s.queue.NumRequeues(_event) < maxRetries {
				// Tried less than the maximum retries?
				logger.Warningf(f+"Error processing item with key %s (will retry): %v", key, err)
				s.queue.AddRateLimited(_event)
			} else {
				// Too many retries?
				logger.Errorf(f+"Error processing %s (giving up): %v", key, err)
				s.queue.Forget(_event)
				utilruntime.HandleError(err)
			}
		} else {
			// Don't process something which is not valid.
			s.queue.Forget(_event)
			utilruntime.HandleError(fmt.Errorf("Error when trying to parse event %#v from the queue", _event))
		}
	}
}

// process will process the event and dispatch the service event
func (s *PcnServiceController) process(event pcn_types.Event) error {
	var serv *core_v1.Service
	var prev *core_v1.Service
	var err error
	defer s.queue.Done(event)

	//-------------------------------------
	//	Retrieve the service from cache
	//-------------------------------------

	if event.NewObject != nil {
		serv, err = s.retrieveServiceFromCache(event.NewObject, event.NewKey)
		if err != nil {
			return err
		}
	}
	if event.OldObject != nil {
		_prev, ok := event.OldObject.(*core_v1.Service)
		if !ok {
			logger.Errorln("[Services Controller](process) could not get previous state")
			return fmt.Errorf("could not get previous state")
		}
		prev = _prev
	}

	//-------------------------------------
	// Dispatch the event
	//-------------------------------------

	switch event.Type {
	case pcn_types.New:
		s.dispatchers.new.Dispatch(serv, nil)
	case pcn_types.Update:
		s.dispatchers.update.Dispatch(serv, prev)
	case pcn_types.Delete:
		s.dispatchers.delete.Dispatch(nil, serv)
	}

	return nil
}

// retrieveServiceFromCache retrieves the namespace from the cache.
// It tries to recover it from the tombstone if deleted.
func (s *PcnServiceController) retrieveServiceFromCache(obj interface{}, key string) (*core_v1.Service, error) {
	f := "[Services Controller](retrieveServiceFromCache)"
	// Get the service by querying the key that kubernetes has assigned to it
	_serv, _, err := s.informer.GetIndexer().GetByKey(key)

	// Errors?
	if err != nil {
		logger.Errorf(f+"An error occurred: cannot find cache element with key %s from store %v", key, err)
		return nil, fmt.Errorf("An error occurred: cannot find cache element with key %s from ", key)
	}

	// Get the serv or try to recover it.
	serv, ok := _serv.(*core_v1.Service)
	if !ok {
		serv, ok = obj.(*core_v1.Service)
		if !ok {
			tombstone, ok := obj.(cache.DeletedFinalStateUnknown)
			if !ok {
				logger.Errorln(f + "error decoding object, invalid type")
				utilruntime.HandleError(fmt.Errorf("error decoding object, invalid type"))
				return nil, fmt.Errorf("error decoding object, invalid type")
			}
			serv, ok = tombstone.Obj.(*core_v1.Service)
			if !ok {
				logger.Errorln(f + "error decoding object tombstone, invalid type")
				utilruntime.HandleError(fmt.Errorf("error decoding object tombstone, invalid type"))
				return nil, fmt.Errorf("error decoding object tombstone, invalid type")
			}
			logger.Infof(f+"Recovered deleted object '%s' from tombstone", serv.GetName())
		}
	}

	return serv, nil
}

// Stop will stop the service controller
func (s *PcnServiceController) Stop() {
	// Make them know that exit has been requested
	close(s.stopCh)

	// Shutdown the queue, making the worker unblock
	s.queue.ShutDown()

	// Clean up the dispatchers
	s.dispatchers.new.CleanUp()
	s.dispatchers.update.CleanUp()
	s.dispatchers.delete.CleanUp()

	logger.Infoln("[Services Controller](Stop) Stopped.")
}

// Subscribe executes the function consumer when the event event is triggered.
// It returns an error if the event type does not exist.
// It returns a function to call when you want to stop tracking that event.
func (s *PcnServiceController) Subscribe(event pcn_types.EventType, consumer func(*core_v1.Service, *core_v1.Service)) (func(), error) {
	// Prepare the function to be executed
	consumerFunc := (func(current, prev interface{}) {
		//------------------------------------------
		// Init
		//------------------------------------------
		var currentState *core_v1.Service
		var prevState *core_v1.Service

		// First, cast the items to a service, so that the consumer will receive
		// exactly what it wants...
		if current != nil {
			currentState = current.(*core_v1.Service)
		}
		if prev != nil {
			prevState = prev.(*core_v1.Service)
		}

		//------------------------------------------
		// Execute
		//------------------------------------------

		// Then, execute the consumer in a separate thread.
		// NOTE: this step can also be done in the event dispatcher,
		// but I want them to be oblivious of the type they're handling.
		// This way, the event dispatcher is as general as possible.
		// (also, it is not their concern to cast objects.)
		go consumer(currentState, prevState)
	})

	// What event are you subscribing to?
	switch event {

	//-------------------------------------
	// New event
	//-------------------------------------

	case pcn_types.New:
		id := s.dispatchers.new.Add(consumerFunc)

		return func() {
			s.dispatchers.new.Remove(id)
		}, nil

	//-------------------------------------
	// Update event
	//-------------------------------------

	case pcn_types.Update:
		id := s.dispatchers.update.Add(consumerFunc)

		return func() {
			s.dispatchers.update.Remove(id)
		}, nil

	//-------------------------------------
	// Delete Event
	//-------------------------------------

	case pcn_types.Delete:
		id := s.dispatchers.delete.Add(consumerFunc)

		return func() {
			s.dispatchers.delete.Remove(id)
		}, nil

	//-------------------------------------
	// Undefined event
	//-------------------------------------

	default:
		return nil, fmt.Errorf("Undefined event type")
	}
}

// List gets service according to a specific service query and a namespace query.
// NOTE: labels, in this case, are those in the spec.Selector, not those of the service itself
func (s *PcnServiceController) List(queryServ, queryNs *pcn_types.ObjectQuery) ([]core_v1.Service, error) {
	// The namespaces the services must be found on
	// If this stays empty it means that I don't care about the namespace they are in
	ns := map[string]bool{}

	//------------------------------------------------
	// Preliminary checks
	//------------------------------------------------
	// -- The namespace
	nsList, err := Namespaces().List(queryNs)
	if err != nil {
		return []core_v1.Service{}, err
	}
	if len(nsList) == 0 {
		// If no namespace is found, it is useless to go on searching for services
		return []core_v1.Service{}, nil
	}
	for _, n := range nsList {
		ns[n.Name] = true
	}

	// BRIEF: from now on, this function has been split in mini-functions
	// inside it for sack of clarity.
	// Jump at the bottom of it to understand it better.

	//-------------------------------------
	// Find by name
	//-------------------------------------

	byName := func(name string) ([]core_v1.Service, error) {
		list := []core_v1.Service{}

		// Do I care or not about the namespace?
		// If not, I'll put the NamespaceAll inside the map as its only value
		if len(ns) == 0 {
			ns[meta_v1.NamespaceAll] = true
		}

		// Loop through all interested namespaces
		for namespace := range ns {
			listOptions := meta_v1.ListOptions{}

			//	Name provided?
			if len(name) != 0 {
				listOptions.FieldSelector = "metadata.name=" + name
			}

			// Get 'em
			lister, err := clientset.CoreV1().Services(namespace).List(listOptions)
			if err == nil {
				list = append(list, lister.Items...)
			} else {
				//return []core_v1.Service, err
				// Just skip this namespace.
			}
		}
		return list, nil
	}

	//-------------------------------------
	// Find by labels
	//-------------------------------------

	byLabels := func(labels map[string]string) ([]core_v1.Service, error) {
		if labels == nil {
			return []core_v1.Service{}, errors.New("Service labels is nil")
		}
		if len(labels) == 0 {
			// If you need to get all Services, put the query as nil
			return []core_v1.Service{}, errors.New("No service labels provided")
		}
		list := []core_v1.Service{}

		// -- If you want to get the service spec.selector...
		// We have to go through all of them :(
		for namespace := range ns {
			// Get 'em
			lister, err := clientset.CoreV1().Services(namespace).List(meta_v1.ListOptions{})
			if err == nil {
				for _, currentServ := range lister.Items {
					if len(currentServ.Spec.Selector) > 0 && utils.AreLabelsContained(currentServ.Spec.Selector, labels) {
						list = append(list, currentServ)
					}
				}
			} else {
				//return []core_v1.Service, err
				// Just skip this namespace.
			}
		}

		return list, nil
	}

	//-------------------------------------
	// Find 'em
	//-------------------------------------

	if queryServ == nil {
		return byName("")
	}

	switch strings.ToLower(queryServ.By) {
	case "name":
		return byName(queryServ.Name)
	case "labels":
		return byLabels(queryServ.Labels)
	default:
		return []core_v1.Service{}, errors.New("Unrecognized service query")
	}
}
