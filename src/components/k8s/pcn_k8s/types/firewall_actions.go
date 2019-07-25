package types

// FirewallAction contains the actions performed when reacting to a pod event.
type FirewallAction struct {
	PodLabels       map[string]string
	NamespaceName   string
	NamespaceLabels map[string]string
	Key             string
	Templates       ParsedRules
}

// Protoport defines the protocol and the port
type ProtoPort struct {
	Protocol string
	Port     int32
}
