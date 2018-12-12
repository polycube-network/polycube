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

	"github.com/polycube-network/polycube/src/polycubectl/cliargs"
	"github.com/polycube-network/polycube/src/polycubectl/config"
	"github.com/polycube-network/polycube/src/polycubectl/httprequest"

	"github.com/Jeffail/gabs2"
	"github.com/iancoleman/orderedmap"
	"github.com/ryanuber/columnize"
)

var (
	ElementsType        string
	ElementsDescription string
	IsRootHelp          = false
	Output              = [][]string{}
)

// "hola" -> hola
func removeQuotes(input string) string {
	return input[1 : len(input)-1]
}

func typeStr(child *gabs2.Container) string {
	if child.S("type").String() == "\"leaf\"" || child.S("type").String() == "\"key\"" || child.S("type") == nil {
		return removeQuotes(child.S("simpletype").String())
	} else if child.S("type").String() == "\"list\"" {
		return "list"
	} else if child.S("type").String() == "\"complex\"" {
		return "container"
	} else {
		return ""
	}
}

func getCommandDesc(command string) string {
	if command == cliargs.AddCommand {
		return "Add entry to a list"
	} else if command == cliargs.SetCommand {
		return "Set a value"
	} else if command == cliargs.DelCommand {
		return "Delete entry of a list"
	} else if command == cliargs.ShowCommand {
		return fmt.Sprintf("Show entry or list [%s | %s | %s | %s | %s]",
			cliargs.ShowNormal, cliargs.ShowBrief, cliargs.ShowVerbose,
			cliargs.ShowJson, cliargs.ShowYaml)
	} else {
		return ""
	}
}

func printHelpNotSupported() {
	Buffer += fmt.Sprintf("\nThere is not help for this command\n")
}

func getElementsTypeDescription(jsonParsed *gabs2.Container) {
	children, _ := jsonParsed.S("params").ChildrenMap()
	if children == nil {
		return
	}
	for _, key := range children.Keys() {
		child_, _ := children.Get(key)
		child := child_.(*gabs2.Container)
		if child.S("type").String() == "\"key\"" {
			for _, key := range children.Keys() {
				child_, _ := children.Get(key)
				child := child_.(*gabs2.Container)
				if child.S("type").String() == "\"key\"" {
					ElementsType = ElementsType + "<" + key + "> "
					ElementsDescription = ElementsDescription + removeQuotes(child.S("description").String()) + " "
				}
			}
		}
	}
}

func printRootHelp() {
	Buffer += fmt.Sprintf("For help on keywords use '?'\n")
}

func printHeader() {
	Output = append(Output, []string{"Keyword", "Type", "Description"})
}

