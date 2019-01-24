package controllers

import (
	core_v1 "k8s.io/api/core/v1"

	//	TODO-ON-MERGE: change the path to polycube
	events "github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/types/events"
)

type PodController interface {
	Run()
	Stop()
	Subscribe(events.EventType, func(*core_v1.Pod)) (func(), error)

	GetPodsByName(string, string) (error, []core_v1.Pod, string)
	AsyncGetPodsByName(chan<- []core_v1.Pod, string, string) (error, []core_v1.Pod, string)

	GetPodsByLabels(map[string]string, string) (error, []core_v1.Pod, string)
	AsyncGetPodsByLabels(chan<- []core_v1.Pod, map[string]string, string) (error, []core_v1.Pod, string)
}
