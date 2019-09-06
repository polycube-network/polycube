package networkpolicies

import (
	log "github.com/sirupsen/logrus"
)

const (
	// UnscheduleThreshold is the maximum number of minutes
	// a firewall manager should live with no pods monitored.
	UnscheduleThreshold = 5
)

var (
	logger   *log.Logger
	basePath string
)

func init() {
	logger = log.New()

	// Only log the debug severity or above.
	logger.SetLevel(log.DebugLevel)

	// All logs will specify who the function that logged it
	//logger.SetReportCaller(true)
}

// SetBasePath sets polycube's base path
func SetBasePath(path string) {
	basePath = path
}
