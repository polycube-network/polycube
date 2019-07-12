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

#include "rest_server.h"

#include "../../Server/ResponseGenerator.h"
#include "../Body/JsonNodeField.h"
#include "../Body/JsonValueField.h"
#include "../Body/LeafResource.h"
#include "../Body/ListKey.h"
#include "Service.h"

namespace polycube::polycubed::Rest::Resources::Endpoint {
LeafListResource::LeafListResource(
    const std::string &name, const std::string &description,
    const std::string &cli_example, const std::string &rest_endpoint,
    const Body::ParentResource *parent, PolycubedCore *core,
    std::unique_ptr<Body::JsonValueField> &&value_field,
    const std::vector<Body::JsonNodeField> &node_fields, bool configuration,
    bool init_only_config, bool mandatory, Types::Scalar type,
    std::vector<std::string> &&default_value)
    : Body::LeafResource(name, description, cli_example, parent, core,
                         std::move(value_field), node_fields, configuration,
                         init_only_config, mandatory, type, nullptr),
      LeafResource(name, description, cli_example, rest_endpoint, parent, core,
                   nullptr, node_fields, configuration, init_only_config,
                   mandatory, type, nullptr, false, {}),
      Body::LeafListResource(name, description, cli_example, parent, core,
                             nullptr, node_fields, configuration,
                             init_only_config, mandatory, type,
                             std::move(default_value)) {}

LeafListResource::~LeafListResource() {}

}  // namespace polycube::polycubed::Rest::Resources::Endpoint
