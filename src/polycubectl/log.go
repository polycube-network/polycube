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
	"github.com/natefinch/lumberjack"
	"log"
	"os"
)

var PathToLogFile = "/var/log/polycube/"
var LogFilename = "polycubectl.log"
var LogAnswerFilename = "polycubectl_response.log"

func LogCommand(args []string) {
	var cmdLog *log.Logger

	f, err := os.OpenFile(PathToLogFile+LogFilename, os.O_WRONLY|os.O_CREATE|os.O_APPEND, 0644)

	if err != nil {
		log.Fatal(err)
	}

	//defer to close when you're done with it
	defer f.Close()

	cmdLog = log.New(f, "", log.Ldate|log.Ltime)
	cmdLog.SetOutput(&lumberjack.Logger{
		Filename:   PathToLogFile + LogFilename,
		MaxSize:    1,  // megabytes after which new file is created
		MaxBackups: 3,  // number of backups
		MaxAge:     28, //days
	})

	cmdLog.Printf("%v", args)
}

func LogCommandAndAnswer(args []string, response string) {
	var cmdLog *log.Logger

	f, err := os.OpenFile(PathToLogFile+LogAnswerFilename, os.O_WRONLY|os.O_CREATE|os.O_APPEND, 0644)

	if err != nil {
		log.Fatal(err)
	}

	//defer to close when you're done with it
	defer f.Close()

	cmdLog = log.New(f, "", log.Ldate|log.Ltime)
	cmdLog.SetOutput(&lumberjack.Logger{
		Filename:   PathToLogFile + LogAnswerFilename,
		MaxSize:    1,  // megabytes after which new file is created
		MaxBackups: 3,  // number of backups
		MaxAge:     28, //days
	})

	cmdLog.Printf("%v\n%s", args, response)
}
