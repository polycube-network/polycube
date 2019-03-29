package types

type FirewallActions struct {
	Ingress []FirewallAction
	Egress  []FirewallAction
}

type FirewallAction struct {
	PodLabels       map[string]string
	NamespaceName   string
	NamespaceLabels map[string]string
	Actions         ParsedRules
}

type ProtoPort struct {
	Protocol string
	Port     int32
}

type FirewallActionType string

const (
	Forward       = "forward"
	Drop          = "drop"
	Log           = "log"
	SameNamespace = "#SAME-NAMESPACE#"
)
