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
#include "UnionValidator.h"

#include <memory>
#include <string>
#include <typeindex>
#include <vector>

#include "../Resources/Body/JsonValueField.h"

namespace polycube::polycubed::Rest::Validators {
UnionValidator::UnionValidator() : map_{} {}

void UnionValidator::AddType(
    std::type_index type,
    const std::vector<std::shared_ptr<ValueValidator> > &validators) {
  map_.emplace(type, validators);
}

bool UnionValidator::Validate(const std::string &value) const {
  auto data = nlohmann::json::parse(value);
  const auto &allowed =
      Resources::Body::JsonValueField::AcceptableTypes(data.type());
  for (const auto &union_type : map_) {
    if (allowed.count(union_type.first) != 0) {
      for (const auto &validator : union_type.second) {
        if (validator->Validate(value))
          return true;
      }
    }
  }
  return false;
}
}  // namespace polycube::polycubed::Rest::Validators
