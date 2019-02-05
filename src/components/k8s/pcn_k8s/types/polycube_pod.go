package types

import (
	core_v1 "k8s.io/api/core/v1"
)

type Pod struct {
	Pod  core_v1.Pod
	Veth string
}
