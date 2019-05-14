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

package cliargs

import (
	"fmt"
	"strconv"
	"strings"

	"os"
	"bufio"
	"bytes"
	"github.com/ghodss/yaml"

	"github.com/polycube-network/polycube/src/polycubectl/config"
	"github.com/polycube-network/polycube/src/polycubectl/httprequest"

	"github.com/Jeffail/gabs"
)

const (
	ShowTypeUnspec  = iota
	ShowTypeNormal  = iota
	ShowTypeBrief   = iota
	ShowTypeVerbose = iota
	ShowTypeJson    = iota
	ShowTypeYaml    = iota
)

const (
	AddCommand  = "add"
	ShowCommand = "show"
	DelCommand  = "del"
	SetCommand  = "set"

	ShowNormal  = "-normal"
	ShowBrief   = "-brief"
	ShowVerbose = "-verbose"
	ShowJson    = "-json"
	ShowYaml    = "-yaml"
)

/* Algorithm
 *
 * The parser loops through all arguments in order to create the http request
 * to be send to the server.
 * The list of arguments is composed by path arguments, body arguments, command
 * and flags.
 *
 * e.x: polycubectl hw0 ports add port1 peer=veth1 type=external
 *                   |    |    |    |       |          |
 *                   |    |    |    |       +----------+---> body arguments
 *                   |    |    +----|----------------------> command
 *                   +----+---------+----------------------> path arguments
 *
 * Path arguments defines what is the resource where the command should
 * be executed.  The URL where the request is send is defined by the
 * concatenation of those arguments.
 * In the example below hw0, ports and port1 are the path arguments, then that
 * request is send to the ../hw0/ports/port1 URL.
 *
 * Command is the action that has to be executed, it can be show, add, del or get.
 * This is as to be presest at most once and is optional.  When it is not present
 * a POST method is used in the http request.
 * By definition the command has to be before the body arguments.
 *
 * Body arguments are a series of peer value pairs using the peer=value notation.
 * Those parameters are used to create the body that is send to the server and
 * hence they can only be present if the command is add or set.
 * There is a special case described below where these parameters also affect
 * the URL.
 *
 * Flags are the modifiers used to change the behavior, for example -verbose
 * causes the show command to print more detailed information.
 * There is also a special character '?' that is used to print help about a
 * specific command.
 *
 * SingleParameterWorkaround:
 * When there is a single body parameter and the SingleParameterWorkaround
 * workaround flag is enabled in the configuration, the body paramater is split,
 * half in the URL and half in the body.
 * ex:
 * polycubectl hw0 ports port1 set peer=veth1
 * with SingleParameterWorkaround disabled would be:
 *   url: .../hw0/ports/port1/
 *   body: {"peer" : "veth1"}
 * with SingleParameterWorkaround enabled it would be:
 *   url: .../hw0/ports/port1/peer/
 *   body: "veth1"
 * This workaround is needed because old services (as bridge) does not accept
 * the parameter in the body. This workaround will be removed once we migrate all
 * services to the new api implementation.
 */

type CLIArgs struct {
	Command  string
	PathArgs []string
	BodyArgs map[string]string

	// kind of output the user requested (NORMAL, BRIEF, VERBOSE, YAML, JSON)
	ShowType int

	// if set, the command is an help command
	IsHelp bool

	// parameters to ignore while printing
	HideList map[string]bool
}

func argIsCommand(arg string) bool {
	return arg == AddCommand || arg == SetCommand ||
		arg == ShowCommand || arg == DelCommand
}

func argIsBodyArg(arg string) bool {
	return strings.Contains(arg, "=")
}

func argIsFlag(arg string) bool {
	return arg[0] == '-'
}

func argIsHelp(arg string) bool {
	return arg == "?"
}

func argIsHide(arg string) bool {
	list := strings.Split(arg, "=")
	return list[0] == "-hide"
}

func parseHideArg(arg string) (map[string]bool, error) {
	hideList := make(map[string]bool)

	list := strings.Split(arg, "=")
	// avoid -hide alone
	if len(list) != 2 {
		return nil, fmt.Errorf("-hide usage: -hide=key0,key1,key")
	}

	// avoid -hide= (without keys to hide)
	if list[1] == "" {
		return nil, fmt.Errorf("-hide usage: -hide=key0,key1,key")
	}

	keysList := strings.Split(list[1], ",")

	for _, k := range keysList {
		hideList[k] = true
	}

	return hideList, nil
}

