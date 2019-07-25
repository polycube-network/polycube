package controllers

import (
	"errors"
	"fmt"
	"strings"
	"sync"
	"time"

	"github.com/polycube-network/polycube/src/components/k8s/utils"

	pcn_types "github.com/polycube-network/polycube/src/components/k8s/pcn_k8s/types"
	log "github.com/sirupsen/logrus"
	core_v1 "k8s.io/api/core/v1"
	meta_v1 "k8s.io/apimachinery/pkg/apis/meta/v1"
	"k8s.io/apimachinery/pkg/runtime"
	utilruntime "k8s.io/apimachinery/pkg/util/runtime"
	"k8s.io/apimachinery/pkg/util/wait"
	"k8s.io/apimachinery/pkg/watch"
	"k8s.io/client-go/kubernetes"
	"k8s.io/client-go/tools/cache"
	workqueue "k8s.io/client-go/util/workqueue"
)

// PodController is the interface of the pod controller
type PodController interface {
	// Run starts the pod controller
	Run()
	// Stop will stop the pod controller
	Stop()
	// Subscribe executes the function consumer when the event event is triggered.
	// It returns an error if the event type does not exist.
	// It returns a function to call when you want to stop tracking that event.
	Subscribe(event pcn_types.EventType, podspec, namespace, node *pcn_types.ObjectQuery, phase core_v1.PodPhase, consumer func(*core_v1.Pod, *core_v1.Pod)) (func(), error)
	// GetPods gets pod according to a specific pod query and a namespace query
	GetPods(queryPod, queryNs, queryNode *pcn_types.ObjectQuery) ([]core_v1.Pod, error)
}

// PcnPodController is the implementation of the pod controller
type PcnPodController struct {
	nsController NamespaceController
	clientset    kubernetes.Interface
	queue        workqueue.RateLimitingInterface
	informer     cache.SharedIndexInformer
	startedOn    time.Time
	dispatchers  EventDispatchersContainer
	stopCh       chan struct{}
	maxRetries   int
	lock         sync.Mutex
}

