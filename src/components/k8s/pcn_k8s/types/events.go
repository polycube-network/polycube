package types

// Event is the event from k8s cache
type Event struct {
	// NewKey is the key assigned to the object
	NewKey string
	// OldKey is the key assigned to the object
	OldKey string
	// Type is the event type (DELETE, UPDATE, NEW)
	Type EventType
	// Namespace is the namespace of the object
	Namespace string
	// NewObject is the object in its current state
	NewObject interface{}
	// OldObject is the object in its previous state
	OldObject interface{}
}

// EventType is the type of the event
type EventType int

const (
	// New represents the new event ID
	New = iota + 1
	// Update represents the update event ID
	Update
	// Delete represents the delete event ID
	Delete
)
