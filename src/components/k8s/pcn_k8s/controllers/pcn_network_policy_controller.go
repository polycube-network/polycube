package controllers

import (
	"errors"
	"fmt"
	"time"

	v1beta "github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/pkg/apis/polycube.network/v1beta"
	pnp_informer "github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/pkg/client/informers/externalversions/polycube.network/v1beta"
	pcn_types "github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/types"
	meta_v1 "k8s.io/apimachinery/pkg/apis/meta/v1"
	utilruntime "k8s.io/apimachinery/pkg/util/runtime"
	"k8s.io/apimachinery/pkg/util/wait"

	//"k8s.io/client-go/kubernetes"
	"k8s.io/client-go/tools/cache"
	"k8s.io/client-go/util/workqueue"
)

// PcnNetworkPolicyController is the interface of the polycube netpol controller
type PcnNetworkPolicyController interface {
	// Run starts the controller
	Run()
	// Stop will stop the controller
	Stop()
	// Subscribe executes the function consumer when the event event is triggered.
	// It returns an error if the event type does not exist.
	// It returns a function to call when you want to stop tracking that event.
	Subscribe(event pcn_types.EventType, consumer func(*v1beta.PolycubeNetworkPolicy, *v1beta.PolycubeNetworkPolicy)) (func(), error)
	// List gets policies according to a policy query and a namespace query
	List(pQuery, nsQuery *pcn_types.ObjectQuery) ([]v1beta.PolycubeNetworkPolicy, error)
}

// PNPController struct defines all the data needed by the controller
type PNPController struct {
	queue       workqueue.RateLimitingInterface
	informer    cache.SharedIndexInformer
	dispatchers EventDispatchersContainer
	stopCh      chan struct{}
}

func createPnpController() PcnNetworkPolicyController {
	//------------------------------------------------
	// Set up the Controller
	//------------------------------------------------

	// retrieve our custom resource informer which was generated from
	// the code generator and pass it the custom resource client, specifying
	// we should be looking through all namespaces for listing and watching
	informer := pnp_informer.NewPolycubeNetworkPolicyInformer(
		pnpclientset,
		meta_v1.NamespaceAll,
		0,
		cache.Indexers{},
	)

	//------------------------------------------------
	// Set up the queue
	//------------------------------------------------

	// create a new queue so that when the informer gets a resource that is either
	// a result of listing or watching, we can add an idenfitying key to the queue
	// so that it can be handled in the handler
	queue := workqueue.NewRateLimitingQueue(workqueue.DefaultControllerRateLimiter())

	//------------------------------------------------
	// Set up the event handlers
	//------------------------------------------------

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
	return &PNPController{
		queue:       queue,
		informer:    informer,
		dispatchers: dispatchers,
		stopCh:      make(chan struct{}),
	}
}

// Run starts the pod controller
func (p *PNPController) Run() {
	// Don't let panics crash the process
	defer utilruntime.HandleCrash()

	// Let's go!
	go p.informer.Run(p.stopCh)

	// Make sure the cache is populated
	if !cache.WaitForCacheSync(p.stopCh, p.informer.HasSynced) {
		utilruntime.HandleError(fmt.Errorf("Timed out waiting for caches to sync"))
		return
	}

	logger.Infoln("[Polycube Network Policies Controller](Run) Started...")

	// Work *until* something bad happens.
	// If that's the case, wait one second and then re-work again.
	// Well, except when someone tells us to stop... in that case, just stop, man
	wait.Until(p.work, time.Second, p.stopCh)
}

// work gets the item from the queue and attempts to process it
func (p *PNPController) work() {
	f := "[Polycube Network Policies Controller](work) "
	for {
		// Get the item's key from the queue
		_event, quit := p.queue.Get()

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
			err := p.process(event)

			// No errors?
			if err == nil {
				// Then reset the ratelimit counters
				p.queue.Forget(_event)
			} else if p.queue.NumRequeues(_event) < maxRetries {
				// Tried less than the maximum retries?
				logger.Warningf(f+"Error processing item with key %s (will retry): %v", key, err)
				p.queue.AddRateLimited(_event)
			} else {
				// Too many retries?
				logger.Errorf(f+"Error processing %s (giving up): %v", key, err)
				p.queue.Forget(_event)
				utilruntime.HandleError(err)
			}
		} else {
			// Don't process something which is not valid.
			p.queue.Forget(_event)
			utilruntime.HandleError(fmt.Errorf("Error when trying to parse event %#v from the queue", _event))
		}
	}
}

// process will process the event and dispatch the policy event
func (p *PNPController) process(event pcn_types.Event) error {
	var ppol *v1beta.PolycubeNetworkPolicy
	var prev *v1beta.PolycubeNetworkPolicy
	var err error
	defer p.queue.Done(event)

	//-------------------------------------
	//	Retrieve the policy from cache
	//-------------------------------------

	if event.NewObject != nil {
		ppol, err = p.retrievePolicyFromCache(event.NewObject, event.NewKey)
		if err != nil {
			return err
		}
	}
	if event.OldObject != nil {
		_prev, ok := event.OldObject.(*v1beta.PolycubeNetworkPolicy)
		if !ok {
			logger.Errorln("[Polycube Network Policies Controller](process) could not get previous state")
			return fmt.Errorf("could not get previous state")
		}
		prev = _prev
	}

	//-------------------------------------
	// Dispatch the event
	//-------------------------------------

	switch event.Type {
	case pcn_types.New:
		p.dispatchers.new.Dispatch(ppol, nil)
	case pcn_types.Update:
		p.dispatchers.update.Dispatch(ppol, prev)
	case pcn_types.Delete:
		p.dispatchers.delete.Dispatch(nil, ppol)
	}

	return nil
}

