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

#include <string>

namespace polycube::polycubed::Rest::Validators {
/// ValueValidator(s) are meant for validating the value space of a given
/// input value (e.g., input is in a given range).
class ValueValidator {
 public:
  virtual bool Validate(const std::string &value) const = 0;

 protected:
  virtual ~ValueValidator() = default;
};
}  // namespace polycube::polycubed::Rest::Validators