func showTypeStrToShowType(arg string) int {
	switch arg {
	case ShowNormal:
		return ShowTypeNormal
	case ShowBrief:
		return ShowTypeBrief
	case ShowVerbose:
		return ShowTypeVerbose
	case ShowJson:
		return ShowTypeJson
	case ShowYaml:
		return ShowTypeYaml
	// TODO: return ShowTypeUnspec??
	default:
		return ShowTypeUnspec
	}
}

func ParseCLIArgs(args []string) (*CLIArgs, error) {
	pathArgs := []string{}
	bodyArgs := make(map[string]string)

	var command string
	var isHelp bool
	var showTypeStr string
	var hideList map[string]bool

	// true when we find the first body parameter
	processingBody := false

	if len(args) == 0 {
		isHelp = true
	}

	// process all args and verifies that they are correct
	for i, arg := range args {
		if argIsCommand(arg) {
			if command != "" {
				return nil, fmt.Errorf("Multiple commands found: %s and %s",
					arg, command)
			}

			if processingBody {
				return nil, fmt.Errorf("%s is not valid after %s", arg, args[i-1])
			}

			command = arg
		} else if argIsFlag(arg) {
			// TODO: if we want to check the relative flag position to other
			// args this is the place to do it

			if arg == ShowNormal || arg == ShowBrief || arg == ShowVerbose ||
				arg == ShowJson || arg == ShowYaml {
				if showTypeStr != "" {
					return nil, fmt.Errorf("%s and %s cannot be use together",
						arg, showTypeStr)
				}

				if command != ShowCommand {
					return nil, fmt.Errorf("%s can only be used with %s command",
						arg, ShowCommand)
				}

				if hideList != nil && (arg == ShowJson || arg == ShowYaml) {
					return nil, fmt.Errorf("-hide is not compatible with %s",
						arg)
				}

				// will be converted to showType later on
				showTypeStr = arg
			} else if arg == "-h" || arg == "--help" {
				if len(args) != 1 {
					return nil, fmt.Errorf("%s have to be the unique argument",
						arg)
				}

				isHelp = true
			} else if argIsHide(arg) {
				var err error
				if command != ShowCommand {
					return nil, fmt.Errorf("-hide can only be used with %s command",
						ShowCommand)
				}

				if showTypeStr == ShowJson || showTypeStr == ShowYaml {
					return nil, fmt.Errorf("-hide is not compatible with %s",
						showTypeStr)
				}

				hideList, err = parseHideArg(arg)
				if err != nil {
					return nil, err
				}
			} else {
				// TODO: print supported flags
				return nil, fmt.Errorf("%s is not a valid flag", arg)
			}
		} else if argIsBodyArg(arg) {
			processingBody = true
			split := strings.Split(arg, "=")
			if len(split) != 2 {
				return nil, fmt.Errorf("%s is not a valid format", arg)
			}

			if _, ok := bodyArgs[split[0]]; ok {
				return nil, fmt.Errorf("Duplicated values for %s", split[0])
			}

			bodyArgs[split[0]] = split[1]
		} else if argIsHelp(arg) {
			// the "?" argument should be the last one
			if len(args)-1 != i {
				return nil, fmt.Errorf("'?' must be the last argument")
			}

			isHelp = true
		} else {
			if processingBody {
				return nil, fmt.Errorf("%s is not valid after %s", arg, args[i-1])
			}

			pathArgs = append(pathArgs, arg)
		}
	}

	req := &CLIArgs{
		Command:  command,
		PathArgs: pathArgs,
		BodyArgs: bodyArgs,
		ShowType: showTypeStrToShowType(showTypeStr),
		IsHelp:   isHelp,
		HideList: hideList,
	}

	return req, nil
}

func (cli *CLIArgs) buildURL() string {
	var url string

	for _, str := range cli.PathArgs {
		url += str + "/"
	}

	if cli.IsHelp {
		// TODO: is this check really needed?
		if len(url) > 0 {
			url = url[:len(url)-1] + cli.getHelpQueryParam()
		} else {
			url = cli.getHelpQueryParam()
		}
	}

	return url
}

