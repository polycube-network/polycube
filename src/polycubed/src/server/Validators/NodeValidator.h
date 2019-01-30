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
#include <vector>

#include "../Resources/Body/ListKey.h"

namespace polycube::polycubed::Rest::Resources::Body {
class Resource;
}

namespace polycube::polycubed::Rest::Validators {

/// NodeValidator(s) are meant for identifing whether a node can be part of
/// the input body (i.e., must and when statements)
class NodeValidator {
 public:
  explicit NodeValidator(std::string clause);
  virtual bool Validate(const Resources::Body::Resource *context,
                        const std::string &cube_name,
                        const ListKeyValues &keys) const = 0;

 protected:
  const std::string clause_;

  virtual ~NodeValidator() = default;
};
}  // namespace polycube::polycubed::Rest::Validators
