package types

import (
	core_v1 "k8s.io/api/core/v1"
)

// These are the constants defined in core_v1,
// but we also added "Terminating" and "Any".
const (
	PodPending     core_v1.PodPhase = "Pending"
	PodRunning     core_v1.PodPhase = "Running"
	PodSucceeded   core_v1.PodPhase = "Succeeded"
	PodFailed      core_v1.PodPhase = "Failed"
	PodTerminating core_v1.PodPhase = "Terminating"
	PodUnknown     core_v1.PodPhase = "Unknown"
	PodAnyPhase    core_v1.PodPhase = "Any"
)
