/*
 * Copyright 2017 The Polycube Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package main

import (
	"fmt"
	"os"

	"github.com/polycube-network/polycube/src/polycubectl/cliargs"
	"github.com/polycube-network/polycube/src/polycubectl/config"
)

const (
	// Return codes
	// the command syntax validation fails
	ErrCommandNotValid = iota

	// the cli is unable to contact the rest server
	ErrHttpRequestFailed = iota

	// the server returns a no success status code
	ErrBadStatusCode = iota
)

var (
	// use a buffer to save stdout in order to allow to duplicate it to a logfile
	Buffer string
)

func main() {
	var err error
	err = config.LoadConfig()
	if err != nil {
		fmt.Fprintf(os.Stderr, "Error loading config file: %s", err.Error())
		os.Exit(1)
	}

	LogCommand(os.Args)

	cliArgs, err := cliargs.ParseCLIArgs(os.Args[1:])
	if err != nil {
		Buffer += err.Error() + "\n"
		ExitAndLog(ErrCommandNotValid)
	}

	FillAliases()
	if err, exit := RunAliases(cliArgs); err != nil {
		Buffer += err.Error() + "\n"
		ExitAndLog(ErrCommandNotValid)
	} else if exit {
		ExitAndLog(0)
	}

	httpReq, err := cliArgs.GetHTTPRequest()

	// print performed request
	if config.GetConfig().Debug {
		Buffer += httpReq.ToString()
	}

	response, error := httpReq.Perform()
	if error != nil {
		Buffer += fmt.Sprintln(error)
		ExitAndLog(ErrHttpRequestFailed)
	}
	printResponse(cliArgs, response)
	ExitAndLog(GetReturnCode(response.StatusCode))
}
