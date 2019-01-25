package podquery

type QueryObject struct {
	By     string
	Name   string
	Labels map[string]string
}

type Query struct {
	Pod       []QueryObject
	Namespace []QueryObject
}