func printHelp(cliArgs *cliargs.CLIArgs, jsonParsed *gabs2.Container) {
	if config.GetConfig().Debug {
		Buffer += fmt.Sprintf("%s\n", jsonParsed.StringIndent("", "  "))
	}

	if IsRootHelp {
		printRootHelp()
	}

	Buffer += fmt.Sprintf("\n")

	countLines := 0

	example := "polycubectl "
	for _, path := range cliArgs.PathArgs {
		if len(path) > 0 {
			example += path + " "
		}
	}

	example += cliArgs.Command + " "

	if cliArgs.Command == cliargs.AddCommand {
		children, _ := jsonParsed.S("params").ChildrenMap()
		if len(children.Keys()) > 0 {
			printHeader()
			countLines++
		}
		for _, key := range children.Keys() {
			child_, _ := children.Get(key)
			child := child_.(*gabs2.Container)
			Output = append(Output, []string{"<" + key + ">",
				typeStr(child),
				removeQuotes(child.S("description").String())})
			example += removeQuotes(child.S("example").String()) + " "
		}

		children, _ = jsonParsed.S("optional-params").ChildrenMap()
		if len(children.Keys()) > 0 {
			Output = append(Output, []string{""})
			Output = append(Output, []string{"Other parameters:"})
		}
		for _, key := range children.Keys() {
			child_, _ := children.Get(key)
			child := child_.(*gabs2.Container)
			Output = append(Output, []string{key + "=value",
				typeStr(child),
				removeQuotes(child.S("description").String())})
			param_example := removeQuotes(child.S("example").String())
			if param_example != "" {
				example += key + "=" + param_example + " "
			}
		}
	}

	if cliArgs.Command == cliargs.ShowCommand {
		count := 0
		children, _ := jsonParsed.S("params").ChildrenMap()
		if children != nil {
			for _, key := range children.Keys() {
				child_, _ := children.Get(key)
				child := child_.(*gabs2.Container)
				if child.S("type").String() != "\"key\"" {
					count++
				}
			}
			if count > 0 {
				printHeader()
				countLines++
				for _, key := range children.Keys() {
					child_, _ := children.Get(key)
					child := child_.(*gabs2.Container)
					if child.S("type").String() != "\"key\"" {
						Output = append(Output, []string{key,
							typeStr(child),
							removeQuotes(child.S("description").String())})
					}
				}
			}
		}

		elements, _ := jsonParsed.S("elements").Children()
		if len(elements) > 0 {
			if countLines == 0 {
				printHeader()
				countLines++
			}

			//omap := elements[0].Data().(orderedmap.OrderedMap)
			//for _, key := range omap.Keys() {
			//	v, _ := omap.Get(key)
			//	example += v.(string) + " "
			//}

			getElementsTypeDescription(jsonParsed)
			for _, child := range elements {
				var elementStr string
				var keyStr string
				omap := child.Data().(orderedmap.OrderedMap)
				for _, key := range omap.Keys() {
					v, _ := omap.Get(key)
					elementStr = elementStr + v.(string) + " "
					keyStr = keyStr + "<" + key + "> "
				}

				Output = append(Output, []string{elementStr, keyStr, ""})
			}
		} else {
			children, _ := jsonParsed.S("params").ChildrenMap()
			if children != nil {
				if len(children.Keys()) > 0 {
					if countLines == 0 {
						printHeader()
					}
					countLines++
				}
				for _, key := range children.Keys() {
					child_, _ := children.Get(key)
					child := child_.(*gabs2.Container)
					if child.S("type").String() == "\"key\"" {
						Output = append(Output, []string{"<" + key + ">",
							typeStr(child),
							removeQuotes(child.S("description").String())})
						//example += removeQuotes(child.S("example").String()) + " "
					}
				}
			}
		}
	}

	if cliArgs.Command == cliargs.SetCommand {
		children, _ := jsonParsed.S("params").ChildrenMap()
		if len(children.Keys()) > 0 {
			printHeader()
			countLines++
		}
		for _, key := range children.Keys() {
			child_, _ := children.Get(key)
			child := child_.(*gabs2.Container)
			Output = append(Output, []string{key + "=value",
				typeStr(child),
				removeQuotes(child.S("description").String())})
		}
	}

	if cliArgs.Command == cliargs.DelCommand {
		elements, _ := jsonParsed.S("elements").Children()
		if len(elements) > 0 {
			if countLines == 0 {
				printHeader()
				countLines++
			}

			omap := elements[0].Data().(orderedmap.OrderedMap)
			for _, key := range omap.Keys() {
				v, _ := omap.Get(key)
				example += v.(string) + " "
			}

			getElementsTypeDescription(jsonParsed)
			for _, child := range elements {
				var elementStr string
				var keyStr string
				omap := child.Data().(orderedmap.OrderedMap)
				for _, key := range omap.Keys() {
					v, _ := omap.Get(key)
					elementStr = elementStr + v.(string) + " "
					keyStr = keyStr + "<" + key + "> "
				}
				Output = append(Output, []string{elementStr, keyStr, ""})
			}
		} else {
			children, _ := jsonParsed.S("params").ChildrenMap()
			if len(children.Keys()) > 0 {
				printHeader()
				countLines++
			}
			for _, key := range children.Keys() {
				child_, _ := children.Get(key)
				child := child_.(*gabs2.Container)
				Output = append(Output, []string{"<" + key + ">",
					typeStr(child),
					removeQuotes(child.S("description").String())})
				example += removeQuotes(child.S("example").String()) + " "
			}
		}
	}

	if cliArgs.Command == "" {
		commands, _ := jsonParsed.S("commands").Children()
		if len(commands) > 0 {
			printHeader()
			countLines++
		}
		for _, child := range commands {
			Output = append(Output, []string{child.Data().(string),
				"command",
				getCommandDesc(child.Data().(string))})
		}

		valuePost := ""
		children, err := jsonParsed.S("params").ChildrenMap()
		if err != nil {
			children, err = jsonParsed.S("in").ChildrenMap()
			valuePost = "=value"
		}

		if err == nil {
			count := 0
			for _, key := range children.Keys() {
				child_, _ := children.Get(key)
				child := child_.(*gabs2.Container)
				if child.S("type").String() != "\"key\"" {
					count++
				}
			}
			if count > 0 {
				if countLines == 0 {
					printHeader()
					countLines++
				}
				for _, key := range children.Keys() {
					child_, _ := children.Get(key)
					child := child_.(*gabs2.Container)
					if child.S("type").String() != "\"key\"" {
						Output = append(Output, []string{key + valuePost,
							typeStr(child),
							removeQuotes(child.S("description").String())})
					}
				}
			}
		}

		elements, _ := jsonParsed.S("elements").Children()
		if len(elements) > 0 {
			if countLines == 0 {
				printHeader()
				countLines++
			}
			getElementsTypeDescription(jsonParsed)
			for _, child := range elements {
				var elementStr string
				var keyStr string
				omap := child.Data().(orderedmap.OrderedMap)
				for _, key := range omap.Keys() {
					v, _ := omap.Get(key)
					elementStr = elementStr + v.(string) + " "
					keyStr = keyStr + "<" + key + "> "
				}
				Output = append(Output, []string{elementStr, keyStr, ""})
			}
		} else {
			children, err := jsonParsed.S("params").ChildrenMap()
			if err == nil {
				if len(children.Keys()) > 0 {
					if countLines == 0 {
						printHeader()
					}
					countLines++
				}
				for _, key := range children.Keys() {
					child_, _ := children.Get(key)
					child := child_.(*gabs2.Container)
					if child.S("type").String() == "\"key\"" {
						Output = append(Output, []string{"<" + key + ">",
							typeStr(child),
							removeQuotes(child.S("description").String())})
					}
				}
			}
		}

		actions, _ := jsonParsed.S("actions").Children()
		if len(actions) > 0 {
			if countLines == 0 {
				printHeader()
				countLines++
			}
			for _, child := range actions {
				Output = append(Output, []string{child.Data().(string),
					"action",
					getCommandDesc(child.Data().(string))})
			}
		}

		// hardcoded root help for connect/disconnect commands
		if len(cliArgs.PathArgs) == 0 && cliArgs.Command == "" {
			Output = append(Output, []string{""})
			Output = append(Output, []string{"connect", "command", "Connect ports"})
			Output = append(Output, []string{"disconnect", "command", "Disconnect ports"})
			Output = append(Output, []string{""})

			// hardcoded root help for services/cubes/netdevs commands
			if config.GetConfig().Expert {
				Output = append(Output, []string{"services", "command", "Show/Add/Del services (e.g. Bridge, Router, ..)"})
			}
			Output = append(Output, []string{"cubes", "command", "Show running service instances (e.g. br1, nat2, ..)"})
			Output = append(Output, []string{"topology", "command", "Show topology of service instances"})
			Output = append(Output, []string{"netdevs", "command", "Show net devices available"})

			cubeList := getCubesList()
			if len(cubeList) > 0 {
				Output = append(Output, []string{""})
				Output = append(Output, []string{"Running Services", "Type"})

				for servicename, cubes := range cubeList {
					for _, cubename := range cubes {
						Output = append(Output, []string{cubename, servicename, "Running Service instance"})
					}
				}
			}
		}
	}

	if countLines == 0 {
		printHelpNotSupported()
	}

	config := columnize.DefaultConfig()
	config.NoTrim = true

	// add a space at the columns from the second row.
	for i, row := range Output {
		if i == 0 {
			continue
		}
		// Use it in case the space should be added only to the first column
		//Output[i] = append([]string{" " + row[0]}, row[1:]...)
		newRow := []string{}
		for _, col := range row {
			newRow = append(newRow, " "+col)
		}
		Output[i] = newRow
	}

	result := columnize.Format(Output, config)
	Buffer += result
	if cliArgs.Command == cliargs.AddCommand || cliArgs.Command == cliargs.DelCommand {
		Buffer += fmt.Sprintf("\nExample: \n %s", example)
	}
	Buffer += "\n"
}

func getCubesList() map[string][]string {
	list := make(map[string][]string)

	req := httprequest.HTTPRequest {
		Method: httprequest.GetStr,
		URL:    config.GetConfig().Url+"/cubes/",
	}

	response, error := req.Perform()
	if error != nil {
		Buffer += fmt.Sprintln(error)
		ExitAndLog(ErrHttpRequestFailed)
	}

	body, _ := ioutil.ReadAll(response.Body)

	jsonParsed, errJSON := gabs2.ParseJSON([]byte(body))
	if errJSON != nil {
		if config.GetConfig().Debug {
			Buffer += fmt.Sprintln(errJSON)
		}
		ExitAndLog(ErrHttpRequestFailed)
	}

	services, _ := jsonParsed.ChildrenMap()

	// foreach service name
	for _, key := range services.Keys() {
		cubes_, _ := services.Get(key)
		cubes, _ := cubes_.(*gabs2.Container).Children()
		var a []string

		for _, cube_ := range cubes {
			cube := cube_.Data().(orderedmap.OrderedMap)
			name, _ := cube.Get("name")
			a = append(a, name.(string))
		}

		list[key] = a
	}
	return list
}
