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
#include "LeafListResource.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "JsonNodeField.h"
#include "JsonValueField.h"

namespace polycube::polycubed::Rest::Resources::Body {

LeafListResource::LeafListResource(
    const std::string &name, const std::string &description,
    const std::string &cli_example, const Body::ParentResource *parent,
    PolycubedCore *core, std::unique_ptr<JsonValueField> &&value_field,
    const std::vector<JsonNodeField> &node_fields, bool configuration,
    bool init_only_config, bool mandatory, Types::Scalar type,
    std::vector<std::string> &&default_value)
    : LeafResource(name, description, cli_example, parent, core,
                   std::move(value_field), node_fields, configuration,
                   init_only_config, mandatory, type, nullptr),
      default_(std::move(default_value)) {}

std::vector<Response> LeafListResource::BodyValidate(
    const std::string &cube_name, const ListKeyValues &keys,
    nlohmann::json &body, bool initialization) const {
  std::vector<Response> errors;
  if (body.type() != nlohmann::detail::value_t::array) {
    errors.push_back({ErrorTag::kBadAttribute, ::strdup(name_.data())});
    return errors;
  }

  for (const auto &node_field : node_fields_) {
    const auto node_validation = node_field.Validate(this, cube_name, keys);
    if (node_validation != kOk) {
      errors.push_back({node_validation, ::strdup(name_.data())});
      return errors;
    }
  }

  for (auto &element : body.items()) {
    const auto &inner_errors = LeafResource::BodyValidate(
        cube_name, keys, element.value(), initialization);
    errors.reserve(errors.size() + inner_errors.size());
    std::move(std::begin(inner_errors), std::end(inner_errors),
              std::back_inserter(errors));
  }
  return errors;
}

void LeafListResource::SetDefaultIfMissing(nlohmann::json &body) const {
  if (body.empty() && !default_.empty()) {
    for (const auto &element : default_) {
      body.push_back(element);
    }
  }
}
}  // namespace polycube::polycubed::Rest::Resources::Body
