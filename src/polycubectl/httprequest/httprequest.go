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

package httprequest

import (
	"bytes"
	"fmt"
	"net/http"
	"crypto/tls"
	"crypto/x509"
	"io/ioutil"

	"github.com/polycube-network/polycube/src/polycubectl/config"
)

const (
	GetStr     = "GET"
	PutStr     = "PUT"
	PatchStr   = "PATCH"
	PostStr    = "POST"
	DeleteStr  = "DELETE"
	OptionsStr = "OPTIONS"
)

type HTTPRequest struct {
	Method string
	URL    string
	Body   []byte
}

func (req *HTTPRequest) ToString() string {
	return fmt.Sprintf("%s %s\n%s\n\n", req.Method, req.URL, req.Body)
}

func (req *HTTPRequest) Perform() (response *http.Response, err error) {
	var resp *http.Response
	var e error
	var req0 *http.Request
	var cer tls.Certificate

	tlsConf := &tls.Config{}
	client := &http.Client{}

	if config.GetConfig().CACert != "" {
		// load root CA certificate to validate server identity
		rootcaCert, err := ioutil.ReadFile(config.GetConfig().CACert)
		if err != nil {
			return nil, err
		}
		rootcaCertPool := x509.NewCertPool()
		rootcaCertPool.AppendCertsFromPEM(rootcaCert)
		tlsConf.RootCAs = rootcaCertPool

		// present certificate to server for client authentication
		if config.GetConfig().Cert != "" {
			cer, e = tls.LoadX509KeyPair(config.GetConfig().Cert, config.GetConfig().Key)
			if err != nil {
				return nil, e
			}

			tlsConf.Certificates = []tls.Certificate{cer}
		}
		tr := &http.Transport{TLSClientConfig: tlsConf}
		client.Transport = tr
	}

	switch req.Method {
	case "", "POST":
		resp, e = client.Post(req.URL, "application/json", bytes.NewReader(req.Body))
	case "GET":
		resp, e = client.Get(req.URL)
	default:
		req0, e = http.NewRequest(req.Method, req.URL, bytes.NewReader(req.Body))
		if e != nil {
			// fmt.Printf("%s\n", e)
		}
		resp, e = client.Do(req0)
	}

	if e != nil {
		// fmt.Println(e)
		return nil, e
	}
	return resp, err
}
