package controllers

import (
	"fmt"
	"time"

	pcn_types "github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/types"

	log "github.com/sirupsen/logrus"
	networking_v1 "k8s.io/api/networking/v1"
	meta_v1 "k8s.io/apimachinery/pkg/apis/meta/v1"
	"k8s.io/apimachinery/pkg/runtime"
	utilruntime "k8s.io/apimachinery/pkg/util/runtime"
	"k8s.io/apimachinery/pkg/util/wait"
	"k8s.io/apimachinery/pkg/watch"
	"k8s.io/client-go/kubernetes"
	"k8s.io/client-go/tools/cache"
	workqueue "k8s.io/client-go/util/workqueue"
)

// K8sNetworkPolicyController is the implementation of
// the k8s network policy controller
type K8sNetworkPolicyController struct {
	// clientset is the clientset of kubernetes
	clientset *kubernetes.Clientset
	// queue contains the events to be processed
	queue workqueue.RateLimitingInterface
	// k8sNetworkPoliciesInformer is the informer that gets
	// the list of policies
	k8sNetworkPoliciesInformer cache.SharedIndexInformer
	// startedOn tells when the controller started working
	startedOn time.Time
	// dispatchers is the structure that dispatches the event
	// to the interested subscribers
	dispatchers EventDispatchersContainer
	// stopCh is the channel used to stop the controller
	stopCh chan struct{}
	// maxRetries tells how many times the controller should attempt
	// decoding an object from the queue
	maxRetries int
}

// NewK8sNetworkPolicyController creates a new policy controller.
// Meant to be a singleton.
func NewK8sNetworkPolicyController(clientset *kubernetes.Clientset) *K8sNetworkPolicyController {
	maxRetries := 5

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
	return &K8sNetworkPolicyController{
		clientset:                  clientset,
		queue:                      queue,
		k8sNetworkPoliciesInformer: npcInformer,
		dispatchers:                dispatchers,
		maxRetries:                 maxRetries,
		stopCh:                     make(chan struct{}),
	}
}

// Run starts the network policy controller
func (npc *K8sNetworkPolicyController) Run() {
	l := log.New().WithFields(log.Fields{"by": KNPC, "method": "Run()"})

	// Don't let panics crash the process
	defer utilruntime.HandleCrash()

	// Record when we started, it is going to be used later
	npc.startedOn = time.Now().UTC()

	// Let's go!
	go npc.k8sNetworkPoliciesInformer.Run(npc.stopCh)

	// Make sure the cache is populated
	if !cache.WaitForCacheSync(npc.stopCh, npc.k8sNetworkPoliciesInformer.HasSynced) {
		utilruntime.HandleError(fmt.Errorf("Timed out waiting for caches to sync"))
		return
	}

	l.Infoln("Started...")

	// Work *until* something bad happens.
	// If that's the case, wait one second and then re-work again.
	// Well, except when someone tells us to stop... in that case, just stop, man
	wait.Until(npc.work, time.Second, npc.stopCh)
}

// work gets the item from the queue and attempts to process it.
func (npc *K8sNetworkPolicyController) work() {
	l := log.New().WithFields(log.Fields{"by": KNPC, "method": "work()"})
	for {
		// Get the item's key from the queue
		_event, quit := npc.queue.Get()
		if quit {
			l.Infoln("Quit requested... worker going to exit.")
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
			} else if npc.queue.NumRequeues(_event) < npc.maxRetries {
				// Tried less than the maximum retries?
				l.Warningf("Error processing item with key %s (will retry): %v", key, err)
				npc.queue.AddRateLimited(_event)
			} else {
				// Too many retries?
				l.Errorf("Error processing %s (giving up): %v", key, err)
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
func (npc *K8sNetworkPolicyController) processPolicy(event pcn_types.Event) error {
	l := log.New().WithFields(log.Fields{"by": KNPC, "method": "processPolicy()"})

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
		l.Infof("Retrieved policy %s in its current state", policy.Name)
	}
	if event.OldObject != nil {
		_prev, ok := event.OldObject.(*networking_v1.NetworkPolicy)
		if !ok {
			l.Errorln("could not get previous state")
			return fmt.Errorf("could not get previous state")
		}
		prev = _prev
		l.Infof("Retrieved policy %s in its previous state", prev.Name)
	}

	return nil
}

func (npc *K8sNetworkPolicyController) retrievePolicyFromCache(obj interface{}, key string) (*networking_v1.NetworkPolicy, error) {
	l := log.New().WithFields(log.Fields{"by": KNPC, "method": "retrievePolicyFromCache()"})

	// Get the policy by querying the key that kubernetes has assigned to it
	_policy, _, err := npc.k8sNetworkPoliciesInformer.GetIndexer().GetByKey(key)

	// Errors?
	if err != nil {
		l.Errorf("An error occurred: cannot find cache element with key %s from store %v", key, err)
		return nil, fmt.Errorf("An error occurred: cannot find cache element with key %s from ", key)
	}

	// Get the policy or try to recover it.
	policy, ok := _policy.(*networking_v1.NetworkPolicy)
	if !ok {
		policy, ok = obj.(*networking_v1.NetworkPolicy)
		if !ok {
			tombstone, ok := obj.(cache.DeletedFinalStateUnknown)
			if !ok {
				l.Errorln("error decoding object, invalid type")
				utilruntime.HandleError(fmt.Errorf("error decoding object, invalid type"))
				return nil, fmt.Errorf("error decoding object, invalid type")
			}
			policy, ok = tombstone.Obj.(*networking_v1.NetworkPolicy)
			if !ok {
				l.Errorln("error decoding object tombstone, invalid type")
				utilruntime.HandleError(fmt.Errorf("error decoding object tombstone, invalid type"))
				return nil, fmt.Errorf("error decoding object tombstone, invalid type")
			}
			l.Infof("Recovered deleted object '%s' from tombstone", policy.GetName())
		}
	}

	return policy, nil
}

// Stop stops the controller
func (npc *K8sNetworkPolicyController) Stop() {
	l := log.New().WithFields(log.Fields{"by": KNPC, "method": "Stop()"})

	// Make them know that exit has been requested
	close(npc.stopCh)

	// Shutdown the queue, making the worker unblock
	npc.queue.ShutDown()

	// Clean up the dispatchers
	npc.dispatchers.new.CleanUp()
	npc.dispatchers.update.CleanUp()
	npc.dispatchers.delete.CleanUp()

	l.Infoln("K8s network policy controller exited.")
}
