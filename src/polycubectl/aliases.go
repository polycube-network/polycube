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
	"fmt"
	//"reflect"
	"github.com/polycube-network/polycube/src/polycubectl/cliargs"
)

const (
	NetdevsCommandStr    = "netdevs"
	CubesCommandStr      = "cubes"
	TopologyCommandStr   = "topology"
	ConnectCommandStr    = "connect"
	DisconnectCommandStr = "disconnect"
	ServicesCommandStr   = "services"
	VersionCommandStr    = "version"
)

/*
 * Alias is way to transform a cliargs into another.
 * Match: Checks if the cliargs matches this alias. It returns true if there is
 * a match. If there is a match but the syntax is not correct it should return
 * error that will be printed in the screen.
 *
 * transform: Takes as input the cliargs that matched and should transfor it in the
 * new one. If returns true the cli stops after it and the request is not send to the
 * server
 */

type AliasConverter func(args *cliargs.CLIArgs) bool
type AliasMatch func(args *cliargs.CLIArgs) (bool, error)

type Alias struct {
	match     AliasMatch
	transform AliasConverter
}

var Aliases []Alias

func FillAliases() {
	// version -> print cli version + show version
	version_alias := Alias {
		match: func(args *cliargs.CLIArgs) (bool, error) {
			if len(args.PathArgs) == 0 {
				return false, nil
			}

			if args.PathArgs[0] != VersionCommandStr {
				return false, nil
			}

			if args.IsHelp {
				return false, fmt.Errorf("There is not help for this command")
			}

			if len(args.PathArgs) != 1 {
				return false, fmt.Errorf("Bad syntax, usage: version [show]")
			}

			return true, nil
		},
		transform: func(args *cliargs.CLIArgs) bool {
			Buffer += fmt.Sprintf("polycubectl:\n version: %s\n", Version)
			args.Command = cliargs.ShowCommand
			return false
		},
	}
	Aliases = append(Aliases, version_alias)

	// services [add, del, show] ? -> print service [add, del, show] help
	services_help_alias := Alias {
		match: func(args *cliargs.CLIArgs) (bool, error) {
			if len(args.PathArgs) == 0 {
				return false, nil
			}

			if 	args.PathArgs[0] != ServicesCommandStr ||
				!args.IsHelp ||	len(args.PathArgs) != 1 {
				return false, nil
			}

			return true, nil
		},
		transform: func(args *cliargs.CLIArgs) bool {
			ServicesHelp(args.Command, args.PathArgs)
			return true
		},
	}
	Aliases = append(Aliases, services_help_alias)

	// netdevs -> netdevs show
	netdevs_alias := Alias {
		match: func(args *cliargs.CLIArgs) (bool, error) {
			math := &cliargs.CLIArgs {
				Command: "",
				PathArgs: []string{NetdevsCommandStr},
			}
			return args.Equals(math), nil
		},
		transform: func(args *cliargs.CLIArgs) bool {
			args.Command = cliargs.ShowCommand
			return false
		},
	}
	Aliases = append(Aliases, netdevs_alias)

	// cubes -> cubes show
	cubes_alias := Alias {
		match: func(args *cliargs.CLIArgs) (bool, error) {
			math := &cliargs.CLIArgs {
				Command: "",
				PathArgs: []string{CubesCommandStr},
			}
			return args.Equals(math), nil
		},
		transform: func(args *cliargs.CLIArgs) bool {
			args.Command = cliargs.ShowCommand
			return false
		},
	}
	Aliases = append(Aliases, cubes_alias)

	// topology -> topology show -verbose -hide=uuid
	topology_alias := Alias {
		match: func(args *cliargs.CLIArgs) (bool, error) {
			math := &cliargs.CLIArgs {
				Command: "",
				PathArgs: []string{TopologyCommandStr},
			}
			return args.Equals(math), nil
		},
		transform: func(args *cliargs.CLIArgs) bool {
			args.Command = cliargs.ShowCommand
			args.ShowType = cliargs.ShowTypeVerbose
			args.HideList = map[string]bool {"uuid": true}
			return false
		},
	}
	Aliases = append(Aliases, topology_alias)

	// connect | disconnect ? -> print help for connect | disconnect
	connect_help_alias := Alias {
		match: func(args *cliargs.CLIArgs) (bool, error) {
			if len(args.PathArgs) == 0 {
				return false, nil
			}

			if args.PathArgs[0] != ConnectCommandStr && args.PathArgs[0] != DisconnectCommandStr{
				return false, nil
			}

			if !args.IsHelp {
				return false, nil
			}

			if len(args.PathArgs) != 1 {
				return false, fmt.Errorf("Bad syntax, usage: connect peer1 peer2")
			}

			return true, nil
		},
		transform: func(args *cliargs.CLIArgs) bool {
			printConnectHelp(args.PathArgs)
			return true
		},
	}
	Aliases = append(Aliases, connect_help_alias)

	// connect a b -> connect add peer1=a peer2=b
	connect_alias := Alias {
		match: func(args *cliargs.CLIArgs) (bool, error) {
			if len(args.PathArgs) == 0 {
				return false, nil
			}

			if args.PathArgs[0] != ConnectCommandStr {
				return false, nil
			}

			if len(args.PathArgs) != 3 {
				return false, fmt.Errorf("Bad syntax, usage: connect peer1 peer2")
			}

			return true, nil
		},
		transform: func(args *cliargs.CLIArgs) bool {
			args.Command = cliargs.AddCommand
			args.BodyArgs = map[string]string{
				"peer1": args.PathArgs[1],
				"peer2": args.PathArgs[2],
			}
			args.PathArgs = []string{ConnectCommandStr}
			return false
		},
	}
	Aliases = append(Aliases, connect_alias)

	// disconnect a b -> disconnect add peer1=a peer2=b
	disconnect_alias := Alias {
		match: func(args *cliargs.CLIArgs) (bool, error) {
			if len(args.PathArgs) == 0 {
				return false, nil
			}

			if args.PathArgs[0] != DisconnectCommandStr {
				return false, nil
			}

			if len(args.PathArgs) != 3 {
				return false, fmt.Errorf("Bad syntax, usage: connect peer1 peer2")
			}

			return true, nil
		},
		transform: func(args *cliargs.CLIArgs) bool {
			args.Command = cliargs.AddCommand
			args.BodyArgs = map[string]string{
				"peer1": args.PathArgs[1],
				"peer2": args.PathArgs[2],
			}
			args.PathArgs = []string{"disconnect"}
			return false
		},
	}
	Aliases = append(Aliases, disconnect_alias)
}

// Loops through all registered aliases
func RunAliases(cliArgs *cliargs.CLIArgs) (error, bool) {
	for _, alias := range Aliases {
		if m, err := alias.match(cliArgs); err != nil {
			return err, true
		} else if (m) {
			if alias.transform(cliArgs) {
				return nil, true
			}
			break
		}
	}

	return nil, false
}
