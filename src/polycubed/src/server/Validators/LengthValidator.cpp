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
#include "LengthValidator.h"

#include <string>
#include <unordered_map>

#include "../Server/Base64.h"

namespace polycube::polycubed::Rest::Validators {
LengthValidator::LengthValidator(bool binary) : binary_(binary), ranges_{} {}

void LengthValidator::AddRange(std::uint64_t min, std::uint64_t max) {
  ranges_.emplace(min, max);
}

void LengthValidator::AddExact(std::uint64_t exact) {
  ranges_.emplace(exact, exact);
}

void LengthValidator::AddRanges(
    std::unordered_map<std::uint64_t, std::uint64_t> ranges) {
  ranges_.merge(ranges);
}

bool LengthValidator::Validate(const std::string &value) const {
  std::size_t data_length;
  if (binary_) {
    data_length = Server::Base64::decode(value).length();
  } else {
    data_length = value.length();
  }
  unsigned falses = 0;
  for (const auto &range : ranges_) {
    if (data_length >= range.first && data_length <= range.second)
      falses += 1;
  }
  return falses != ranges_.size();
}
}  // namespace polycube::polycubed::Rest::Validators
