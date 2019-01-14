package eventyypes

type Event struct {
	key       string
	eventType EventType
	namespace string
}

type EventType int

const (
	New = iota + 1
	Update
	Delete
)
