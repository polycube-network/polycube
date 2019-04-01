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
#include "PatternValidator.h"

#include <regex>
#include <string>

namespace polycube::polycubed::Rest::Validators {
PatternValidator::PatternValidator(const char *pattern, bool inverse)
    : pattern_(pattern, std::regex_constants::optimize |
                            std::regex_constants::ECMAScript),
      inverse_(inverse) {}

bool PatternValidator::Validate(const std::string &value) const {
  return !inverse_ == std::regex_match(value, pattern_);
}
}  // namespace polycube::polycubed::Rest::Validators
