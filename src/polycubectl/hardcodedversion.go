/*
 * Copyright 2018 The Polycube Authors
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
	"github.com/polycube-network/polycube/src/polycubectl/httprequest"
)

var versionMap map[string]string

// by default CLI uses version provided in configuration file.
// this is a way to force CLI to use a specific version for specific services

func init() {
	versionMap = make(map[string]string)

	// MODIFY CONFIGURATION HERE:

	versionMap["bridge"] = "1"
	versionMap["firewall"] = "2"
	versionMap["ddosmitigator"] = "2"
	versionMap["lbrp"] = "2"
	versionMap["lbdsr"] = "2"
	versionMap["helloworld"] = "2"
	versionMap["pbforwarder"] = "2"
	versionMap["simpleforwarder"] = "2"
	versionMap["kubenat"] = "2"
	versionMap["nat"] = "2"
	versionMap["router"] = "2"
	versionMap["simplebridge"] = "2"

	// other services uses default option provided in config file
}

func GetHardCodedVersion(args []string) string {
	if len(args) > 0 {
		v, ok := versionMap[args[0]]
		if ok {
			return v
		}
	}

	return ""
}

// some services are using v2 of cli
// this implies that set method is PATCH instead of PUT
// connect command has to know the SetMethod (PATCH/PUT)
// given a specific service
func GetSetMethodFromServiceName(service string) string {
	v, ok := versionMap[service]
	if ok {
		if v == "2" {
			return httprequest.PatchStr
		}
		if v == "1" {
			return httprequest.PutStr
		}
	}
	return httprequest.PatchStr
}
