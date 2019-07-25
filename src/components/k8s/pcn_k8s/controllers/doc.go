// Package controllers contains the controllers used by Polycube.
// They are used to watch for events about resources or to get
// information about them.
package controllers

const (
	// PC is the short name of the Pod Controller
	PC string = "PodController"
	// KNPC is the short name of the K8s Network Policy Controller
	KNPC string = "K8sNetworkPolicyController"
	// NC is the short name of the Namespace Controller
	NC string = "NamespaceController"
)