func (cli *CLIArgs) buildBody() []byte {
	JSONObj := gabs.New()

	for k, v := range cli.BodyArgs {
		number, err := strconv.Atoi(v)
		if err == nil {
			JSONObj.SetP(number, k)
			continue
		}

		float, err := strconv.ParseFloat(v, 64)
		if err == nil {
			JSONObj.SetP(float, k)
			continue
		}

		b, err := strconv.ParseBool(v)
		if err == nil {
			JSONObj.SetP(b, k)
			continue
		}

		// TODO: what if k is ""
		JSONObj.SetP(v, k)
	}

	return JSONObj.Bytes()
}

func (cli *CLIArgs) buildSingleParamBody() (string, []byte) {
	var path string
	var body string
	var err error

	for k, v := range cli.BodyArgs {
		path = k + "/"

		if v == "" {
			body = "\"\""
			break
		}

		// if string is a number, a float or a boolean, put it in the body
		// withsout quotes
		_, err = strconv.Atoi(v)
		if err == nil {
			body = v
			break
		}

		_, err = strconv.ParseFloat(v, 64)
		if err == nil {
			body = v
			break
		}

		_, err = strconv.ParseBool(v)
		if err == nil {
			body = v
			break
		}

		// is it not qouted yet?
		if v[0] != '"' && v[len(v)-1] != '"' {
			body = "\"" + v + "\""
			break
		}

		body = v

		break
	}

	return path, []byte(body)
}

func (cli *CLIArgs) getRequestMethod() string {
	if cli.IsHelp {
		return httprequest.OptionsStr
	} else if cli.Command == AddCommand || cli.Command == "" {
		return httprequest.PostStr
	} else if cli.Command == SetCommand {
		if config.GetConfig().Version == "1" {
			return httprequest.PutStr
		} else {
			return httprequest.PatchStr
		}
	} else if cli.Command == ShowCommand {
		return httprequest.GetStr
	} else if cli.Command == DelCommand {
		return httprequest.DeleteStr
	} else if cli.Command != "" {
		// Method = "POST"
	}

	return ""
}

func (cli *CLIArgs) getHelpQueryParam() string {
	help := "?help="
	if cli.Command == "" {
		help += "NONE"
	} else {
		help += strings.ToUpper(cli.Command)
	}
	return help
}

// Creates an HTTP request based on the CLIArgs
func (cli *CLIArgs) GetHTTPRequest() (*httprequest.HTTPRequest, error) {
	var body []byte
	var url string

	url = cli.buildURL()

	// TODO: is command == "" required?
	if !cli.IsHelp && (cli.Command == AddCommand ||
		cli.Command == SetCommand || cli.Command == "") {
		if len(cli.BodyArgs) == 1 && config.GetConfig().SingleParameterWorkaround &&
			cli.Command == SetCommand {
			url0, body0 := cli.buildSingleParamBody()
			url += url0
			body = body0
		} else {
			// if there is text on the standard input that'll used as the body
			var buffer bytes.Buffer
			reader := bufio.NewReader(os.Stdin)
			text, _ := reader.ReadString('\n')
			for text != "" {
				buffer.WriteString(text)
				text, _ = reader.ReadString('\n')
			}

			if buffer.Len() > 0 {
				jsonBuff, err := yaml.YAMLToJSON(buffer.Bytes())
				if err != nil {
					return nil, err
				} else {
					body = jsonBuff
				}
			} else {
				body = cli.buildBody()
			}
		}
	}

	return &httprequest.HTTPRequest {
		Method: cli.getRequestMethod(),
		URL:    config.GetConfig().Url+url,
		Body:   body,
	}, nil
}

func (a0 *CLIArgs) Equals(a1 *CLIArgs) bool {

	if a0.Command != a1.Command {
		return false
	}

	if len(a0.PathArgs) != len(a1.PathArgs) {
		return false
	}

	for i, i0 := range a0.PathArgs {
		if a1.PathArgs[i] != i0 {
			return false
		}
	}

	return true


	//return reflect.DeepEqual(a0, a1)
}
