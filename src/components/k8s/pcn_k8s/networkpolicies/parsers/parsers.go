package parsers

import (
	log "github.com/sirupsen/logrus"
)

var (
	logger *log.Logger
)

func init() {
	logger = log.New()

	// Only log the debug severity or above.
	logger.SetLevel(log.DebugLevel)

	// All logs will specify who the function that logged it
	//logger.SetReportCaller(true)
}
