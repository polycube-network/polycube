package types

// ObjectQuery is a struct that specifies how the object should be found
type ObjectQuery struct {
	// By specifies how to search ("name" or "labels")
	By string
	// Name is the name of the resource, to use when By is "name"
	Name string
	// Labels contains the labels of the resource, when By is "labels"
	Labels map[string]string
}
