package controllers

import (
	"errors"
	"fmt"
	"strings"
	"sync"
	"time"

	//	TODO-ON-MERGE: change the path to polycube
	pcn_types "github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/types"

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

type NamespaceController interface {
	Run()
	Stop()
	Subscribe(pcn_types.EventType, func(*core_v1.Namespace)) (func(), error)

	GetNamespaces(pcn_types.ObjectQuery) ([]core_v1.Namespace, error)
}

type PcnNamespaceController struct {
	nodeName    string
	clientset   *kubernetes.Clientset
	queue       workqueue.RateLimitingInterface
	informer    cache.SharedIndexInformer
	startedOn   time.Time
	dispatchers EventDispatchersContainer
	stopCh      chan struct{}
	maxRetries  int
	logBy       string
	namespaces  map[string]*core_v1.Namespace
	lock        sync.Mutex
}

func NewNsController(nodeName string, clientset *kubernetes.Clientset) NamespaceController {

	//	Init here
	//	TODO: make maxRetries settable on parameters?
	logBy := "PcnNamespaceController"
	maxRetries := 5

	//	Let them know we're starting
	/*log.SetLevel(log.DebugLevel)
	var l = log.WithFields(log.Fields{
		"by":     logBy,
		"method": "NewNsController()",
	})*/

	//l.Infof("Namespace Controller called on node %s", nodeName)

	//------------------------------------------------
	//	Set up the Namespace Controller
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
	//	Set up the queue
	//------------------------------------------------

	//	Start the queue
	queue := workqueue.NewRateLimitingQueue(workqueue.DefaultControllerRateLimiter())

	//------------------------------------------------
	//	Set up the event handlers
	//------------------------------------------------

	//	Whenever something happens to network policies, the event is routed by this event handler and routed to the queue. It'll know what to do.
	informer.AddEventHandler(cache.ResourceEventHandlerFuncs{
		AddFunc: func(obj interface{}) {
			key, err := cache.MetaNamespaceKeyFunc(obj)
			/*log.WithFields(log.Fields{
				"by":     logBy,
				"method": "AddFunc()",
			}).Infof("Something has been added! Workspaces is %s", key)*/

			//	Set up the event
			event := pcn_types.Event{
				Key:       key,
				Type:      pcn_types.New,
				Namespace: strings.Split(key, "/")[0],
			}

			//	Add this event to the queue
			if err == nil {
				queue.Add(event)
			}
		},
		UpdateFunc: func(old, new interface{}) {
			/*log.WithFields(log.Fields{
				"by":     logBy,
				"method": "UpdateFunc()",
			}).Info("Something has been updated!")*/

			key, err := cache.MetaNamespaceKeyFunc(new)

			//	Set up the event
			event := pcn_types.Event{
				Key:       key,
				Type:      pcn_types.Update,
				Namespace: strings.Split(key, "/")[0],
			}
			//	Add this event to the queue
			if err == nil {
				queue.Add(event)
			}
		},
		DeleteFunc: func(obj interface{}) {
			/*log.WithFields(log.Fields{
				"by":     logBy,
				"method": "DeleteFunc()",
			}).Info("Something has been deleted!")*/

			key, err := cache.MetaNamespaceKeyFunc(obj)

			//	Set up the event
			event := pcn_types.Event{
				Key:       key,
				Type:      pcn_types.Delete,
				Namespace: strings.Split(key, "/")[0],
			}
			//	Add this event to the queue
			if err == nil {
				queue.Add(event)
			}
		},
	})

	//------------------------------------------------
	//	Set up the dispatchers
	//------------------------------------------------

	dispatchers := EventDispatchersContainer{
		new:    NewEventDispatcher("new-ns-event-dispatcher"),
		update: NewEventDispatcher("update-ns-event-dispatcher"),
		delete: NewEventDispatcher("delete-ns-event-dispatcher"),
	}

	//	Everything set up, return the controller
	return &PcnNamespaceController{
		nodeName:    nodeName,
		clientset:   clientset,
		queue:       queue,
		informer:    informer,
		dispatchers: dispatchers,
		logBy:       logBy,
		maxRetries:  maxRetries,
		stopCh:      make(chan struct{}),
		namespaces:  map[string]*core_v1.Namespace{},
	}
}

func (n *PcnNamespaceController) Run() {

	var l = log.WithFields(log.Fields{
		"by":     n.logBy,
		"method": "Start()",
	})

	//	Don't let panics crash the process
	defer utilruntime.HandleCrash()

	//	Record when we started, it is going to be used later
	n.startedOn = time.Now().UTC()

	//	Let's go!
	go n.informer.Run(n.stopCh)

	//	Make sure the cache is populated
	if !cache.WaitForCacheSync(n.stopCh, n.informer.HasSynced) {
		utilruntime.HandleError(fmt.Errorf("Timed out waiting for caches to sync"))
		return
	}

	l.Infoln("Namespace controller started.")

	//	Work *until* something bad happens. If that's the case, wait one second and then re-work again.
	//	Well, except when someone tells us to stop... in that case, just stop, man
	wait.Until(n.work, time.Second, n.stopCh)
	//l.Infoln("Run method exiting...")
}

func (n *PcnNamespaceController) work() {

	var stop = false
	var l = log.WithFields(log.Fields{
		"by":     n.logBy,
		"method": "work()",
	})

	//l.Info("Starting method work...")

	for !stop {

		//l.Infof("Ok, I'm going to get a new item from the queue...")

		//	Get the item's key from the queue
		_event, quit := n.queue.Get()

		if quit {
			l.Infoln("Quit requested... worker going to exit.")
			return
		}

		event := _event.(pcn_types.Event)

		//l.Infof("Just got the item: its key is %s on namespace %s", event.Key, event.Namespace)

		err := n.process(event)

		//	No errors?
		if err == nil {
			//	Then reset the ratelimit counters
			n.queue.Forget(_event)
			//l.Infof("Item with key %s has been forgotten from the queue", event.Key)
		} else if n.queue.NumRequeues(_event) < n.maxRetries {
			//	Tried less than the maximum retries?
			l.Warningf("Error processing item with key %s (will retry): %v", event.Key, err)
			n.queue.AddRateLimited(_event)
		} else {
			//	Too many retries?
			l.Errorf("Error processing %s (giving up): %v", event.Key, err)
			n.queue.Forget(_event)
			utilruntime.HandleError(err)
		}

		stop = quit
	}
}

func (n *PcnNamespaceController) process(event pcn_types.Event) error {

	var l = log.WithFields(log.Fields{
		"by":     n.logBy,
		"method": "processPolicy()",
	})
	var ns *core_v1.Namespace

	defer n.queue.Done(event)

	//l.Infof("Starting to process namespace")

	//	Get the policy by querying the key that kubernetes has assigned to this in its cache
	_ns, exists, err := n.informer.GetIndexer().GetByKey(event.Key)

	//	Errors?
	if err != nil {
		l.Errorf("An error occurred: cannot find cache element with key %s from store %v", event.Key, err)

		//	TODO: check this
		return fmt.Errorf("An error occurred: cannot find cache element with key %s from ", event.Key)
	}

	//	Get the ns
	if _ns != nil {
		ns = _ns.(*core_v1.Namespace)
	}

	//-------------------------------------
	//	Dispatch the event
	//-------------------------------------

	switch event.Type {

	case pcn_types.New:
		n.addNewNamespace(ns)
		n.dispatchers.new.Dispatch(ns)
	case pcn_types.Update:
		//n.removeNamespace(ns)
		n.addNewNamespace(ns)
		n.dispatchers.update.Dispatch(ns)
	case pcn_types.Delete:
		ns, ok := n.namespaces[event.Namespace]
		if ok {
			n.removeNamespace(ns)
			n.dispatchers.delete.Dispatch(ns)
		}
	}

	//	Does not exist?
	if !exists {
		//l.Infof("Object with key %s does not exist. Going to trigger a onDelete function", event.Key)
	}

	return nil
}

func (n *PcnNamespaceController) addNewNamespace(ns *core_v1.Namespace) {

	n.lock.Lock()
	defer n.lock.Unlock()

	//	Add it in the main map
	n.namespaces[ns.Name] = ns
}

func (n *PcnNamespaceController) removeNamespace(ns *core_v1.Namespace) {
	n.lock.Lock()
	defer n.lock.Unlock()

	_, exists := n.namespaces[ns.Name]
	if exists {
		delete(n.namespaces, ns.Name)
	}
}

func (n *PcnNamespaceController) Stop() {

	l := log.WithFields(log.Fields{
		"by":     n.logBy,
		"method": "Stop())",
	})

	//	Make them know that exit has been requested
	close(n.stopCh)

	//	Shutdown the queue, making the worker unblock
	n.queue.ShutDown()

	//	Clean up the dispatchers
	n.dispatchers.new.CleanUp()
	n.dispatchers.update.CleanUp()
	n.dispatchers.delete.CleanUp()

	l.Infoln("Namespace controller stopped.")
}

/*Subscribe executes the function consumer when the event event is triggered. It returns an error if the event type does not exist.
It returns a function to call when you want to stop tracking that event.*/
func (n *PcnNamespaceController) Subscribe(event pcn_types.EventType, consumer func(*core_v1.Namespace)) (func(), error) {

	//	Prepare the function to be executed
	consumerFunc := (func(item interface{}) {

		//	First, cast the item to a namespace, so that the consumer will receive exactly what it wants...
		ns := item.(*core_v1.Namespace)

		//	Then, execute the consumer in a separate thread.
		//	NOTE: this step can also be done in the event dispatcher, but I want it to make them oblivious of the type they're handling.
		//	This way, the event dispatcher is as general as possible (also, it is not their concern to cast objects.)
		go consumer(ns)
	})

	//	What event are you subscribing to?
	switch event {

	//-------------------------------------
	//	New event
	//-------------------------------------

	case pcn_types.New:
		id := n.dispatchers.new.Add(consumerFunc)

		return func() {
			n.dispatchers.new.Remove(id)
		}, nil

	//-------------------------------------
	//	Update event
	//-------------------------------------

	case pcn_types.Update:
		id := n.dispatchers.update.Add(consumerFunc)

		return func() {
			n.dispatchers.update.Remove(id)
		}, nil

	//-------------------------------------
	//	Delete Event
	//-------------------------------------

	case pcn_types.Delete:
		id := n.dispatchers.delete.Add(consumerFunc)

		return func() {
			n.dispatchers.delete.Remove(id)
		}, nil

	//-------------------------------------
	//	Undefined event
	//-------------------------------------

	default:
		return nil, fmt.Errorf("Undefined event type")
	}
}

func (n *PcnNamespaceController) implodeLabels(labels map[string]string) string {
	implodedLabels := ""

	for k, v := range labels {
		implodedLabels += k + "=" + v + ","
	}

	return strings.Trim(implodedLabels, ",")
}

func (n *PcnNamespaceController) GetNamespaces(query pcn_types.ObjectQuery) ([]core_v1.Namespace, error) {

	//	Query by name
	if strings.ToLower(query.By) == "name" {

		if len(query.Name) < 1 {
			return []core_v1.Namespace{}, errors.New("Namespace name not provided")
		}

		n.lock.Lock()
		defer n.lock.Unlock()

		//	All namespaces?
		if query.Name == "*" {

			list := []core_v1.Namespace{}
			for _, ns := range n.namespaces {
				list = append(list, *ns)
			}

			return list, nil
		}

		//	Particular namespace?
		if ns, exists := n.namespaces[query.Name]; exists {
			return []core_v1.Namespace{*ns}, nil
		}

		return []core_v1.Namespace{}, nil
	}

	//	By labels
	if strings.ToLower(query.By) == "labels" {

		if query.Labels == nil {
			return []core_v1.Namespace{}, errors.New("Namespace labels is nil")
		}

		//	If you provide an empty map, I assume you want all namespaces with no labels
		//	To Do: how to do this? check later

		lister, err := n.clientset.CoreV1().Namespaces().List(meta_v1.ListOptions{
			LabelSelector: n.implodeLabels(query.Labels),
		})

		if err != nil {
			log.Errorln("Error occurred while getting namespaces:", err)
			return []core_v1.Namespace{}, errors.New("Error occurred while getting namespaces")
		}

		return lister.Items, nil
	}

	//	Unrecognized query
	return []core_v1.Namespace{}, errors.New("Unrecognized namespace query")
}
