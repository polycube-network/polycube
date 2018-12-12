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

#pragma once

#include <map>
#include <string>
#include "http_defs.h"

namespace polycube {
namespace service {

using Http::Method;
using Http::Code;

/*
 * HttpHandleRequest is used to pass http requests to the service control api.
 * this class is still under contruction.
 */

enum class HelpType {
  NO_HELP,
  NONE,
  SHOW,
  SET,
  DEL,
  ADD
};

class HttpHandleRequest {
 public:
  HttpHandleRequest();
  // TODO: add headers to this constructor
  HttpHandleRequest(Method method, const std::string &url,
                   const std::string &body, HelpType help_type = HelpType::NO_HELP);
  virtual ~HttpHandleRequest();

  std::string resource() const;
  Method method() const;
  std::string body() const;
  HelpType help_type() const;

 private:
  Method method_;
  std::string url_;
  std::map<std::string, std::string> headers_;
  std::string body_;
  HelpType help_type_;
};

/*
 * HttpHandleResponse is used by the control api of the services to answer an
 * http request.
 * this class is still under contruction.
 */
class HttpHandleResponse {
 public:
  HttpHandleResponse();
  virtual ~HttpHandleResponse();

  std::string body() const;
  void set_body(const std::string &body);

  Code code() const;
  void set_code(Code code);

  // to be compatible with pistache
  void send(Code code, const std::string &body);
  void send(Code code);

 private:
  Code code_;
  std::map<std::string, std::string> headers_;
  std::string body_;
};

}  // namespace service
}  // namespace polycube
