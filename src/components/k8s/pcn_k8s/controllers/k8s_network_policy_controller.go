package controllers

import (
	"errors"
	"fmt"
	"time"

	pcn_types "github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/types"
	networking_v1 "k8s.io/api/networking/v1"
	meta_v1 "k8s.io/apimachinery/pkg/apis/meta/v1"
	"k8s.io/apimachinery/pkg/runtime"
	utilruntime "k8s.io/apimachinery/pkg/util/runtime"
	"k8s.io/apimachinery/pkg/util/wait"
	"k8s.io/apimachinery/pkg/watch"
	"k8s.io/client-go/tools/cache"
	workqueue "k8s.io/client-go/util/workqueue"
)

// K8sNetworkPolicyController is the interface of the Kubernetes Network
// Policies Controller
type K8sNetworkPolicyController interface {
	// Run starts the controller
	Run()
	// Stop will stop the controller
	Stop()
	// Subscribe executes the function consumer when the event event is triggered.
	// It returns an error if the event type does not exist.
	// It returns a function to call when you want to stop tracking that event.
	Subscribe(event pcn_types.EventType, consumer func(*networking_v1.NetworkPolicy, *networking_v1.NetworkPolicy)) (func(), error)
	// List gets policies according to a specific query
	List(pQuery, nsQuery *pcn_types.ObjectQuery) ([]networking_v1.NetworkPolicy, error)
}

// PcnK8sNetworkPolicyController is the implementation of
// the k8s network policy controller
type PcnK8sNetworkPolicyController struct {
	// queue contains the events to be processed
	queue workqueue.RateLimitingInterface
	// informer is the informer that gets
	// the list of policies
	informer cache.SharedIndexInformer
	// dispatchers is the structure that dispatches the event
	// to the interested subscribers
	dispatchers EventDispatchersContainer
	// stopCh is the channel used to stop the controller
	stopCh chan struct{}
}