// retrievePolicyFromCache retrieves the namespace from the cache.
// It tries to recover it from the tombstone if deleted.
func (p *PNPController) retrievePolicyFromCache(obj interface{}, key string) (*v1beta.PolycubeNetworkPolicy, error) {
	f := "[Polycube Network Policies Controller](retrievePolicyFromCache) "
	// Get the policy by querying the key that kubernetes has assigned to it
	_pol, _, err := p.informer.GetIndexer().GetByKey(key)

	// Errors?
	if err != nil {
		logger.Errorf(f+"An error occurred: cannot find cache element with key %s from store %v", key, err)
		return nil, fmt.Errorf("An error occurred: cannot find cache element with key %s from ", key)
	}

	// Get the policy or try to recover it.
	pol, ok := _pol.(*v1beta.PolycubeNetworkPolicy)
	if !ok {
		pol, ok = obj.(*v1beta.PolycubeNetworkPolicy)
		if !ok {
			tombstone, ok := obj.(cache.DeletedFinalStateUnknown)
			if !ok {
				logger.Errorln(f + "error decoding object, invalid type")
				utilruntime.HandleError(fmt.Errorf("error decoding object, invalid type"))
				return nil, fmt.Errorf("error decoding object, invalid type")
			}
			pol, ok = tombstone.Obj.(*v1beta.PolycubeNetworkPolicy)
			if !ok {
				logger.Errorln(f + "error decoding object tombstone, invalid type")
				utilruntime.HandleError(fmt.Errorf("error decoding object tombstone, invalid type"))
				return nil, fmt.Errorf("error decoding object tombstone, invalid type")
			}
			logger.Infof(f+"Recovered deleted object '%s' from tombstone", pol.GetName())
		}
	}

	return pol, nil
}

// Stop will stop the controller
func (p *PNPController) Stop() {
	// Make them know that exit has been requested
	close(p.stopCh)

	// Shutdown the queue, making the worker unblock
	p.queue.ShutDown()

	// Clean up the dispatchers
	p.dispatchers.new.CleanUp()
	p.dispatchers.update.CleanUp()
	p.dispatchers.delete.CleanUp()

	logger.Infoln("[Polycube Network Policies Controller](Stop) Stopped.")
}

// Subscribe executes the function consumer when the event event is triggered.
// It returns an error if the event type does not exist.
// It returns a function to call when you want to stop tracking that event.
func (p *PNPController) Subscribe(event pcn_types.EventType, consumer func(*v1beta.PolycubeNetworkPolicy, *v1beta.PolycubeNetworkPolicy)) (func(), error) {
	// Prepare the function to be executed
	consumerFunc := (func(current, prev interface{}) {
		//------------------------------------------
		// Init
		//------------------------------------------
		var currentState *v1beta.PolycubeNetworkPolicy
		var prevState *v1beta.PolycubeNetworkPolicy

		// First, cast the items to a pod, so that the consumer will receive
		// exactly what it wants...
		if current != nil {
			currentState = current.(*v1beta.PolycubeNetworkPolicy)
		}
		if prev != nil {
			prevState = prev.(*v1beta.PolycubeNetworkPolicy)
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
		id := p.dispatchers.new.Add(consumerFunc)

		return func() {
			p.dispatchers.new.Remove(id)
		}, nil

	//-------------------------------------
	// Update event
	//-------------------------------------

	case pcn_types.Update:
		id := p.dispatchers.update.Add(consumerFunc)

		return func() {
			p.dispatchers.update.Remove(id)
		}, nil

	//-------------------------------------
	// Delete Event
	//-------------------------------------

	case pcn_types.Delete:
		id := p.dispatchers.delete.Add(consumerFunc)

		return func() {
			p.dispatchers.delete.Remove(id)
		}, nil

	//-------------------------------------
	// Undefined event
	//-------------------------------------

	default:
		return nil, fmt.Errorf("Undefined event type")
	}
}

// List returns a list of polycube network policies that match the specified
// criteria. Returns an empty list if no network policies have been found
// or an error occurred. The error is returned in this case.
func (p *PNPController) List(pQuery, nsQuery *pcn_types.ObjectQuery) ([]v1beta.PolycubeNetworkPolicy, error) {
	//-------------------------------------
	//	Basic query checks
	//-------------------------------------

	// The policy
	if pQuery != nil {
		if pQuery.By != "name" {
			// Network policies can only be found by name.
			return []v1beta.PolycubeNetworkPolicy{}, errors.New("Query type not supported")
		}

		if len(pQuery.Name) == 0 {
			return []v1beta.PolycubeNetworkPolicy{}, errors.New("No name provided")
		}
	}

	// The namespace
	namespace := meta_v1.NamespaceAll
	if nsQuery != nil {
		if nsQuery.By != "name" {
			// Policies cannot be applied to namespace labels, only to names.
			return []v1beta.PolycubeNetworkPolicy{}, errors.New("Query type not supported")
		}

		if len(nsQuery.Name) == 0 {
			return []v1beta.PolycubeNetworkPolicy{}, errors.New("No name provided")
		}
		namespace = nsQuery.Name
	}

	//-------------------------------------
	//	Find by name
	//-------------------------------------

	lister := pnpclientset.PolycubeV1beta().PolycubeNetworkPolicies(namespace)

	//	All?
	if pQuery == nil {
		list, err := lister.List(meta_v1.ListOptions{})
		if err != nil {
			return []v1beta.PolycubeNetworkPolicy{}, err
		}
		return list.Items, nil
	}

	//	Specific name?
	list, err := lister.List(meta_v1.ListOptions{FieldSelector: "metadata.name=" + pQuery.Name})
	if err != nil {
		return []v1beta.PolycubeNetworkPolicy{}, err
	}
	return list.Items, nil
}
