package v1beta

import metav1 "k8s.io/apimachinery/pkg/apis/meta/v1"

// +genclient
// +k8s:deepcopy-gen:interfaces=k8s.io/apimachinery/pkg/runtime.Object

// PolycubeNetworkPolicy is a network policy handled by polycube
type PolycubeNetworkPolicy struct {
	metav1.TypeMeta `json:",inline"`
	// +optional
	metav1.ObjectMeta `json:"metadata,omitempty"`
	// ApplyTo defines who this policy is intended for
	ApplyTo PolycubeNetworkPolicyTarget `json:"applyTo,omitempty"`
	// Priority explicitly defines the priority of the policy
	Priority PolycubeNetworkPolicyPriority `json:"priority,omitempty"`
	// Spec of this policy
	Spec PolycubeNetworkPolicySpec `json:"spec,omitempty"`
}

// PolycubeNetworkPolicyPriority is the explicit priority of the policy
type PolycubeNetworkPolicyPriority int

// PolycubeNetworkPolicyTarget is the target of this policy
type PolycubeNetworkPolicyTarget struct {
	// Target is the object that should enforce this policy
	Target PolycubeNetworkPolicyTargetObject `json:"target,omitempty"`
	// +optional
	// If name and labels are irrelevant
	Any *bool `json:"any,omitempty"`
	// +optional
	// WithName specifies the name of the object. Valid only for Service
	WithName string `json:"withName,omitempty"`
	// +optional
	// WithLabels specifies the labels of the target. Valid only for Pod
	WithLabels map[string]string `json:"withLabels,omitempty"`
}

// PolycubeNetworkPolicyTargetObject is the target object
type PolycubeNetworkPolicyTargetObject string

const (
	// PodTarget represents a Pod
	PodTarget PolycubeNetworkPolicyTargetObject = "pod"
	// ServiceTarget represents a Service
	ServiceTarget PolycubeNetworkPolicyTargetObject = "service"
)

// PolycubeNetworkPolicySpec contains the specifications of this Network Policy
type PolycubeNetworkPolicySpec struct {
	// +optional
	// Description is the description of the policy
	Description string `json:"description,omitempty"`
	// +optional
	// IngressRules contains the ingress rules
	IngressRules PolycubeNetworkPolicyIngressRuleContainer `json:"ingressRules,omitempty"`
	// +optional
	// EgressRules contains the egress rules
	EgressRules PolycubeNetworkPolicyEgressRuleContainer `json:"egressRules,omitempty"`
}

// PolycubeNetworkPolicyIngressRuleContainer is a container of ingress rules
type PolycubeNetworkPolicyIngressRuleContainer struct {
	// +optional
	// DropAll specifies to drop everything in ingress
	DropAll *bool `json:"dropAll,omitempty"`
	// +optional
	// AllowAll specifies to allow anyone in ingress
	AllowAll *bool `json:"allowAll,omitempty"`
	// +optional
	// PreventDirectAccess specifies to allow anyone in ingress
	PreventDirectAccess *bool `json:"preventDirectAccess,omitempty"`
	// +optional
	// Rules is a list of ingress rules
	Rules []PolycubeNetworkPolicyIngressRule `json:"rules,omitempty"`
}

// PolycubeNetworkPolicyEgressRuleContainer is a container of egress rules
type PolycubeNetworkPolicyEgressRuleContainer struct {
	// +optional
	// DropAll specifies to drop everything in egress
	DropAll *bool `json:"dropAll,omitempty"`
	// +optional
	// AllowAll specifies to allow anyone in egress
	AllowAll *bool `json:"allowAll,omitempty"`
	// +optional
	// Rules is a list of egress rules
	Rules []PolycubeNetworkPolicyEgressRule `json:"rules,omitempty"`
}

// PolycubeNetworkPolicyIngressRule is an ingress rule
type PolycubeNetworkPolicyIngressRule struct {
	// From is the peer
	From PolycubeNetworkPolicyPeer `json:"from,omitempty"`
	// Protocols is the level 4 protocol list
	Protocols []PolycubeNetworkPolicyProtocolContainer `json:"protocols,omitempty"`
	// TCPFlags is a list of TCP flags
	//TCPFlags []PolycubeNetworkPolicyTCPFlag `json:"tcpflags,omitempty"`
	// Action is the action to be taken
	Action PolycubeNetworkPolicyRuleAction `json:"action,omitempty"`
	// Description is the description of the rule
	Description string `json:"description,omitempty"`
}

// PolycubeNetworkPolicyProtocolContainer contains the protocol details
type PolycubeNetworkPolicyProtocolContainer struct {
	// Ports is the container of the ports
	Ports PolycubeNetworkPolicyPorts `json:"ports,omitempty"`
	// Protocol is the l4 protocol
	Protocol PolycubeNetworkPolicyProtocol `json:"protocol,omitempty"`
}

// PolycubeNetworkPolicyEgressRule the rule for egress
type PolycubeNetworkPolicyEgressRule struct {
	// To is the peer
	To PolycubeNetworkPolicyPeer `json:"to,omitempty"`
	// Protocols is the protocols list
	Protocols []PolycubeNetworkPolicyProtocolContainer `json:"protocols,omitempty"`
	// TCPFlags is a list of TCP flags
	TCPFlags []PolycubeNetworkPolicyTCPFlag `json:"tcpflags,omitempty"`
	// Action is the action to be taken
	Action PolycubeNetworkPolicyRuleAction `json:"action,omitempty"`
	// Description is the description of the rule
	Description string `json:"description,omitempty"`
}