// createK8sNetworkPolicyController creates a new policy controller.
// Meant to be a singleton.
func createK8sNetworkPolicyController() K8sNetworkPolicyController {
	//------------------------------------------------
	// Set up the default network policies informer
	//------------------------------------------------

	npcInformer := cache.NewSharedIndexInformer(&cache.ListWatch{
		ListFunc: func(options meta_v1.ListOptions) (runtime.Object, error) {
			return clientset.NetworkingV1().NetworkPolicies(meta_v1.NamespaceAll).List(options)
		},
		WatchFunc: func(options meta_v1.ListOptions) (watch.Interface, error) {
			return clientset.NetworkingV1().NetworkPolicies(meta_v1.NamespaceAll).Watch(options)
		},
	},
		&networking_v1.NetworkPolicy{},
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

	// Whenever something happens to network policies, the event is routed by
	// this event handler and routed to the queue. It'll know what to do.
	npcInformer.AddEventHandler(cache.ResourceEventHandlerFuncs{
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
	return &PcnK8sNetworkPolicyController{
		queue:       queue,
		informer:    npcInformer,
		dispatchers: dispatchers,
		stopCh:      make(chan struct{}),
	}
}

// Run starts the network policy controller
func (npc *PcnK8sNetworkPolicyController) Run() {
	// Don't let panics crash the process
	defer utilruntime.HandleCrash()

	// Let's go!
	go npc.informer.Run(npc.stopCh)

	// Make sure the cache is populated
	if !cache.WaitForCacheSync(npc.stopCh, npc.informer.HasSynced) {
		utilruntime.HandleError(fmt.Errorf("Timed out waiting for caches to sync"))
		return
	}

	logger.Infoln("[K8s Network Policies Controller](Run) Started...")

	// Work *until* something bad happens.
	// If that's the case, wait one second and then re-work again.
	// Well, except when someone tells us to stop... in that case, just stop, man
	wait.Until(npc.work, time.Second, npc.stopCh)
}

// work gets the item from the queue and attempts to process it.
func (npc *PcnK8sNetworkPolicyController) work() {
	for {
		// Get the item's key from the queue
		_event, quit := npc.queue.Get()
		if quit {
			logger.Infoln("[K8s Network Policies Controller](work) Quit requested... worker going to exit.")
			return
		}

		event, ok := _event.(pcn_types.Event)
		if ok {
			// Get keys
			key := event.NewKey
			if len(key) == 0 {
				key = event.OldKey
			}
			err := npc.processPolicy(event)

			// No errors?
			if err == nil {
				// Then reset the ratelimit counters
				npc.queue.Forget(_event)
			} else if npc.queue.NumRequeues(_event) < maxRetries {
				// Tried less than the maximum retries?
				logger.Warningf("[K8s Network Policies Controller](work) Error processing item with key %s (will retry): %v", key, err)
				npc.queue.AddRateLimited(_event)
			} else {
				// Too many retries?
				logger.Errorf("[K8s Network Policies Controller](work) Error processing %s (giving up): %v", key, err)
				npc.queue.Forget(_event)
				utilruntime.HandleError(err)
			}
		} else {
			// Don't process something which is not valid.
			npc.queue.Forget(_event)
			utilruntime.HandleError(fmt.Errorf("Error when trying to parse event %#v from the queue", _event))
		}
	}
}

// processPolicy will process the policy and dispatch it
func (npc *PcnK8sNetworkPolicyController) processPolicy(event pcn_types.Event) error {
	var policy *networking_v1.NetworkPolicy
	var prev *networking_v1.NetworkPolicy
	var err error
	defer npc.queue.Done(event)

	//-------------------------------------
	//	Retrieve the policy from cache
	//-------------------------------------

	if event.NewObject != nil {
		policy, err = npc.retrievePolicyFromCache(event.NewObject, event.NewKey)
		if err != nil {
			return err
		}
	}
	if event.OldObject != nil {
		_prev, ok := event.OldObject.(*networking_v1.NetworkPolicy)
		if !ok {
			logger.Errorln("[K8s Network Policies Controller](processPolicy) could not get previous state")
			return fmt.Errorf("could not get previous state")
		}
		prev = _prev
	}

	//-------------------------------------
	//	Dispatch the event
	//-------------------------------------

	switch event.Type {
	case pcn_types.New:
		npc.dispatchers.new.Dispatch(policy, nil)
	case pcn_types.Update:
		npc.dispatchers.update.Dispatch(policy, prev)
	case pcn_types.Delete:
		npc.dispatchers.delete.Dispatch(nil, policy)
	}

	return nil
}

func (npc *PcnK8sNetworkPolicyController) retrievePolicyFromCache(obj interface{}, key string) (*networking_v1.NetworkPolicy, error) {
	f := "[K8s Network Policies Controller](retrievePolicyFromCache) "
	// Get the policy by querying the key that kubernetes has assigned to it
	_policy, _, err := npc.informer.GetIndexer().GetByKey(key)

	// Errors?
	if err != nil {
		logger.Errorf(f+"An error occurred: cannot find cache element with key %s from store %v", key, err)
		return nil, fmt.Errorf("An error occurred: cannot find cache element with key %s from ", key)
	}

	// Get the policy or try to recover it.
	policy, ok := _policy.(*networking_v1.NetworkPolicy)
	if !ok {
		policy, ok = obj.(*networking_v1.NetworkPolicy)
		if !ok {
			tombstone, ok := obj.(cache.DeletedFinalStateUnknown)
			if !ok {
				logger.Errorln(f + "error decoding object, invalid type")
				utilruntime.HandleError(fmt.Errorf("error decoding object, invalid type"))
				return nil, fmt.Errorf("error decoding object, invalid type")
			}
			policy, ok = tombstone.Obj.(*networking_v1.NetworkPolicy)
			if !ok {
				logger.Errorln(f + "error decoding object tombstone, invalid type")
				utilruntime.HandleError(fmt.Errorf("error decoding object tombstone, invalid type"))
				return nil, fmt.Errorf("error decoding object tombstone, invalid type")
			}
			logger.Infof(f+"Recovered deleted object '%s' from tombstone", policy.GetName())
		}
	}

	return policy, nil
}

// List returns a list of network policies that match the specified
// criteria. Returns an empty list if no network policies have been found
// or an error occurred. The error is returned in this case.
func (npc *PcnK8sNetworkPolicyController) List(pQuery, nsQuery *pcn_types.ObjectQuery) ([]networking_v1.NetworkPolicy, error) {
	//-------------------------------------
	//	Basic query checks
	//-------------------------------------

	// The policy
	if pQuery != nil {
		if pQuery.By != "name" {
			// Network policies can only be found by name.
			return []networking_v1.NetworkPolicy{}, errors.New("Query type not supported")
		}

		if len(pQuery.Name) == 0 {
			return []networking_v1.NetworkPolicy{}, errors.New("No name provided")
		}
	}

	// The namespace
	namespace := meta_v1.NamespaceAll
	if nsQuery != nil {
		if nsQuery.By != "name" {
			// Policies cannot be applied to namespace labels, only to names.
			return []networking_v1.NetworkPolicy{}, errors.New("Query type not supported")
		}

		if len(nsQuery.Name) == 0 {
			return []networking_v1.NetworkPolicy{}, errors.New("No name provided")
		}
		namespace = nsQuery.Name
	}

	//-------------------------------------
	//	Find by name
	//-------------------------------------

	lister := clientset.NetworkingV1().NetworkPolicies(namespace)

	//	All kubernetes policies?
	if pQuery == nil {
		list, err := lister.List(meta_v1.ListOptions{})
		if err != nil {
			return []networking_v1.NetworkPolicy{}, err
		}
		return list.Items, nil
	}

	//	Specific name?
	list, err := lister.List(meta_v1.ListOptions{FieldSelector: "metadata.name=" + pQuery.Name})
	if err != nil {
		return []networking_v1.NetworkPolicy{}, err
	}
	return list.Items, nil
}

// Stop stops the controller
func (npc *PcnK8sNetworkPolicyController) Stop() {
	// Make them know that exit has been requested
	close(npc.stopCh)

	// Shutdown the queue, making the worker unblock
	npc.queue.ShutDown()

	// Clean up the dispatchers
	npc.dispatchers.new.CleanUp()
	npc.dispatchers.update.CleanUp()
	npc.dispatchers.delete.CleanUp()

	logger.Infoln("[K8s Network Policies Controller](Stop) K8s network policy controller exited.")
}

// Subscribe executes the function consumer when the event event is triggered.
// It returns an error if the event type does not exist.
// It returns a function to call when you want to stop tracking that event.
func (npc *PcnK8sNetworkPolicyController) Subscribe(event pcn_types.EventType, consumer func(*networking_v1.NetworkPolicy, *networking_v1.NetworkPolicy)) (func(), error) {
	//	Prepare the function to be executed
	consumerFunc := (func(current, prev interface{}) {
		//------------------------------------------
		// Init
		//------------------------------------------
		var currentState *networking_v1.NetworkPolicy
		var prevState *networking_v1.NetworkPolicy

		// First, cast the item to a pod, so that the consumer will receive exactly
		// what it wants...
		if current != nil {
			currentState = current.(*networking_v1.NetworkPolicy)
		}
		if prev != nil {
			prevState = prev.(*networking_v1.NetworkPolicy)
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

	//	What event are you subscribing to?
	switch event {

	//-------------------------------------
	//	New event
	//-------------------------------------

	case pcn_types.New:
		id := npc.dispatchers.new.Add(consumerFunc)

		return func() {
			npc.dispatchers.new.Remove(id)
		}, nil

	//-------------------------------------
	//	Update event
	//-------------------------------------

	case pcn_types.Update:
		id := npc.dispatchers.update.Add(consumerFunc)

		return func() {
			npc.dispatchers.update.Remove(id)
		}, nil

	//-------------------------------------
	//	Delete Event
	//-------------------------------------

	case pcn_types.Delete:
		id := npc.dispatchers.delete.Add(consumerFunc)

		return func() {
			npc.dispatchers.delete.Remove(id)
		}, nil

	//-------------------------------------
	//	Undefined event
	//-------------------------------------

	default:
		return nil, fmt.Errorf("Undefined event type")
	}
}
