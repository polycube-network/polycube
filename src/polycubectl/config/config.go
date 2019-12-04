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

package config

import (
	"fmt"
	"io/ioutil"
	"os"
	"strconv"
	"strings"

	"gopkg.in/yaml.v2"
)

// if no Config file is present, create a new one.
// config.yaml file is located under $HOME/.config/polycubectl

const (
	PathToConfigFile = "/.config/polycube/"
	ConfigFile       = "polycubectl_config.yaml"
)

type Config struct {
	Debug   bool   `yaml:"debug"`
	Expert  bool   `yaml:"expert"`
	Url     string `yaml:"url"`
	// enables Workaround for urls (e.g. set peer=veth1 PUT /peer/ "veth1")
	SingleParameterWorkaround bool

	CACert  string `yaml:"cacert"`
	Cert    string `yaml:"cert"`
	Key     string `yaml:"key"`
}

var (
	config Config
)

func NewDefaultConfig() Config {
	config := Config{
		Debug:                     false,
		Expert:                    true,
		Url:                       "http://localhost:9000/polycube/v1/",
		SingleParameterWorkaround: true,
	}

	return config
}

var defaultConfig = NewDefaultConfig()

func GetConfig() *Config {
	return &config
}

func saveConfig(config Config) error {
	home := os.Getenv("HOME")
	comment_ :=
`# debug: shows http method/url and body of the response
# expert: enables the possibility to add new services
# url: is the base url to contact the rest server
# cacert: path to certification authority certificate to validate polycubed identity
# cert: path to certificate to present to polycubed for validate polycubectl identity
# key: path to private key for polycubectl

`
	comment := []byte(comment_)

	bytes, err := yaml.Marshal(config)
	if err != nil {
		return fmt.Errorf("error during json Marshal: %s", err.Error())
	}

	// Create a directory for the config file with read, write and execute
	// permission for the current user (700)
	// (both w and x are needed to edit the content of the dir)
	os.MkdirAll(home+PathToConfigFile, 0700)

	f, err := os.Create(home + PathToConfigFile + ConfigFile)
	defer f.Close()
	if err != nil {
		return fmt.Errorf("error creating file: %s", err.Error())
	}

	bytes = append(comment, bytes...)

	n, err := f.Write(bytes)
	if err != nil {
		return fmt.Errorf("error writing file (wrote %d bytes): %s", n, err.Error())
	}

	return nil
}

func LoadConfig() (error) {
	home := os.Getenv("HOME")

	path := home + PathToConfigFile + ConfigFile

	file, err := ioutil.ReadFile(path)
	if err != nil {
		//return fmt.Errorf("error loading config file %s: %s", path, err.Error())
		saveConfig(defaultConfig)
		config = defaultConfig
		return nil
		// if config file is not present, create a new one with default values
		//createConfigFile()
		//return
	}

	config = defaultConfig
	err = yaml.Unmarshal(file, &config)
	if err != nil {
		return fmt.Errorf("error decoding config file: %s", err.Error())
	}

	if os.Getenv("POLYCUBECTL_URL") != "" {
		config.Url = os.Getenv("POLYCUBECTL_URL")
	}

	if os.Getenv("POLYCUBECTL_DEBUG") != "" {
		b, errb := strconv.ParseBool(os.Getenv("POLYCUBECTL_DEBUG"))
		if errb == nil {
			config.Debug = b
		}
	}

	if os.Getenv("POLYCUBECTL_EXPERT") != "" {
		b, errb := strconv.ParseBool(os.Getenv("POLYCUBECTL_EXPERT"))
		if errb == nil {
			config.Expert = b
		}
	}

	if os.Getenv("POLYCUBECTL_CACERT") != "" {
		config.CACert = os.Getenv("POLYCUBECTL_CACERT")
	}

	if os.Getenv("POLYCUBECTL_CERT") != "" {
		config.Cert = os.Getenv("POLYCUBECTL_CERT")
	}

	if os.Getenv("POLYCUBECTL_KEY") != "" {
		config.Key = os.Getenv("POLYCUBECTL_KEY")
	}

	if strings.HasPrefix(config.Url, "https") {
		if config.CACert == "" {
			return fmt.Errorf("https is used but there is not a CA cert defined\n")
		}
	} else {
		if config.CACert != "" {
			fmt.Println("Warning: CACert defined but using http")
		}

		if config.Cert != "" {
			fmt.Println("Warning: Cert defined but using http")
		}

		if config.Key != "" {
			fmt.Println("Warning: Key defined but using http")
		}
	}

	if config.Cert != "" && config.Key == "" {
		return fmt.Errorf("cert is defined but key is not.\n")
	}

	if config.Cert == "" && config.Key != "" {
		return fmt.Errorf("key is defined but cert is not.\n")
	}

	return nil
}
