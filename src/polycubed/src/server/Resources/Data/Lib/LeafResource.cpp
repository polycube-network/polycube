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

#include <functional>
#include <memory>
#include <string>
#include <utility>

#include "../../Body/JsonNodeField.h"
#include "KeyListArray.h"

namespace polycube::polycubed::Rest::Resources::Data::Lib {
LeafResource::LeafResource(
    std::function<Response(const char *, const Key *, size_t)> read_handler,
    const std::string &name, const std::string &description,
    const std::string &cli_example, const std::string &rest_endpoint,
    const Body::ParentResource *parent, bool configuration,
    bool init_only_config, PolycubedCore *core,
    std::unique_ptr<Body::JsonValueField> &&value_field,
    const std::vector<Body::JsonNodeField> &node_fields, bool mandatory,
    Types::Scalar type, std::unique_ptr<const std::string> &&default_value)
    : Body::LeafResource(name, description, cli_example, parent, core,
                         std::move(value_field), node_fields, configuration,
                         init_only_config, mandatory, type,
                         std::move(default_value)),
      Endpoint::LeafResource(name, description, cli_example, rest_endpoint,
                             parent, core, nullptr, node_fields, configuration,
                             init_only_config, mandatory, type, nullptr),
      read_handler_{std::move(read_handler)},
      replace_handler_{} {}

LeafResource::LeafResource(
    std::function<Response(const char *, const Key *, size_t, const char *)>
        replace_handler,
    std::function<Response(const char *, const Key *, size_t)> read_handler,
    const std::string &name, const std::string &description,
    const std::string &cli_example, const std::string &rest_endpoint,
    const Body::ParentResource *parent, PolycubedCore *core,
    std::unique_ptr<Body::JsonValueField> &&value_field,
    const std::vector<Body::JsonNodeField> &node_fields, bool mandatory,
    Types::Scalar type, std::unique_ptr<const std::string> &&default_value)
    : Body::LeafResource(name, description, cli_example, parent, core,
                         std::move(value_field), node_fields, true, false,
                         mandatory, type, std::move(default_value)),
      Endpoint::LeafResource(name, description, cli_example, rest_endpoint,
                             parent, core, nullptr, node_fields, true, false,
                             mandatory, type, nullptr),
      read_handler_{std::move(read_handler)},
      replace_handler_{std::move(replace_handler)} {}

const Response LeafResource::ReadValue(const std::string &cube_name,
                                       const ListKeyValues &keys) const {
  const auto &key_params = KeyListArray::Generate(keys);
  return read_handler_(cube_name.data(), key_params.data(), key_params.size());
}

Response LeafResource::WriteValue(const std::string &cube_name,
                                  const nlohmann::json &value,
                                  const ListKeyValues &keys,
                                  Endpoint::Operation operation) {
  const auto &key_params = KeyListArray::Generate(keys);
  // No check on operation since only allowed write operation for leafs is
  // replace
  return replace_handler_(cube_name.data(), key_params.data(),
                          key_params.size(), value.dump().data());
}
}  // namespace polycube::polycubed::Rest::Resources::Data::Lib
