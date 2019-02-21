package controllers

import (
	//	TODO-ON-MERGE: change the path to polycube
	"fmt"
	"strings"
	"sync"
	"time"

	pcn_types "github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/types"

	"crypto/sha256"

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

type PodController interface {
	Run()
	Stop()
	Subscribe(pcn_types.EventType, func(*core_v1.Pod)) (func(), error)

	GetPods(pcn_types.PodQuery) ([]pcn_types.Pod, error)
}

type PcnPodController struct {
	nodeName string

	//clientset *kubernetes.Clientset

	queue workqueue.RateLimitingInterface

	informer cache.SharedIndexInformer

	startedOn time.Time

	dispatchers eventDispatchers

	stopCh chan struct{}

	maxRetries int

	logBy string

	pods map[string]podStore
	lock sync.Mutex

	//	TODO: map of pods by labels?
}

type podStore struct {
	pod          *pcn_types.Pod
	hashedLabels int
}

func NewPodController(nodeName string, clientset *kubernetes.Clientset) PodController {

	//	Init here
	//	TODO: make maxRetries settable on parameters?
	logBy := "PcnPodController"
	maxRetries := 5

	//	Let them know we're starting
	log.SetLevel(log.DebugLevel)
	var l = log.WithFields(log.Fields{
		"by":     logBy,
		"method": "NewPodController()",
	})

	l.Infof("Pod Controller called on node %s", nodeName)

	//------------------------------------------------
	//	Set up the Pod Controller
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
			log.WithFields(log.Fields{
				"by":     logBy,
				"method": "AddFunc()",
			}).Infof("Something has been added! Workspaces is %s", key)

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
			log.WithFields(log.Fields{
				"by":     logBy,
				"method": "UpdateFunc()",
			}).Info("Something has been updated!")

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
			log.WithFields(log.Fields{
				"by":     logBy,
				"method": "DeleteFunc()",
			}).Info("Something has been deleted!")

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

	dispatchers := eventDispatchers{
		new:    NewEventDispatcher("new-pod-event-dispatcher"),
		update: NewEventDispatcher("update-pod-event-dispatcher"),
		delete: NewEventDispatcher("delete-pod-event-dispatcher"),
	}

	//	Everything set up, return the controller
	return &PcnPodController{
		nodeName: nodeName,
		//clientset: clientset,
		queue:       queue,
		informer:    informer,
		dispatchers: dispatchers,
		logBy:       logBy,
		maxRetries:  maxRetries,
		stopCh:      make(chan struct{}),
		pods:        map[string]podStore{},
	}
}

func (p *PcnPodController) Run() {

	var l = log.WithFields(log.Fields{
		"by":     p.logBy,
		"method": "Start()",
	})

	l.Info("Network Policy Controller starting")

	//	Don't let panics crash the process
	defer utilruntime.HandleCrash()

	//	Record when we started, it is going to be used later
	p.startedOn = time.Now().UTC()

	//	Let's go!
	go p.informer.Run(p.stopCh)

	//	Make sure the cache is populated
	if !cache.WaitForCacheSync(p.stopCh, p.informer.HasSynced) {
		utilruntime.HandleError(fmt.Errorf("Timed out waiting for caches to sync"))
		return
	}

	//	Work *until* something bad happens. If that's the case, wait one second and then re-work again.
	//	Well, except when someone tells us to stop... in that case, just stop, man
	wait.Until(p.work, time.Second, p.stopCh)
	l.Infoln("Run method exiting...")
}

func (p *PcnPodController) work() {

	var stop = false
	var l = log.WithFields(log.Fields{
		"by":     p.logBy,
		"method": "work()",
	})

	l.Info("Starting method work...")

	for !stop {

		l.Infof("Ok, I'm going to get a new item from the queue...")

		//	Get the item's key from the queue
		_event, quit := p.queue.Get()

		if quit {
			l.Infoln("Quit requested... worker going to exit.")
			return
		}

		event := _event.(pcn_types.Event)

		l.Infof("Just got the item: its key is %s on namespace %s", event.Key, event.Namespace)

		err := p.process(event)

		//	No errors?
		if err == nil {
			//	Then reset the ratelimit counters
			p.queue.Forget(_event)
			l.Infof("Item with key %s has been forgotten from the queue", event.Key)
		} else if p.queue.NumRequeues(_event) < p.maxRetries {
			//	Tried less than the maximum retries?
			l.Warningf("Error processing item with key %s (will retry): %v", event.Key, err)
			p.queue.AddRateLimited(_event)
		} else {
			//	Too many retries?
			l.Errorf("Error processing %s (giving up): %v", event.Key, err)
			p.queue.Forget(_event)
			utilruntime.HandleError(err)
		}

		stop = quit
	}
}

func (p *PcnPodController) process(event pcn_types.Event) error {

	var l = log.WithFields(log.Fields{
		"by":     p.logBy,
		"method": "processPolicy()",
	})
	var pod *core_v1.Pod

	defer p.queue.Done(event)

	l.Infof("Starting to process pod")

	//	Get the policy by querying the key that kubernetes has assigned to this in its cache
	_pod, exists, err := p.informer.GetIndexer().GetByKey(event.Key)

	//	Errors?
	if err != nil {
		l.Errorf("An error occurred: cannot find cache element with key %s from store %v", event.Key, err)

		//	TODO: check this
		return fmt.Errorf("An error occurred: cannot find cache element with key %s from ", event.Key)
	}

	//	Get the pod
	if _pod != nil {
		pod = _pod.(*core_v1.Pod)
	}

	//-------------------------------------
	//	Dispatch the event
	//-------------------------------------

	switch event.Type {

	case pcn_types.New:
		p.addNewPod(pod)
		p.dispatchers.new.Dispatch(pod)
	case pcn_types.Update:
		p.removePod(pod)
		p.addNewPod(pod)
		p.dispatchers.update.Dispatch(pod)
		/*case pcn_types.Delete:
		p.removePod(pod)
		p.dispatchers.delete.Dispatch(pod)*/
	}

	//	Does not exist?
	if !exists {
		l.Infof("Object with key %s does not exist. Going to trigger a onDelete function", event.Key)
	}

	return nil
}

func (p *PcnPodController) addNewPod(pod *core_v1.Pod) {
	//	First calculate its labels
	var _labels string
	for key, val := range pod.Labels {
		_labels += key + ":" + val + ";"
	}
	sha := sha256.New()
	sha.Write([]byte(_labels))
	labels, _ := fmt.Printf("%x", sha.Sum(nil))

	p.lock.Lock()
	defer p.lock.Unlock()

	//	Add it in the main map
	p.pods[pod.Name] = podStore{
		pod: &pcn_types.Pod{
			Pod:  *pod,
			Veth: "",
		},
		hashedLabels: labels,
	}
}

func (p *PcnPodController) removePod(pod *core_v1.Pod) {
	p.lock.Lock()
	defer p.lock.Unlock()

	_, exists := p.pods[pod.Name]
	if exists {
		delete(p.pods, pod.Name)
	}
}

func (p *PcnPodController) Stop() {

	log.WithFields(log.Fields{
		"by":     p.logBy,
		"method": "Stop())",
	}).Info("Default network policy controller going to shutdown!")

	//	Make them know that exit has been requested
	close(p.stopCh)

	//	Shutdown the queue, making the worker unblock
	p.queue.ShutDown()

	//	Clean up the dispatchers
	p.dispatchers.new.CleanUp()
	p.dispatchers.update.CleanUp()
	p.dispatchers.delete.CleanUp()
}

/*Subscribe executes the function consumer when the event event is triggered. It returns an error if the event type does not exist.
It returns a function to call when you want to stop tracking that event.*/
func (p *PcnPodController) Subscribe(event pcn_types.EventType, consumer func(*core_v1.Pod)) (func(), error) {

	//	Prepare the function to be executed
	consumerFunc := (func(item interface{}) {

		//	First, cast the item to a network policy, so that the consumer will receive exactly what it wants...
		pod := item.(*core_v1.Pod)

		//	Then, execute the consumer in a separate thread.
		//	NOTE: this step can also be done in the event dispatcher, but I want it to make them oblivious of the type they're handling.
		//	This way, the event dispatcher is as general as possible (also, it is not their concern to cast objects.)
		go consumer(pod)
	})

	//	What event are you subscribing to?
	switch event {

	//-------------------------------------
	//	New event
	//-------------------------------------

	case pcn_types.New:
		id := p.dispatchers.new.Add(consumerFunc)

		return func() {
			p.dispatchers.new.Remove(id)
		}, nil

	//-------------------------------------
	//	Update event
	//-------------------------------------

	case pcn_types.Update:
		id := p.dispatchers.update.Add(consumerFunc)

		return func() {
			p.dispatchers.update.Remove(id)
		}, nil

	//-------------------------------------
	//	Delete Event
	//-------------------------------------

	case pcn_types.Delete:
		id := p.dispatchers.delete.Add(consumerFunc)

		return func() {
			p.dispatchers.delete.Remove(id)
		}, nil

	//-------------------------------------
	//	Undefined event
	//-------------------------------------

	default:
		return nil, fmt.Errorf("Undefined event type")
	}

}

func (p *PcnPodController) GetPods(query pcn_types.PodQuery) ([]pcn_types.Pod, error) {

	//allNamespaces := true

	result := []pcn_types.Pod{}

	if strings.ToLower(query.Pod.By) == "name" {
		//	If we query by name, we don't need the namespace...
		if query.Pod.Name == "*" {
			p.lock.Lock()
			for _, pod := range p.pods {
				result = append(result, *pod.pod)
			}
			p.lock.Unlock()
			return result, nil
		}

		//	Get the pod with that name
		if pod, exists := p.pods[query.Pod.Name]; exists {
			result = append(result, *pod.pod)
		}

		return result, nil
	}

	if strings.ToLower(query.Pod.By) == "labels" {

		var _labels string
		for key, val := range query.Pod.Labels {
			_labels += key + ":" + val + ";"
		}
		sha := sha256.New()
		sha.Write([]byte(_labels))
		labels, _ := fmt.Printf("%x", sha.Sum(nil))

		podsFound := []pcn_types.Pod{}
		p.lock.Lock()
		for _, currentPod := range p.pods {
			if currentPod.hashedLabels == labels {
				if query.Namespace.By == "name" && (query.Namespace.Name == "*" || query.Namespace.Name == currentPod.pod.Pod.Namespace) {
					log.Debugln("namespace is", query.Namespace.Name, "found:", currentPod.pod.Pod.Name, "on", currentPod.pod.Pod.Namespace)
					podsFound = append(podsFound, *currentPod.pod)
				}
			}
		}
		p.lock.Unlock()
		return podsFound, nil
	}

	return nil, nil
}
