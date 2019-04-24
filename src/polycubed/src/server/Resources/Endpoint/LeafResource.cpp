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

#include <algorithm>
#include <memory>
#include <string>
#include <utility>

#include "../Body/JsonNodeField.h"
#include "../Body/ListKey.h"

#include "../../Server/ResponseGenerator.h"
#include "Service.h"
#include "rest_server.h"

namespace polycube::polycubed::Rest::Resources::Endpoint {
LeafResource::LeafResource(
    const std::string &name, const std::string &description,
    const std::string &cli_example, const std::string &rest_endpoint,
    const Body::ParentResource *parent, PolycubedCore *core,
    std::unique_ptr<Body::JsonValueField> &&value_field,
    const std::vector<Body::JsonNodeField> &node_fields, bool configuration,
    bool init_only_config, bool mandatory, Types::Scalar type,
    std::unique_ptr<const std::string> &&default_value,
    bool is_enum, const std::vector<std::string> &values)
    : Body::LeafResource(name, description, cli_example, parent, core,
                         std::move(value_field), node_fields, configuration,
                         init_only_config, mandatory, type,
                         std::move(default_value)),
      Endpoint::Resource(rest_endpoint),
      is_enum_(is_enum), values_(values) {
  using Pistache::Rest::Routes::bind;
  auto router = core_->get_rest_server()->get_router();
  router->get(rest_endpoint_, bind(&LeafResource::get, this));
  router->options(rest_endpoint_, bind(&LeafResource::options, this));
  if (configuration_ && !init_only_config_) {
    router->patch(rest_endpoint_, bind(&LeafResource::patch, this));
  }
}

LeafResource::~LeafResource() {
  using Pistache::Http::Method;
  auto router = core_->get_rest_server()->get_router();
  router->removeRoute(Method::Get, rest_endpoint_);
  router->removeRoute(Method::Options, rest_endpoint_);
  if (configuration_ && !init_only_config_) {
    router->removeRoute(Method::Patch, rest_endpoint_);
  }
}

void LeafResource::Keys(const Pistache::Rest::Request &request,
                        ListKeyValues &parsed) const {
  return dynamic_cast<const ParentResource *const>(parent_)->Keys(request,
                                                                  parsed);
}

void LeafResource::get(const Request &request, ResponseWriter response) {
  std::vector<Response> errors;
  auto rerrors =
      dynamic_cast<const ParentResource *const>(parent_)->RequestValidate(
          request, name_);
  errors.reserve(rerrors.size());
  std::copy(std::begin(rerrors), std::end(rerrors), std::back_inserter(errors));
  if (errors.empty()) {
    const auto &cube_name = Service::Cube(request);
    ListKeyValues keys{};
    dynamic_cast<const ParentResource *const>(parent_)->Keys(request, keys);
    errors.push_back(ReadValue(cube_name, keys));
  }
  Server::ResponseGenerator::Generate(std::move(errors), std::move(response));
}

void LeafResource::CreateReplaceUpdate(const Pistache::Rest::Request &request,
                                       Pistache::Http::ResponseWriter response,
                                       bool update, bool initialization) {
  auto errors = RequestValidate(request, name_);
  nlohmann::json jbody;
  if (request.body().empty()) {
    jbody = nlohmann::json::parse("{}");
  } else {
    jbody = nlohmann::json::parse(request.body());
  }

  // This method can be reached only as direct call on the update method
  // of this LeafResource. It is possible only for configuration
  // nodes not marked as init-only-config.
  SetDefaultIfMissing(jbody);

  const auto cube_name = Service::Cube(request);
  ListKeyValues keys{};
  dynamic_cast<const ParentResource *const>(parent_)->Keys(request, keys);
  auto body = BodyValidate(cube_name, keys, jbody, false);
  errors.reserve(errors.size() + body.size());
  std::copy(std::begin(body), std::end(body), std::back_inserter(errors));

  if (errors.empty()) {
    // Update (PATCH) and replace (PUT) would have the same effect on a
    // scalar type. For easier CLI implementation, PATCH was used.
    auto op = Operation::kUpdate;
    auto resp = WriteValue(cube_name, jbody, keys, op);
    errors.push_back(resp);
  }
  Server::ResponseGenerator::Generate(std::move(errors), std::move(response));
}

std::vector<Response> LeafResource::RequestValidate(
    const Pistache::Rest::Request &request,
    [[maybe_unused]] const std::string &caller_name) const {
  std::vector<Response> errors;
  if (parent_ != nullptr) {
    auto rerrors =
        dynamic_cast<const ParentResource *const>(parent_)->RequestValidate(
            request, name_);
    errors.reserve(rerrors.size());
    std::move(std::begin(rerrors), std::end(rerrors),
              std::back_inserter(errors));
  }
  return errors;
}

void LeafResource::patch(const Request &request, ResponseWriter response) {
  CreateReplaceUpdate(request, std::move(response), true, false);
}

void LeafResource::options(const Request &request, ResponseWriter response) {
  const auto &query_param = request.query();
  if (!query_param.has("help")) {
    Server::ResponseGenerator::Generate({{kBadRequest, nullptr}},
                                        std::move(response));
    return;
  }

  ListKeyValues keys{};
  Keys(request, keys);

  if (query_param.has("completion")) {
    Server::ResponseGenerator::Generate(
        {Completion(Service::Cube(request), keys)}, std::move(response));
    return;
  }

  Server::ResponseGenerator::Generate({{kBadRequest, nullptr}},
                                      std::move(response));
}

Response LeafResource::Completion(const std::string &cube_name,
                                  const ListKeyValues &keys) {
  nlohmann::json val = values_;
  return {kOk, ::strdup(val.dump().c_str())};
}

}  // namespace polycube::polycubed::Rest::Resources::Endpoint
