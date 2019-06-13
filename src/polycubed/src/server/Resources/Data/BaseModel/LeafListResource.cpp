/*
 * Copyright 2019 The Polycube Authors
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

#include <functional>
#include <memory>
#include <string>
#include <utility>

#include "../../Body/JsonNodeField.h"

namespace polycube::polycubed::Rest::Resources::Data::BaseModel {
LeafListResource::LeafListResource(
    std::function<Response(const std::string &, const ListKeyValues &keys)>
        read_handler,
    std::function<Response(const std::string &, const nlohmann::json &,
                           const ListKeyValues &, Endpoint::Operation)>
        replace_handler,
    const std::string &name, const std::string &description,
    const std::string &cli_example, const std::string &rest_endpoint,
    const Body::ParentResource *parent, bool configuration,
    bool init_only_config, PolycubedCore *core,
    std::unique_ptr<Body::JsonValueField> &&value_field,
    const std::vector<Body::JsonNodeField> &node_fields, bool mandatory,
    Types::Scalar type, std::vector<std::string> &&default_value)
    : Body::LeafListResource(name, description, cli_example, parent, core,
                         std::move(value_field), node_fields, configuration,
                         init_only_config, mandatory, type, std::move(default_value)),
      Body::LeafResource(name, description, cli_example, parent, core,
                         std::move(value_field), node_fields, configuration,
                         init_only_config, mandatory, type, nullptr),
      Endpoint::LeafListResource(name, description, cli_example, rest_endpoint,
                             parent, core, nullptr, node_fields, configuration,
                             init_only_config, mandatory, type, std::move(default_value)),
      read_handler_{std::move(read_handler)},
      replace_handler_{std::move(replace_handler)} {}

const Response LeafListResource::ReadValue(const std::string &cube_name,
                                       const ListKeyValues &keys) const {
  return read_handler_(cube_name, keys);
}

Response LeafListResource::WriteValue(const std::string &cube_name,
                                  const nlohmann::json &value,
                                  const ListKeyValues &keys,
                                  Endpoint::Operation operation) {
  return replace_handler_(cube_name, value, keys, operation);
}
}  // namespace polycube::polycubed::Rest::Resources::Data::BaseModel
