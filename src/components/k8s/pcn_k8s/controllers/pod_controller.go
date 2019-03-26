package controllers

import (
	//	TODO-ON-MERGE: change the path to polycube
	"errors"
	"fmt"
	"strings"
	"sync"
	"time"

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

type PodController interface {
	Run()
	Stop()
	Subscribe(pcn_types.Event, func(*core_v1.Pod)) (func(), error)

	GetPods(pcn_types.ObjectQuery, pcn_types.ObjectQuery) ([]core_v1.Pod, error)
}

type PcnPodController struct {
	nodeName     string
	nsController NamespaceController
	clientset    *kubernetes.Clientset
	queue        workqueue.RateLimitingInterface
	informer     cache.SharedIndexInformer
	startedOn    time.Time
	dispatchers  EventDispatchersContainer
	stopCh       chan struct{}
	maxRetries   int
	logBy        string
	pods         map[string]*pcn_types.Pod
	lock         sync.Mutex
}

func NewPodController(nodeName string, clientset *kubernetes.Clientset, nsController NamespaceController) PodController {

	//	Init here
	//	TODO: make maxRetries settable on parameters?
	logBy := "PcnPodController"
	maxRetries := 5

	//	Let them know we're starting
	/*log.SetLevel(log.DebugLevel)
	var l = log.WithFields(log.Fields{
		"by":     logBy,
		"method": "NewPodController()",
	})*/

	//l.Infof("Pod Controller called on node %s", nodeName)

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
		new:    NewEventDispatcher("new-pod-event-dispatcher"),
		update: NewEventDispatcher("update-pod-event-dispatcher"),
		delete: NewEventDispatcher("delete-pod-event-dispatcher"),
	}

	//	Everything set up, return the controller
	return &PcnPodController{
		nodeName:     nodeName,
		nsController: nsController,
		clientset:    clientset,
		queue:        queue,
		informer:     informer,
		dispatchers:  dispatchers,
		logBy:        logBy,
		maxRetries:   maxRetries,
		stopCh:       make(chan struct{}),
		pods:         map[string]*pcn_types.Pod{},
	}
}

func (p *PcnPodController) Run() {

	var l = log.WithFields(log.Fields{
		"by":     p.logBy,
		"method": "Start()",
	})

	//l.Info("Pod Controller starting")

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

	l.Infoln("Pod controller started.")

	//	Work *until* something bad happens. If that's the case, wait one second and then re-work again.
	//	Well, except when someone tells us to stop... in that case, just stop, man
	wait.Until(p.work, time.Second, p.stopCh)
	//l.Infoln("Run method exiting...")
}

