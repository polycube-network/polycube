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

#include "ListKey.h"
#include "polycube/services/json.hpp"
#include "polycube/services/response.h"

namespace polycube::polycubed::Rest::Validators {
class NodeValidator;
}  // namespace polycube::polycubed::Rest::Validators

namespace polycube::polycubed::Rest::Resources::Body {
class Resource;

class JsonNodeField {
 public:
  explicit JsonNodeField(
      std::vector<std::shared_ptr<Validators::NodeValidator>> &&validators);

  ErrorTag Validate(const Resource *context, const std::string &cube_name,
                    const ListKeyValues &keys) const;

 private:
  const std::vector<std::shared_ptr<Validators::NodeValidator>> validators_;
};
}  // namespace polycube::polycubed::Rest::Resources::Body
