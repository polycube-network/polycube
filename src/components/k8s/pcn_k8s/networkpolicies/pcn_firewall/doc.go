// Package pcnfirewall contains the logic of the firewall manager.
// A firewall manager manages local firewalls (e.g.: in this node),
// attached to similar pods (same namespace and same labels).
// This will prevent us from parsing and doing n times each operation if we have
// n similar pods: it will be done only once, as the firewall manager will
// take care of it, so it performs better when it is monitoring more than 1 pod.
// The firewall manager will react to pod events on its own, without
// intermediaries and according to the defined network policies.
// Pods which match the namespace and labels, should be linked to their
// appropriate firewall manager instead of creating a new one.
// A firewall manager may have no pods monitored left (e.g.: they all died):
// in that case *DO NOT* destroy the firewall manager manually,
// because the fw manager will still continue to parse policies and react to
// events; it will just have no firewall pods to inject these rules to.
// This is because, if monitored pods die, it is very probable that they will be
// re-scheduled and will be born again. When they do, no parse will be needed:
// the fw manager will already have all the rules ready for them and will be
// updated to the last minute, so it'll just inject the rules, sparing us
// a lot of computation. In case no pod is deployed and a firewall manager
// has no monitored pods for at least X minutes, then it will be automatically
// deleted.
package pcnfirewall

const (
	// CleanFirewall specifies the clean action
	CleanFirewall UnlinkOperation = "clean"
	// DestroyFirewall specifies the destroy action
	DestroyFirewall UnlinkOperation = "destroy"
	// DoNothing specifies that no action should be taken
	DoNothing UnlinkOperation = "nothing"
)

// UnlinkOperation is the operation that should be performed
// after unlinking a pod
type UnlinkOperation string
