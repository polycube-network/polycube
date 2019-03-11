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
#pragma once

#include <memory>
#include <string>

#include "../Resources/Body/ListKey.h"
#include "XPathScanner.h"

namespace polycube::polycubed::Rest::Resources::Body {
class Resource;
}

namespace polycube::polycubed::Rest::Parser {
class XPathParserDriver : std::enable_shared_from_this<XPathParserDriver> {
 public:
  XPathParserDriver(const Resources::Body::Resource *caller, ListKeyValues keys,
                    std::string cube_name);

  /// enable debug output in the flex scanner
  bool trace_scanning;

  /// enable debug output in the bison parser
  bool trace_parsing;

  /** Invoke the scanner and parser on an input string.
   * @param input	input string
   * @param sname	stream name for error messages
   * @return		true if successfully parsed
   */
  bool parse_string(const std::string &input);

  /** General error handling. This can be modified to output the error
   * e.g. to a dialog box. */
  void error(const std::string &m);

  /** Pointer to the current lexer instance, this is used to connect the
   * parser to the scanner. It is used in the yylex macro. */
  std::unique_ptr<XPathScanner> lexer;

  const Resources::Body::Resource *current;
  ListKeyValues keys;
  std::string current_cube_name;

 private:
  /** Invoke the scanner and parser for a stream.
   * @param in	input stream
   * @return		true if successfully parsed
   */
  bool parse_stream(std::istream &in);
};
}  // namespace polycube::polycubed::Rest::Parser