// PolycubeNetworkPolicyPeer contains data of the peer
type PolycubeNetworkPolicyPeer struct {
	// Peer is the peer type
	Peer PolycubeNetworkPolicyPeerObject `json:"peer,omitempty"`
	// +optional
	// Any tells if name and labels don't matter
	Any *bool `json:"any,omitempty"`
	// +optional
	// WithName specifies the name of the object. Only for Service
	WithName string `json:"withName,omitempty"`
	// +optional
	// WithLabels specifies the labels of the object. Only for Pod
	WithLabels map[string]string `json:"withLabels,omitempty"`
	// +optional
	// WithIP specifies the ip. Only for World
	WithIP PolycubeNetworkPolicyWithIP `json:"withIP,omitempty"`
	// +optional
	// OnNamespace specifies the namespaces of the peer. Only for Pod
	OnNamespace *PolycubeNetworkPolicyNamespaceSelector `json:"onNamespace,omitempty"`
}

// PolycubeNetworkPolicyWithIP is the IP container
type PolycubeNetworkPolicyWithIP struct {
	//	List is a list of IPs in CIDR notation
	List []string `json:"list,omitempty"`
}

// PolycubeNetworkPolicyPeerObject is the object peer
type PolycubeNetworkPolicyPeerObject string

const (
	// PodPeer is the Pod
	PodPeer PolycubeNetworkPolicyPeerObject = "pod"
	// WorldPeer is the World
	WorldPeer PolycubeNetworkPolicyPeerObject = "world"
)

// PolycubeNetworkPolicyNamespaceSelector is a selector for namespaces
type PolycubeNetworkPolicyNamespaceSelector struct {
	// +optional
	// WithNames is a list of the names of the namespace
	WithNames []string `json:"withNames,omitempty"`
	// +optional
	// WithLabels is the namespace's labels
	WithLabels map[string]string `json:"withLabels,omitempty"`
	// +optional
	// Any specifies any namespace
	Any *bool `json:"any,omitempty"`
}

// PolycubeNetworkPolicyProtocol is the level 4 protocol
type PolycubeNetworkPolicyProtocol string

const (
	// TCP is TCP
	TCP PolycubeNetworkPolicyProtocol = "tcp"
	// UDP is UDP
	UDP PolycubeNetworkPolicyProtocol = "udp"
	// ICMP is ICMPv4
	ICMP PolycubeNetworkPolicyProtocol = "icmp"
)

// PolycubeNetworkPolicyPorts contains the ports
type PolycubeNetworkPolicyPorts struct {
	// +optional
	// Source is the source port
	Source int32 `json:"source,omitempty"`
	// Destination is the destination port
	Destination int32 `json:"destination,omitempty"`
}

// PolycubeNetworkPolicyTCPFlag is the TCP flag
type PolycubeNetworkPolicyTCPFlag string

const (
	// SYNFlag is SYN
	SYNFlag PolycubeNetworkPolicyTCPFlag = "SYN"
	// FINFlag is FIN
	FINFlag PolycubeNetworkPolicyTCPFlag = "FIN"
	// ACKFlag is ACK
	ACKFlag PolycubeNetworkPolicyTCPFlag = "ACK"
	// RSTFlag is RST
	RSTFlag PolycubeNetworkPolicyTCPFlag = "RST"
	// PSHFlag is PSH
	PSHFlag PolycubeNetworkPolicyTCPFlag = "PSH"
	// URGFlag is URG
	URGFlag PolycubeNetworkPolicyTCPFlag = "URG"
	// CWRFlag is CWR
	CWRFlag PolycubeNetworkPolicyTCPFlag = "CWR"
	// ECEFlag is ECE
	ECEFlag PolycubeNetworkPolicyTCPFlag = "ECE"
)

// PolycubeNetworkPolicyRuleAction is the action
type PolycubeNetworkPolicyRuleAction string

const (

	//	Drop actions

	// DropAction is DROP
	DropAction PolycubeNetworkPolicyRuleAction = "drop"
	// BlockAction is DROP
	BlockAction PolycubeNetworkPolicyRuleAction = "block"
	// ForbidAction is DROP
	ForbidAction PolycubeNetworkPolicyRuleAction = "forbid"
	// ProhibitAction is DROP
	ProhibitAction PolycubeNetworkPolicyRuleAction = "prohibit"

	//	Allow actions

	// ForwardAction is Forward
	ForwardAction PolycubeNetworkPolicyRuleAction = "forward"
	// AllowAction is Forward
	AllowAction PolycubeNetworkPolicyRuleAction = "allow"
	// PassAction is Forward
	PassAction PolycubeNetworkPolicyRuleAction = "pass"
	// PermitAction is Forward
	PermitAction PolycubeNetworkPolicyRuleAction = "permit"
)

// +k8s:deepcopy-gen:interfaces=k8s.io/apimachinery/pkg/runtime.Object

// PolycubeNetworkPolicyList contains a list of Network Policies.
type PolycubeNetworkPolicyList struct {
	metav1.TypeMeta `json:",inline"`
	// +optional
	metav1.ListMeta `son:"metadata,omitempty"`
	// Items contains the network policies
	Items []PolycubeNetworkPolicy `json:"items"`
}
