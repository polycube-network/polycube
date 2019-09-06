package controllers

import (
	log "github.com/sirupsen/logrus"
	"k8s.io/client-go/kubernetes"
)

var (
	k8sPoliciesController K8sNetworkPolicyController
	podController         PodController
	nsController          NamespaceController
	serviceController     ServiceController
	clientset             kubernetes.Interface
	logger                *log.Logger
)

const (
	// maxRetries tells how many times the controller should attempt
	// decoding an object from the queue
	maxRetries                 int = 5
	podControllerWorkers       int = 1
	k8sPolicyControllerWorkers int = 1
	nsControllerWorkers        int = 1
)

func init() {
	logger = log.New()

	// Only log the debug severity or above.
	logger.SetLevel(log.DebugLevel)

	// All logs will specify who the function that logged it
	//l.SetReportCaller(true)
}

// Start starts all the controllers included
func Start(cs kubernetes.Interface) {
	clientset = cs

	//--------------------------------
	// Set up...
	//--------------------------------

	// Set up the network policy controller (for the kubernetes policies)
	k8sPoliciesController = createK8sNetworkPolicyController()

	// Get the namespace controller
	nsController = createNsController()

	// Get the pod controller
	podController = createPodController()

	// Get the service controller
	serviceController = createServiceController()

	//--------------------------------
	// ... and start
	//--------------------------------

	// Start the default network policy controller
	go k8sPoliciesController.Run()

	// Start the namespace controller
	go nsController.Run()

	// Start the pod controller
	go podController.Run()

	// Start the service controller
	go serviceController.Run()
}

// Stop stops all the controllers that have been started at once.
func Stop() {
	// Resetting them as nil, prevents further calls to this function from
	// crashing the whole application ("close of a closed channel" error)
	if podController != nil {
		podController.Stop()
		podController = nil
	}

	if k8sPoliciesController != nil {
		k8sPoliciesController.Stop()
		k8sPoliciesController = nil
	}

	if nsController != nil {
		nsController.Stop()
		nsController = nil
	}

	if serviceController != nil {
		serviceController.Stop()
		serviceController = nil
	}
}

// Pods returns the pod controller
func Pods() PodController {
	return podController
}

// K8sPolicies returns the Kubernetes Network Policy Controller
func K8sPolicies() K8sNetworkPolicyController {
	return k8sPoliciesController
}

// Namespaces returns the Namespace controller
func Namespaces() NamespaceController {
	return nsController
}

// Services returns the Service Controller
func Services() ServiceController {
	return serviceController
}
