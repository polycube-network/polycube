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
#include "BitsValidator.h"

#include <string>

namespace polycube::polycubed::Rest::Validators {
BitsValidator::BitsValidator() : bits_{} {}

void BitsValidator::AddBit(std::uint32_t position, const std::string &name) {
  bits_.emplace(position, name);
}

bool BitsValidator::Validate(const std::string &value) const {
  auto lpos = 0ul;
  auto rpos = value.find(' ');
  std::string current = value.substr(0, rpos);
  for (const auto &bit : bits_) {
    if (bit.second == current) {
      lpos = rpos + 1;
      rpos = value.find(' ', lpos);
      current = value.substr(lpos, rpos);
    }
  }
  return current.find(' ', lpos) != std::string::npos;
}
}  // namespace polycube::polycubed::Rest::Validators
