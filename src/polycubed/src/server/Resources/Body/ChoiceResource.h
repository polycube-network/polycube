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

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "ParentResource.h"

namespace polycube::polycubed::Rest::Resources::Body {
class ChoiceResource : public virtual ParentResource {
 public:
  ChoiceResource(const std::string &name, const std::string &description,
                 const std::string &cli_example,
                 const ParentResource *const parent, PolycubedCore *core,
                 bool mandatory,
                 std::unique_ptr<const std::string> &&default_case);

  std::vector<Response> BodyValidate(const std::string &cube_name,
                                     const ListKeyValues &keys,
                                     nlohmann::json &body,
                                     bool initialization) const final;

  using ParentResource::AddChild;

  bool IsMandatory() const final;

  void SetDefaultIfMissing(nlohmann::json &body) const final;

 private:
  const bool mandatory_;
  const std::unique_ptr<const std::string> default_case_;
};
}  // namespace polycube::polycubed::Rest::Resources::Body
