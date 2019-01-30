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
#include "WhenValidator.h"

#include "../Parser/XPathParserDriver.h"
#include "../Resources/Body/ListKey.h"

namespace polycube::polycubed::Rest::Validators {

WhenValidator::WhenValidator(std::string clause)
    : NodeValidator(std::move(clause)) {}

bool WhenValidator::Validate(const Resources::Body::Resource *context,
                             const std::string &cube_name,
                             const ListKeyValues &keys) const {
  return Parser::XPathParserDriver(context, keys, cube_name)
      .parse_string(clause_);
}
}  // namespace polycube::polycubed::Rest::Validators