// NewPodController will start a new pod controller
func NewPodController(clientset kubernetes.Interface, nsController NamespaceController) PodController {
	maxRetries := 5

	//------------------------------------------------
	// Set up the Pod Controller
	//------------------------------------------------

	informer := cache.NewSharedIndexInformer(&cache.ListWatch{
		ListFunc: func(options meta_v1.ListOptions) (runtime.Object, error) {
			return clientset.CoreV1().Pods(meta_v1.NamespaceAll).List(options)
		},
		WatchFunc: func(options meta_v1.ListOptions) (watch.Interface, error) {
			return clientset.CoreV1().Pods(meta_v1.NamespaceAll).Watch(options)
		},
	},
		&core_v1.Pod{},
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
	return &PcnPodController{
		nsController: nsController,
		clientset:    clientset,
		queue:        queue,
		informer:     informer,
		dispatchers:  dispatchers,
		maxRetries:   maxRetries,
		stopCh:       make(chan struct{}),
	}
}

// Run starts the pod controller
func (p *PcnPodController) Run() {
	l := log.New().WithFields(log.Fields{"by": PC, "method": "Run()"})

	// Don't let panics crash the process
	defer utilruntime.HandleCrash()

	// Record when we started, it is going to be used later
	p.startedOn = time.Now().UTC()

	// Let's go!
	go p.informer.Run(p.stopCh)

	// Make sure the cache is populated
	if !cache.WaitForCacheSync(p.stopCh, p.informer.HasSynced) {
		utilruntime.HandleError(fmt.Errorf("Timed out waiting for caches to sync"))
		return
	}

	l.Infoln("Started...")

	// Work *until* something bad happens.
	// If that's the case, wait one second and then re-work again.
	// Well, except when someone tells us to stop... in that case, just stop, man
	wait.Until(p.work, time.Second, p.stopCh)
}

// work gets the item from the queue and attempts to process it
func (p *PcnPodController) work() {
	l := log.New().WithFields(log.Fields{"by": PC, "method": "work()"})

	for {
		// Get the item's key from the queue
		_event, quit := p.queue.Get()

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
			err := p.process(event)

			// No errors?
			if err == nil {
				// Then reset the ratelimit counters
				p.queue.Forget(_event)
			} else if p.queue.NumRequeues(_event) < p.maxRetries {
				// Tried less than the maximum retries?
				l.Warningf("Error processing item with key %s (will retry): %v", key, err)
				p.queue.AddRateLimited(_event)
			} else {
				// Too many retries?
				l.Errorf("Error processing %s (giving up): %v", key, err)
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

// process will process the event and dispatch the pod event
func (p *PcnPodController) process(event pcn_types.Event) error {
	l := log.New().WithFields(log.Fields{"by": PC, "method": "process()"})

	var pod *core_v1.Pod
	var prev *core_v1.Pod
	var err error
	defer p.queue.Done(event)

	//-------------------------------------
	//	Retrieve the pod from cache
	//-------------------------------------

	if event.NewObject != nil {
		pod, err = p.retrievePodFromCache(event.NewObject, event.NewKey)
		if err != nil {
			return err
		}
	}
	if event.OldObject != nil {
		_prev, ok := event.OldObject.(*core_v1.Pod)
		if !ok {
			l.Errorln("could not get previous state")
			return fmt.Errorf("could not get previous state")
		}
		prev = _prev
	}

	//-------------------------------------
	// Dispatch the event
	//-------------------------------------

	switch event.Type {
	case pcn_types.New:
		p.dispatchers.new.Dispatch(pod, nil)
	case pcn_types.Update:
		p.dispatchers.update.Dispatch(pod, prev)
	case pcn_types.Delete:
		p.dispatchers.delete.Dispatch(nil, pod)
	}

	return nil
}

// retrievePodFromCache retrieves the namespace from the cache.
// It tries to recover it from the tombstone if deleted.
func (p *PcnPodController) retrievePodFromCache(obj interface{}, key string) (*core_v1.Pod, error) {
	l := log.New().WithFields(log.Fields{"by": PC, "method": "retrievePodFromCache()"})

	// Get the pod by querying the key that kubernetes has assigned to it
	_pod, _, err := p.informer.GetIndexer().GetByKey(key)

	// Errors?
	if err != nil {
		l.Errorf("An error occurred: cannot find cache element with key %s from store %v", key, err)
		return nil, fmt.Errorf("An error occurred: cannot find cache element with key %s from ", key)
	}

	// Get the pod or try to recover it.
	pod, ok := _pod.(*core_v1.Pod)
	if !ok {
		pod, ok = obj.(*core_v1.Pod)
		if !ok {
			tombstone, ok := obj.(cache.DeletedFinalStateUnknown)
			if !ok {
				l.Errorln("error decoding object, invalid type")
				utilruntime.HandleError(fmt.Errorf("error decoding object, invalid type"))
				return nil, fmt.Errorf("error decoding object, invalid type")
			}
			pod, ok = tombstone.Obj.(*core_v1.Pod)
			if !ok {
				l.Errorln("error decoding object tombstone, invalid type")
				utilruntime.HandleError(fmt.Errorf("error decoding object tombstone, invalid type"))
				return nil, fmt.Errorf("error decoding object tombstone, invalid type")
			}
			l.Infof("Recovered deleted object '%s' from tombstone", pod.GetName())
		}
	}

	return pod, nil
}

// Stop will stop the pod controller
func (p *PcnPodController) Stop() {
	l := log.New().WithFields(log.Fields{"by": PC, "method": "Stop()"})

	// Make them know that exit has been requested
	close(p.stopCh)

	// Shutdown the queue, making the worker unblock
	p.queue.ShutDown()

	// Clean up the dispatchers
	p.dispatchers.new.CleanUp()
	p.dispatchers.update.CleanUp()
	p.dispatchers.delete.CleanUp()

	l.Infoln("Stopped.")
}

// Subscribe executes the function consumer when the event event is triggered.
// It returns an error if the event type does not exist.
// It returns a function to call when you want to stop tracking that event.
func (p *PcnPodController) Subscribe(event pcn_types.EventType, podspec, namespace, node *pcn_types.ObjectQuery, phase core_v1.PodPhase, consumer func(*core_v1.Pod, *core_v1.Pod)) (func(), error) {
	// Prepare the function to be executed
	consumerFunc := (func(current, prev interface{}) {
		//------------------------------------------
		// Init
		//------------------------------------------
		var currentState *core_v1.Pod
		var prevState *core_v1.Pod

		// First, cast the items to a pod, so that the consumer will receive
		// exactly what it wants...
		if current != nil {
			currentState = current.(*core_v1.Pod)
		}
		if prev != nil {
			prevState = prev.(*core_v1.Pod)
		}

		//------------------------------------------
		// Check for pod's criteria
		//------------------------------------------

		// First, check for its current state OR its previous one.
		// NOTE: if this is an UPDATE and the two differ, this could be the
		// last time this event is triggered for this pod, i.e. if the phase
		// or the labels changed.
		if !p.podMeetsCriteria(currentState, podspec, namespace, node, phase) && !p.podMeetsCriteria(prevState, podspec, namespace, node, phase) {
			return
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

// podMeetsCriteria is called before dispatching the event,
// to verify if the pod should be dispatched or not
func (p *PcnPodController) podMeetsCriteria(pod *core_v1.Pod, podSpec, nsSpec, nodeSpec *pcn_types.ObjectQuery, phase core_v1.PodPhase) bool {

	// This is actually useless but who knows....
	if pod == nil {
		return false
	}

	//-------------------------------------
	// The node
	//-------------------------------------
	if nodeSpec != nil && pod.Spec.NodeName != nodeSpec.Name {
		return false
	}

	//-------------------------------------
	// The phase
	//-------------------------------------
	if phase != pcn_types.PodAnyPhase {

		if phase != pcn_types.PodTerminating {
			// I don't want terminating pods.
			if pod.ObjectMeta.DeletionTimestamp != nil {
				// The pod is terminating.
				return false
			}

			if pod.Status.Phase != phase {
				// The pod is not in the phase I want
				return false
			}
		} else {
			// I want terminating pods
			if pod.ObjectMeta.DeletionTimestamp == nil {
				// The pod is not terminating
				return false
			}
		}
	}

	//-------------------------------------
	// The namespace
	//-------------------------------------
	if nsSpec != nil {
		//	Check the namespace name
		if len(nsSpec.Name) > 0 {
			if nsSpec.Name != pod.Namespace {
				return false
			}
		} else {
			// Without this else we would be checking ns labels even if
			// name check was successful
			// Check the labels of the namespace
			if len(nsSpec.Labels) > 0 {
				// Get the list
				nsList, err := p.nsController.GetNamespaces(&pcn_types.ObjectQuery{
					By:     "labels",
					Labels: nsSpec.Labels,
				})
				if err != nil {
					return false
				}

				// Among all the namespaces found, check if the pod's is one
				found := false
				for _, n := range nsList {
					if n.Name == pod.Namespace {
						found = true
						break
					}
				}
				if !found {
					return false
				}
			} else {
				return false
			}
		}
	}

	//-------------------------------------
	// The Pod Labels
	//-------------------------------------
	// Check the labels: if this pod does not contain all the labels
	// I am interested in, then stop right here.
	// It should be very rare to see pods with more than 5 labels...
	if podSpec != nil && len(podSpec.Labels) > 0 {
		if !utils.AreLabelsContained(podSpec.Labels, pod.Labels) {
			return false
		}
		/*labelsFound := 0
		labelsToFind := len(podSpec.Labels)

		for neededKey, neededValue := range podSpec.Labels {
			if value, exists := pod.Labels[neededKey]; exists && value == neededValue {
				labelsFound++
				if labelsFound == labelsToFind {
					break
				}
			} else {
				// I didn't find this key or the value wasn't the one I wanted:
				// it's pointless to go on checking the other labels.
				break
			}
		}

		// Did we find all labels we needed?
		if labelsFound != labelsToFind {
			return false
		}*/
	}

	return true
}

// GetPods gets pod according to a specific pod query and a namespace query
func (p *PcnPodController) GetPods(queryPod, queryNs, queryNode *pcn_types.ObjectQuery) ([]core_v1.Pod, error) {
	// The namespaces the pods must be found on
	// If this stays empty it means that I don't care about the namespace they are in
	ns := map[string]bool{}

	//------------------------------------------------
	// Preliminary checks
	//------------------------------------------------
	// -- The namespace
	nsList, err := p.nsController.GetNamespaces(queryNs)
	if err != nil {
		return []core_v1.Pod{}, err
	}
	if len(nsList) == 0 {
		// If no namespace is found, it is useless to go on searching for pods
		return []core_v1.Pod{}, nil
	}
	for _, n := range nsList {
		ns[n.Name] = true
	}

	// -- The node
	if queryNode != nil {
		if queryNode.By != "name" {
			return []core_v1.Pod{}, fmt.Errorf("node query type not supported")
		}
		if len(queryNode.Name) == 0 {
			return []core_v1.Pod{}, fmt.Errorf("node name not entered")
		}
	}

	// BRIEF: from now on, this function has been split in mini-functions
	// inside it for sack of clarity.
	// Jump at the bottom of it to understand it better.

	//------------------------------------------------
	// Filter the results
	//------------------------------------------------

	getAndFilter := func(listOptions meta_v1.ListOptions) ([]core_v1.Pod, error) {
		list := []core_v1.Pod{}

		// Do I care or not about the namespace?
		// If not, I'll put the NamespaceAll inside the map as its only value
		if len(ns) == 0 {
			ns[meta_v1.NamespaceAll] = true
		}

		// Loop through all interested namespaces
		for namespace := range ns {
			lister, err := p.clientset.CoreV1().Pods(namespace).List(listOptions)
			if err == nil {
				for _, currentPod := range lister.Items {
					if queryNode == nil || (queryNode != nil && currentPod.Spec.NodeName == queryNode.Name) {
						list = append(list, currentPod)
					}
				}
			} else {
				//return []core_v1.Pod, err
				// Just skip this namespace.
			}
		}
		return list, nil
	}

	//-------------------------------------
	// Find by name
	//-------------------------------------

	byName := func(name string) ([]core_v1.Pod, error) {
		if len(name) == 0 {
			return []core_v1.Pod{}, errors.New("Pod name not provided")
		}

		return getAndFilter(meta_v1.ListOptions{
			FieldSelector: "metadata.name=" + name,
		})
	}

	//-------------------------------------
	// Find by labels
	//-------------------------------------

	byLabels := func(labels map[string]string) ([]core_v1.Pod, error) {
		if labels == nil {
			return []core_v1.Pod{}, errors.New("Pod labels is nil")
		}
		if len(labels) == 0 {
			// If you need to get all pods, use get by name and name *
			return []core_v1.Pod{}, errors.New("No pod labels provided")
		}

		listOptions := meta_v1.ListOptions{
			LabelSelector: utils.ImplodeLabels(labels, ",", false),
		}

		return getAndFilter(listOptions)
	}

	//-------------------------------------
	// Find 'em
	//-------------------------------------

	if queryPod == nil {
		return getAndFilter(meta_v1.ListOptions{})
	}

	switch strings.ToLower(queryPod.By) {
	case "name":
		return byName(queryPod.Name)
	case "labels":
		return byLabels(queryPod.Labels)
	default:
		return []core_v1.Pod{}, errors.New("Unrecognized pod query")
	}
}
