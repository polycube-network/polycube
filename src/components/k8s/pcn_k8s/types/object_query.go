package types

// ObjectQuery is a struct that specifies how the object should be found
type ObjectQuery struct {
	By     string
	Name   string
	Labels map[string]string
}
