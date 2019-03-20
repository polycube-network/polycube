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
                   mandatory, type, nullptr),
      Body::LeafListResource(name, description, cli_example, parent, core,
                             nullptr, node_fields, configuration,
                             init_only_config, mandatory, type,
                             std::move(default_value)) {
  using Pistache::Rest::Routes::bind;
  auto router = core_->get_rest_server()->get_router();
  router->get(rest_endpoint_ + "/:entry",
              bind(&LeafListResource::get_entry, this));
}

LeafListResource::~LeafListResource() {
  using Pistache::Http::Method;
  auto router = core_->get_rest_server()->get_router();
  router->removeRoute(Method::Get, rest_endpoint_ + "/:entry");
}

void LeafListResource::get_entry(const Request &request,
                                 ResponseWriter response) {
  std::vector<Response> errors;
  if (parent_ != nullptr) {
    auto rerrors =
        dynamic_cast<const ParentResource *const>(parent_)->RequestValidate(
            request, name_);
    errors.reserve(rerrors.size());
    std::move(std::begin(rerrors), std::end(rerrors),
              std::back_inserter(errors));
  }
  auto value = nlohmann::json::parse(request.param(":entry").as<std::string>());

  const auto cube_name = Service::Cube(request);
  ListKeyValues keys{};
  dynamic_cast<const ParentResource *const>(parent_)->Keys(request, keys);
  auto body = LeafResource::BodyValidate(cube_name, keys, value, false);
  errors.reserve(errors.size() + body.size());
  std::move(std::begin(body), std::end(body), std::back_inserter(errors));
  if (errors.empty()) {
    errors.push_back({kOk, nullptr});
  }
  Server::ResponseGenerator::Generate(std::move(errors), std::move(response));
}
}  // namespace polycube::polycubed::Rest::Resources::Endpoint
