package controllers

import (
	"fmt"
	"strings"
	"time"

	//	To check for protocol type
	v1 "k8s.io/api/core/v1"

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

type DefaultNetworkPolicyController struct {
	nodeName string

	clientset *kubernetes.Clientset

	queue workqueue.RateLimitingInterface

	defaultNetworkPoliciesInformer cache.SharedIndexInformer

	startedOn time.Time
}

var maxRetries = 5

func NewDefaultNetworkPolicyController(nodeName string, clientset *kubernetes.Clientset) *DefaultNetworkPolicyController {

	//	Let them know we're starting
	log.SetLevel(log.DebugLevel)
	var l = log.WithFields(log.Fields{
		"by":     "NPC",
		"method": "NewNetworkPolicyController()",
	})

	l.Infof("Network Policy Controller just called on node %s", nodeName)

	//------------------------------------------------
	//	Set up the default network policies informer
	//------------------------------------------------

	//	TODO: is there really a need for a *shared* informer?
	//	After all, I am the only one who's querying for network policies here.
	//	Different story for the pod informer (which I'm going to do later).
	//	So, yeah... remember to check this up.
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

	l.Info("Just set up the informer!")

	//------------------------------------------------
	//	Set up the queue
	//------------------------------------------------

	//	Start the queue
	queue := workqueue.NewRateLimitingQueue(workqueue.DefaultControllerRateLimiter())

	l.Info("Just set up the queue")

	//------------------------------------------------
	//	Set up the event handlers
	//------------------------------------------------

	//	Whenever something happens to network policies, the event is routed by this event handler and routed to the queue. It'll know what to do.
	npcInformer.AddEventHandler(cache.ResourceEventHandlerFuncs{
		AddFunc: func(obj interface{}) {
			key, err := cache.MetaNamespaceKeyFunc(obj)
			log.WithFields(log.Fields{
				"by":     "NPC",
				"method": "AddFunc()",
			}).Infof("Something has been added! Workspaces is %s", key)

			//	Add this key to the queue
			if err == nil {
				queue.Add(key)
			}
		},
		UpdateFunc: func(old, new interface{}) {
			/*newEvent.key, err = cache.MetaNamespaceKeyFunc(old)
			newEvent.eventType = "update"
			newEvent.resourceType = resourceType
			logrus.WithField("pkg", "kubewatch-"+resourceType).Infof("Processing update to %v: %s", resourceType, newEvent.key)
			if err == nil {
				queue.Add(newEvent)
			}*/
			log.WithFields(log.Fields{
				"by":     "NPC",
				"method": "UpdateFunc()",
			}).Info("Something has been updated!")
		},
		DeleteFunc: func(obj interface{}) {
			/*newEvent.key, err = cache.DeletionHandlingMetaNamespaceKeyFunc(obj)
			newEvent.eventType = "delete"
			newEvent.resourceType = resourceType
			newEvent.namespace = utils.GetObjectMetaData(obj).Namespace
			logrus.WithField("pkg", "kubewatch-"+resourceType).Infof("Processing delete to %v: %s", resourceType, newEvent.key)
			if err == nil {
				queue.Add(newEvent)
			}*/
			log.WithFields(log.Fields{
				"by":     "NPC",
				"method": "DeleteFunc()",
			}).Info("Something has been deleted!")
		},
	})

	l.Info("Just set up the event handlers")

	//	Everything set up, return the controller
	return &DefaultNetworkPolicyController{
		nodeName:  nodeName,
		clientset: clientset,
		queue:     queue,
		defaultNetworkPoliciesInformer: npcInformer,
	}
}

func (npc *DefaultNetworkPolicyController) Run() {

	var l = log.WithFields(log.Fields{
		"by":     "NPC",
		"method": "Start()",
	})

	l.Info("Network Policy Controller starting...")

	stopCh := make(chan struct{})
	defer close(stopCh)

	//	Don't let panics crash the process
	defer utilruntime.HandleCrash()

	//	Make sure the work queue is shutdown which will trigger workers to end
	defer npc.queue.ShutDown()

	//	Record when we started, it is going to be used later
	npc.startedOn = time.Now().UTC()

	//	Let's go!
	go npc.defaultNetworkPoliciesInformer.Run(stopCh)

	//	Make sure the cache is populated
	if !cache.WaitForCacheSync(stopCh, npc.defaultNetworkPoliciesInformer.HasSynced) {
		utilruntime.HandleError(fmt.Errorf("Timed out waiting for caches to sync"))
		return
	}

	//	Work *until* something bad happens. If that's the case, wait one second and then re-work again.
	//	Well, except when someone tells us to stop... in that case, just stop, man
	wait.Until(npc.work, time.Second, stopCh)
}

func (npc *DefaultNetworkPolicyController) work() {

	var stop = false
	var l = log.WithFields(log.Fields{
		"by":     "NPC",
		"method": "work()",
	})

	l.Info("Starting method...")

	for !stop {

		l.Infof("Ok, I'm going to get a new item from the queue...")

		//	Get the item's key from the queue
		//	NOTE: as far as I have learned, this blocks when no items are there.
		//	So this is a looper that does not consume cpu cycles
		key, quit := npc.queue.Get()

		l.Infof("Just got the item: its key is %s", key)

		//	Ok, parse & process the policy
		err := npc.processPolicy(key.(string))

		//	No errors?
		if err == nil {
			//	Then reset the ratelimit counters
			npc.queue.Forget(key)
			l.Infof("Item with key %s has been forgotten from the queue", key)
		} else if npc.queue.NumRequeues(key) < maxRetries {
			//	Tried less than the maximum retries?
			l.Warningf("Error processing item with key %s (will retry): %v", key, err)
			npc.queue.AddRateLimited(key)
		} else {
			//	Too many retries?
			l.Errorf("Error processing %s (giving up): %v", key, err)
			npc.queue.Forget(key)
			utilruntime.HandleError(err)
		}

		stop = quit
	}
}

func (npc *DefaultNetworkPolicyController) processPolicy(key string) error {

	var l = log.WithFields(log.Fields{
		"by":     "NPC",
		"method": "processPolicy()",
	})
	var policy *networking_v1.NetworkPolicy

	defer npc.queue.Done(key)
	//var annotations map[string]string

	l.Infof("Starting to process policy")

	//	Get the policy by querying the key that kubernetes has assigned to this in its cache
	_policy, exists, err := npc.defaultNetworkPoliciesInformer.GetIndexer().GetByKey(key)

	//	Errors?
	if err != nil {
		l.Errorf("An error occurred: cannot find cache element with key %s from store %v", key, err)

		//	TODO: check this
		return fmt.Errorf("An error occurred: cannot find cache element with key %s from ", key)
	}

	//	Get the policy
	policy = _policy.(*networking_v1.NetworkPolicy)

	//	Get the annotations
	//	TODO: this is going to be used to check for whitelist/blacklist feature
	//policy.ObjectMeta.GetAnnotations()

	//	Get the specs
	spec := policy.Spec

	//-------------------------------------
	//	Parse the ingress rules
	//-------------------------------------

	ingress := spec.Ingress

	//	Apparently, when yaml has Ingress: [] this is called, instead of len() < 1
	if ingress == nil {
		l.Info("Ingress is null: this resource accepts no connections.")
	} else {
		//	Nothing? This means that this resource doesn't accept anything in ingress
		if len(ingress) < 1 {
			l.Info("There are no ingress rules: this resource accepts no connections.")
		} else {
			l.Info("The following rules have been found")

			for _, ingressRule := range ingress {

				//-------------------------------------
				//	Peer Rules
				//-------------------------------------

				//	TODO: check this, because when ingress: - {} is found, this is called, totally different from the case above!!!
				if ingressRule.From == nil {
					l.Info("from is null: nothing can be accessed.")
				} else {

					//	From is specified but is an empty array => nothing is allowed
					if len(ingressRule.From) < 1 {
						l.Info("Found empty array, resource doesn't accept connections")
					} else {
						for _, peer := range ingressRule.From {

							//-------------------------------------
							//	Select an IP block
							//-------------------------------------

							if peer.IPBlock == nil {
								l.Info("No IPBlock has been specified")
							} else {

								IPBlock := peer.IPBlock
								cidr := IPBlock.CIDR

								l.Info("%s specified", cidr)

								if IPBlock.Except != nil && len(IPBlock.Except) > 0 {
									l.Info("The following exceptions have been found")

									for _, exception := range IPBlock.Except {
										l.Info("%s", exception)
									}
								}
							}

							//-------------------------------------
							//	Select pods
							//-------------------------------------

							if peer.PodSelector == nil {
								l.Info("No pod selector has been specified")
							} else {

								podSelector := peer.PodSelector

								//	Get match labels
								//	TODO: same as above
								if podSelector.MatchLabels == nil {
									l.Info("Empty podSelector: I have to select all pods")
								} else {
									l.Info("I found the following match labels rules")
									for key, value := range podSelector.MatchLabels {
										l.Infof("%s => %s", key, value)
									}
								}

								//	Get Expression match
								if len(podSelector.MatchExpressions) < 1 {
									l.Info("There are no match expressions")
								} else {
									for _, selectorRequirements := range podSelector.MatchExpressions {
										//	TODO: selectorRequirements.Operator returns a LabelSelectorOperator, not a string. So check this
										l.Infof("%s => %s: %s", selectorRequirements.Key, selectorRequirements.Operator, strings.Join(selectorRequirements.Values, ","))
									}
								}

							}

							//-------------------------------------
							//	Select namespaces
							//-------------------------------------

							if peer.NamespaceSelector == nil {
								l.Info("No namespaces has been specified ")
							} else {

								//	Commented just to make the compiler shut up
								//nameSpaceSelector := peer.NamespaceSelector

								//	basically do exactly the same as above. So the above piece of code must be done in a function
								l.Info("Gotta parse the namespace, it seems!")
							}
						}
					}
				}

				//-------------------------------------
				//	Port rules
				//-------------------------------------

				if len(ingressRule.Ports) < 1 {
					l.Info("There a no port rules")
				} else {
					for _, portStruct := range ingressRule.Ports {

						//	TODO: protocol is a struct, not a string
						_protocol := portStruct.Protocol
						var protocol string

						//	TODO: port is a
						_port := portStruct.Port
						var port int32

						switch *_protocol {
						case v1.ProtocolTCP:
							protocol = "TCP"
						case v1.ProtocolUDP:
							protocol = "UDP"
						case v1.ProtocolSCTP:
							protocol = "SCTP"
						}

						/*if *_port.Type == intstr.String {
							port = *_port.IntVal
						} else {
							port = convert the string to int
						}*/
						//	TODO: check if this always works. Or if I must do a type check (like above)
						port = _port.IntVal

						l.Infof("protocol is %s and port is %d", protocol, port)
					}
				}
			}
		}
	}

	//	Does not exist?
	if !exists {
		l.Infof("Object with key %s does not exist. Going to trigger a onDelete function", key)
	}

	return nil
}

func (npc *DefaultNetworkPolicyController) Stop() {

	log.WithFields(log.Fields{
		"by":     "NPC",
		"method": "Stop())",
	}).Info("Network Policy Controller just stopped")
}
