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

#include "polycube/services/http.h"

namespace polycube {
namespace service {

HttpHandleRequest::HttpHandleRequest() {}
HttpHandleRequest::HttpHandleRequest(Method method, const std::string &url,
                                     const std::string &body,
                                     HelpType help_type)
    : method_(method), url_(url), body_(body), help_type_(help_type) {}
HttpHandleRequest::~HttpHandleRequest() {}

std::string HttpHandleRequest::resource() const {
  return url_;
}
Method HttpHandleRequest::method() const {
  return method_;
}
std::string HttpHandleRequest::body() const {
  return body_;
}
HelpType HttpHandleRequest::help_type() const {
  return help_type_;
}
HttpHandleResponse::HttpHandleResponse() {}
HttpHandleResponse::~HttpHandleResponse() {}

std::string HttpHandleResponse::body() const {
  return body_;
}
void HttpHandleResponse::set_body(const std::string &body) {
  body_ = body;
}

Code HttpHandleResponse::code() const {
  return code_;
}
void HttpHandleResponse::set_code(Code code) {
  code_ = code;
}

// to be compatible with pistache
void HttpHandleResponse::send(Code code, const std::string &body) {
  code_ = code;
  body_ = body;
}

void HttpHandleResponse::send(Code code) {
  code_ = code;
}

}  // namespace service
}  // namespace polycube
