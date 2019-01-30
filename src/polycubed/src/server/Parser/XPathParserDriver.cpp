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
#include "XPathParserDriver.h"

#include <sstream>

#include "../Resources/Body/ListKey.h"

namespace polycube::polycubed::Rest::Parser {
XPathParserDriver::XPathParserDriver(const Resources::Body::Resource *caller,
                                     ListKeyValues keys, std::string cube_name)
    : trace_scanning(false),
      trace_parsing(false),
      lexer{},
      current(caller),
      keys(std::move(keys)),
      current_cube_name(std::move(cube_name)) {}

bool XPathParserDriver::parse_stream(std::istream &in) {
  lexer = std::make_unique<XPathScanner>(&in);
  lexer->set_debug(trace_scanning);

  XPathParser parser{shared_from_this()};
  return parser.parse() == 0;
}

bool XPathParserDriver::parse_string(const std::string &input) {
  std::istringstream iss{input};
  return parse_stream(iss);
}

void XPathParserDriver::error(const std::string &m) {
  std::cerr << m << std::endl;
}
}  // namespace polycube::polycubed::Rest::Parser