func (p *PcnPodController) work() {

	var stop = false
	var l = log.WithFields(log.Fields{
		"by":     p.logBy,
		"method": "work()",
	})

	//l.Info("Starting method work...")

	for !stop {

		//l.Infof("Ok, I'm going to get a new item from the queue...")

		//	Get the item's key from the queue
		_event, quit := p.queue.Get()

		if quit {
			//l.Infoln("Quit requested... worker going to exit.")
			return
		}

		event := _event.(pcn_types.Event)

		//l.Infof("Just got the item: its key is %s on namespace %s", event.Key, event.Namespace)

		err := p.process(event)

		//	No errors?
		if err == nil {
			//	Then reset the ratelimit counters
			p.queue.Forget(_event)
			//l.Infof("Item with key %s has been forgotten from the queue", event.Key)
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

	//l.Infof("Starting to process pod")

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
		//p.removePod(pod)
		p.addNewPod(pod)
		p.dispatchers.update.Dispatch(pod)
	case pcn_types.Delete:
		splitted := strings.Split(event.Key, "/")
		pod, ok := p.pods[splitted[1]]
		if ok {
			p.dispatchers.delete.Dispatch(&pod.Pod)
			p.removePod(pod.Pod)
		}
	}

	//	Does not exist?
	if !exists {
		//l.Infof("Object with key %s does not exist. Going to trigger a onDelete function", event.Key)
	}

	return nil
}

func (p *PcnPodController) addNewPod(pod *core_v1.Pod) {

	p.lock.Lock()
	defer p.lock.Unlock()

	podContainer := &pcn_types.Pod{
		Pod:  pod,
		Veth: "",
	}

	//	Add it in the main map
	p.pods[pod.Name] = podContainer
}

func (p *PcnPodController) removePod(pod *core_v1.Pod) {
	p.lock.Lock()
	defer p.lock.Unlock()

	_, exists := p.pods[pod.Name]
	if exists {
		delete(p.pods, pod.Name)
	}
}

func (p *PcnPodController) implodeLabels(labels map[string]string) string {
	implodedLabels := ""

	for k, v := range labels {
		implodedLabels += k + "=" + v + ","
	}

	return strings.Trim(implodedLabels, ",")
}

func (p *PcnPodController) Stop() {

	log.WithFields(log.Fields{
		"by":     p.logBy,
		"method": "Stop())",
	}).Info("Namespace controller going to shutdown!")

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
func (p *PcnPodController) Subscribe(event pcn_types.Event, consumer func(*core_v1.Pod)) (func(), error) {

	//	Prepare the function to be executed
	consumerFunc := (func(item interface{}) {

		//	First, cast the item to a pod, so that the consumer will receive exactly what it wants...
		pod := item.(*core_v1.Pod)

		//	Check the namespace: if this pod belongs to a namespace I am not interested in, then stop right here.
		if len(event.Namespace) > 0 && pod.Namespace != event.Namespace {
			return
		}

		//	Check the labels: if this pod does not contain all the labels I am interested in, then stop right here.
		//	It should be very rare to see pods with more than 5 labels...
		if event.Labels != nil && len(event.Labels) > 0 {
			labelsFound := 0
			labelsToFind := len(event.Labels)

			for neededKey, neededValue := range event.Labels {
				if value, exists := pod.Labels[neededKey]; exists && value == neededValue {
					labelsFound++
					if labelsFound == labelsToFind {
						break
					}
				} else {
					//	I didn't find this key or the value wasn't the one I wanted: it's pointless to go on checking the other labels.
					break
				}
			}

			//	Did we find all labels we needed?
			if labelsFound != labelsToFind {
				return
			}
		}

		//	Then, execute the consumer in a separate thread.
		//	NOTE: this step can also be done in the event dispatcher, but I want it to make them oblivious of the type they're handling.
		//	This way, the event dispatcher is as general as possible (also, it is not their concern to cast objects.)
		go consumer(pod)
	})

	//	What event are you subscribing to?
	switch event.Type {

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

func (p *PcnPodController) GetPods(queryPod pcn_types.ObjectQuery, queryNs pcn_types.ObjectQuery) ([]core_v1.Pod, error) {

	//	The namespaces the pods must be found on
	ns := []string{}

	//	First get the namespace
	found, err := p.nsController.GetNamespaces(queryNs)
	if err != nil {
		return []core_v1.Pod{}, err
	}
	if len(found) < 1 {
		//	If no namespace is found, it is useless to go on searching for pods
		return []core_v1.Pod{}, nil
	}
	for _, n := range found {
		ns = append(ns, n.Name)
	}

	//	Query by name
	if strings.ToLower(queryPod.By) == "name" {
		//	If we query by name, we don't need the namespace...
		list := []core_v1.Pod{}
		if len(queryPod.Name) < 0 {
			return list, errors.New("Pod name not provided")
		}

		p.lock.Lock()
		defer p.lock.Unlock()

		if queryPod.Name == "*" {
			for _, pod := range p.pods {
				if len(ns) < 1 {
					list = append(list, *pod.Pod)
				}
				for _, namespace := range ns {
					if pod.Pod.Namespace == namespace {
						list = append(list, *pod.Pod)
					}
				}

			}
			return list, nil
		}

		//	Get the pod with that name
		if pod, exists := p.pods[queryPod.Name]; exists {
			return []core_v1.Pod{*pod.Pod}, nil
		}

		return []core_v1.Pod{}, nil
	}

	//	Query by labels
	if strings.ToLower(queryPod.By) == "labels" {

		if queryPod.Labels == nil {
			return []core_v1.Pod{}, errors.New("Pod labels is nil")
		}

		if len(queryPod.Labels) < 1 {
			return []core_v1.Pod{}, errors.New("No pod labels provided")
		}

		list := []core_v1.Pod{}
		labels := p.implodeLabels(queryPod.Labels)

		if len(ns) < 1 {
			ns = append(ns, meta_v1.NamespaceAll)
		}

		for _, namespace := range ns {

			if currentList, err := p.clientset.CoreV1().Pods(namespace).List(meta_v1.ListOptions{
				LabelSelector: labels,
			}); err != nil {
				log.Error("Error while trying to get pods with labels", labels, "on namespace", namespace)
			} else {
				list = append(list, currentList.Items...)
			}
		}

		return list, nil
	}

	return []core_v1.Pod{}, errors.New("Unrecognized pod query")
}
