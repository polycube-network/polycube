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

#include "LeafResource.h"
#include "JsonNodeField.h"
#include "JsonValueField.h"

namespace polycube::polycubed::Rest::Resources::Body {
using Pistache::Http::ResponseWriter;
using Pistache::Rest::Request;

LeafResource::LeafResource(
    const std::string &name, const std::string &description,
    const std::string &cli_example, const ParentResource *const parent,
    PolycubedCore *core, std::unique_ptr<JsonValueField> &&value_field,
    const std::vector<JsonNodeField> &node_fields, bool configuration,
    bool init_only_config, bool mandatory, Types::Scalar type,
    std::unique_ptr<const std::string> &&default_value)
    : Resource(name, description, cli_example, parent, core, configuration,
               init_only_config, node_fields),
      value_field_{std::move(value_field)},
      mandatory_(mandatory),
      type_(type),
      default_{std::move(default_value)} {}

std::vector<Response> LeafResource::BodyValidate(const std::string &cube_name,
                                                 const ListKeyValues &keys,
                                                 nlohmann::json &body,
                                                 bool initialization) const {
  std::vector<Response> errors;
  if (body.empty()) {
    errors.push_back({ErrorTag::kMissingAttribute, ::strdup(name_.data())});
    return errors;
  }

  for (const auto &node_field : node_fields_) {
    const auto node_validation = node_field.Validate(this, cube_name, keys);
    if (node_validation != kOk) {
      errors.push_back({node_validation, ::strdup(name_.data())});
      return errors;
    }
  }

  auto value_field = value_field_->Validate(body);
  if (value_field != kOk) {
    errors.push_back({value_field, ::strdup(name_.data())});
  }
  return errors;
}

void LeafResource::SetDefaultIfMissing(nlohmann::json &body,
                                       bool initialization) const {
  if (body.empty() && default_ != nullptr) {
    switch (type_) {
    case Types::Scalar::Integer:
      body = std::stoll(*default_);
      break;
    case Types::Scalar::Unsigned:
      body = std::stoull(*default_);
      break;
    case Types::Scalar::Decimal:
      body = std::stod(*default_);
      break;
    case Types::Scalar::String:
      body = *default_;
      break;
    case Types::Scalar::Boolean:
      body = *default_ == "true";
      break;
    case Types::Scalar::Empty:
      body = {nullptr};
    }
  }
}
}  // namespace polycube::polycubed::Rest::Resources::Body
