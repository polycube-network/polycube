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

#include <pistache/router.h>

#include <memory>
#include <string>
#include <vector>

#include "polycube/services/response.h"

namespace polycube::polycubed::Rest {
namespace Resources::Body {
class Resource;
class ListKey;
}  // namespace Resources::Body

namespace Validators {
class ValueValidator;
}
}  // namespace polycube::polycubed::Rest

namespace polycube::polycubed::Rest::Resources::Endpoint {

class PathParamField {
 public:
  PathParamField(
      const std::string &name,
      std::vector<std::shared_ptr<Validators::ValueValidator>> validators);

  const std::string &Name() const;

  ErrorTag Validate(const Pistache::Rest::Request &value) const;

 private:
  const std::string name_;
  const std::vector<std::shared_ptr<Validators::ValueValidator>> validators_;
};
}  // namespace polycube::polycubed::Rest::Resources::Endpoint
