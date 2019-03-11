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
#include "EnumValidator.h"

#include <algorithm>
#include <string>

namespace polycube::polycubed::Rest::Validators {
EnumValidator::EnumValidator() : values_{} {}

void EnumValidator::AddEnum(const std::string &value) {
  std::string upper;
  upper.reserve(value.length());
  std::transform(std::begin(value), std::end(value), std::back_inserter(upper),
                 static_cast<int (*)(int)>(std::tolower));
  values_.emplace(upper);
}

bool EnumValidator::Validate(const std::string &value) const {
  std::string upper;
  upper.reserve(value.length());
  std::transform(std::begin(value), std::end(value), std::back_inserter(upper),
                 static_cast<int (*)(int)>(std::tolower));
  return values_.count(upper) == 1;
}
}  // namespace polycube::polycubed::Rest::Validators
