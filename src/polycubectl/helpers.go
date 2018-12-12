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
	"io/ioutil"
	"net/http"
	"os"
	"strings"

	"github.com/polycube-network/polycube/src/polycubectl/cliargs"
	"github.com/polycube-network/polycube/src/polycubectl/config"

	"github.com/Jeffail/gabs2"
	"github.com/ghodss/yaml"
)

func printResponse(req *cliargs.CLIArgs, response *http.Response) {

	body, err := ioutil.ReadAll(response.Body)

	if response.StatusCode < 200 || response.StatusCode >= 300 {
		bodystr := string([]byte(body))
		if bodystr != "" {
			Buffer += fmt.Sprintf("%s\n", bodystr)
			return
		}
	}

	if !req.IsHelp {
		if config.GetConfig().Debug {
			Buffer += fmt.Sprintf("%s\n", response.Status)
		} else {
			if response.StatusCode < 200 || response.StatusCode >= 300 {
				Buffer += fmt.Sprintf("%s\n", response.Status)
			}
		}
	} else {
		if response.StatusCode < 200 || response.StatusCode >= 300 {
			printHelpNotSupported()
			Buffer += fmt.Sprintln("")
		}
	}

	if err != nil {
		if config.GetConfig().Debug {
			Buffer += fmt.Sprintln(err)
		} else if response.StatusCode < 200 || response.StatusCode >= 300 {
			Buffer += fmt.Sprintln(err)
		}
		return
	}

	jsonParsed, errJSON := gabs2.ParseJSON([]byte(body))
	if errJSON != nil {
		if config.GetConfig().Debug {
			Buffer += fmt.Sprintln(errJSON)
		}
		return
	}

	if req.IsHelp {
		printHelp(req, jsonParsed)
	} else {
		// Print the body of the response only if
		//  - error
		//  - show command

		if req.Command == cliargs.AddCommand ||
			req.Command == cliargs.DelCommand ||
			req.Command == cliargs.SetCommand ||
			response.StatusCode < 200 || response.StatusCode >= 300 {
			return
		}

		switch req.ShowType {
		case cliargs.ShowTypeUnspec:
			fallthrough
		case cliargs.ShowTypeNormal:
			buf, _ := PrettyPrint([]byte(body), req.HideList, 2)
			Buffer += fmt.Sprintf("%s\n", string([]byte(buf)))
		case cliargs.ShowTypeBrief:
			buf, _ := PrettyPrint([]byte(body), req.HideList, 1)
			Buffer += fmt.Sprintf("%s\n", string([]byte(buf)))
		case cliargs.ShowTypeVerbose:
			buf, _ := PrettyPrint([]byte(body), req.HideList, 3)
			Buffer += fmt.Sprintf("%s\n", string([]byte(buf)))
		case cliargs.ShowTypeJson:
			Buffer += fmt.Sprintf("%s\n", jsonParsed.StringIndent("", "  "))
		case cliargs.ShowTypeYaml:
			yamlbuffer, err := yaml.JSONToYAML([]byte(body))
			if err != nil {
				Buffer += fmt.Sprintf("Yaml converter error %v\n", err)
			}
			Buffer += fmt.Sprintf("%s\n", string([]byte(yamlbuffer)))
		}
	}
}

// GetReturnCode : returns the exit code of the cli based on http Response code
func GetReturnCode(httpStatusCode int) int {
	// if the response is not a 2xx StatusCode, return BAD_STATUS_CODE
	if httpStatusCode < 200 || httpStatusCode >= 300 {
		return ErrBadStatusCode
	}
	// if the response is a 2xx StatusCode, return success
	return 0
}

func ExitAndLog(retcode int) {
	BufferReplace()
	// Print Buffer on stdout
	fmt.Printf("%s", Buffer)
	// Log Command and Response
	LogCommandAndAnswer(os.Args, Buffer)
	// Exit with return code
	os.Exit(retcode)
}

// this function is called to substitute some character/messagges before print them
func BufferReplace() {
	// replace /n /t in datamodel
	if strings.Contains(Buffer, "datamodel:") {
		//remove \n in datamodel string before print it
		Buffer = strings.Replace(Buffer, "\n    ", " ", -1)

		Buffer = strings.Replace(Buffer, "\\n", "\n", -1)
		Buffer = strings.Replace(Buffer, "\\t", "\t", -1)
		Buffer = strings.Replace(Buffer, "\\\"", "\"", -1)
	}

	//polycubed not running
	if strings.Contains(Buffer, "getsockopt: connection refused") {
		errmsg := "Cannot contact the polycubed daemon at " + config.GetConfig().Url + " Please verify that the daemon is running."
		if config.GetConfig().Debug {
			Buffer += fmt.Sprintln()
			Buffer += fmt.Sprintln(errmsg)
		} else {
			Buffer = fmt.Sprintln(errmsg)
		}
	}
}
