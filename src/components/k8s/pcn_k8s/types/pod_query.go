package types

type PodQueryObject struct {
	By     string
	Name   string
	Labels map[string]string
}

type PodQuery struct {
	Pod       PodQueryObject
	Namespace PodQueryObject
}
