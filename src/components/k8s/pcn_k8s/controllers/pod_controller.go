package controllers

import (

	//	TODO-ON-MERGE: change the path to polycube
	pcn_types "github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/types"
)

type PodController interface {
	Run()
	Stop()
	Subscribe(pcn_types.EventType, func(*pcn_types.Pod)) (func(), error)

	GetPods(pcn_types.Query) ([]pcn_types.Pod, error)
	/*AsyncGetPods(chan<- []polycube_pod.Pod, query.Query) ([]polycube_pod.Pod, error)

	GetPodsByName(string, string) ([]polycube_pod.Pod, error)
	AsyncGetPodsByName(chan<- []polycube_pod.Pod, string, string) ([]polycube_pod.Pod, error)

	GetPodsByLabels(map[string]string, string) ([]polycube_pod.Pod, error)
	AsyncGetPodsByLabels(chan<- []polycube_pod.Pod, map[string]string, string) ([]polycube_pod.Pod, error)*/
}
