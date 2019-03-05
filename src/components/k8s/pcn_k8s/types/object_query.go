package types

// QueryObject is a struct that specifies how the object should be found
type QueryObject struct {
	By     string
	Name   string
	Labels map[string]string
}
