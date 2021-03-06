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

#include <libyang/libyang.h>

#include <memory>
#include <string>
#include <typeindex>
#include <unordered_set>
#include <utility>
#include <vector>

#include "polycube/services/json.hpp"
#include "polycube/services/response.h"

namespace polycube::polycubed::Rest {
namespace Resources::Body {
class Resource;
class ListKey;
}

namespace Validators {
class ValueValidator;
}
}

namespace polycube::polycubed::Rest::Resources::Body {

class JsonValueField {
 public:
  JsonValueField();

  JsonValueField(
      LY_DATA_TYPE type,
      std::vector<std::shared_ptr<Validators::ValueValidator>> &&validators);

  ErrorTag Validate(const nlohmann::json &value) const;

  static const std::unordered_set<std::type_index> AcceptableTypes(
      nlohmann::detail::value_t type);

 private:
  const std::unordered_set<nlohmann::detail::value_t> allowed_types_;
  const std::vector<std::shared_ptr<Validators::ValueValidator>> validators_;

  static const std::unordered_set<nlohmann::detail::value_t> FromYangType(
      LY_DATA_TYPE type);
};
}  // namespace polycube::polycubed::Rest::Resources::Body
