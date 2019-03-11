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
#include <unordered_map>
#include <vector>

#include "../../Types/Scalar.h"
#include "Resource.h"
#include "polycube/services/response.h"

namespace polycube::polycubed::Rest::Resources::Body {
class JsonValueField;

class LeafResource : public Resource {
 public:
  LeafResource(const std::string &name, const std::string &description,
               const std::string &cli_example,
               const ParentResource *const parent, PolycubedCore *core,
               std::unique_ptr<JsonValueField> &&value_fields,
               const std::vector<JsonNodeField> &node_fields,
               bool configuration, bool init_only_config, bool mandatory,
               Types::Scalar type,
               std::unique_ptr<const std::string> &&default_value);

  ~LeafResource() override = default;

  std::vector<Response> BodyValidate(const std::string &cube_name,
                                     const ListKeyValues &keys,
                                     nlohmann::json &body,
                                     bool initialization) const override;

  bool IsMandatory() const final {
    return mandatory_;
  }

  bool IsConfiguration() const final {
    return configuration_;
  }

  void SetDefaultIfMissing(nlohmann::json &body,
                           bool initialization) const override;

 protected:
  const std::unique_ptr<JsonValueField> value_field_;
  const bool mandatory_;
  const Types::Scalar type_;
  const std::unique_ptr<const std::string> default_;
};
}  // namespace polycube::polycubed::Rest::Resources::Body
