package controllers

import (

	//	TODO-ON-MERGE: change the path to polycube
	events "github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/types/events"
	query "github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/types/podquery"
	polycube_pod "github.com/SunSince90/polycube/src/components/k8s/pcn_k8s/types/polycubepod"
)

type PodController interface {
	Run()
	Stop()
	Subscribe(events.EventType, func(*polycube_pod.Pod)) (func(), error)

	GetPods(query.Query) ([]polycube_pod.Pod, error)
	AsyncGetPods(chan<- []polycube_pod.Pod, query.Query) ([]polycube_pod.Pod, error)

	GetPodsByName(string, string) ([]polycube_pod.Pod, error)
	AsyncGetPodsByName(chan<- []polycube_pod.Pod, string, string) ([]polycube_pod.Pod, error)

	GetPodsByLabels(map[string]string, string) ([]polycube_pod.Pod, error)
	AsyncGetPodsByLabels(chan<- []polycube_pod.Pod, map[string]string, string) ([]polycube_pod.Pod, error)
}
