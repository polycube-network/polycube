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
#include "JsonNodeField.h"

#include "../../Validators/NodeValidator.h"

namespace polycube::polycubed::Rest::Resources::Body {

JsonNodeField::JsonNodeField(
    std::vector<std::shared_ptr<Validators::NodeValidator>> &&validators)
    : validators_{std::move(validators)} {}

ErrorTag JsonNodeField::Validate(const Resource *context,
                                 const std::string &cube_name,
                                 const ListKeyValues &keys) const {
  for (const auto &validator : validators_) {
    if (!validator->Validate(context, cube_name, keys))
      return ErrorTag::kBadAttribute;
  }
  return ErrorTag::kOk;
}
}  // namespace polycube::polycubed::Rest::Resources::Body
