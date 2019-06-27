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
#include "ListResource.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "../../Server/ResponseGenerator.h"
#include "../Body/JsonNodeField.h"
#include "../Body/ListKey.h"
#include "PathParamField.h"
#include "Service.h"
#include "rest_server.h"

#include "../Data/Lib/ListResource.h"

namespace polycube::polycubed::Rest::Resources::Endpoint {
ListResource::ListResource(const std::string &name,
                           const std::string &description,
                           const std::string &cli_example,
                           const Body::ParentResource *parent,
                           PolycubedCore *core,
                           const std::string &rest_endpoint,
                           const std::string &rest_endpoint_multiple,
                           std::vector<Body::ListKey> &&keys,
                           const std::vector<Body::JsonNodeField> &node_fields,
                           bool configuration, bool init_only_config)
    : Body::ParentResource(name, description, cli_example, parent, core,
                           node_fields, configuration, init_only_config, false),
      ParentResource(name, description, cli_example, rest_endpoint, parent,
                     core, node_fields, configuration, init_only_config, false,
                     false),
      Body::ListResource(name, description, cli_example, parent, core,
                         std::move(keys), node_fields, configuration,
                         init_only_config),
      key_params_{},
      multiple_endpoint_(rest_endpoint_multiple) {
  using Pistache::Rest::Routes::bind;
  for (const auto &key : keys_) {
    key_params_.emplace_back(key.Name(), key.Validators());
  }
  auto router = core_->get_rest_server()->get_router();
  router->get(multiple_endpoint_, bind(&ListResource::get_multiple, this));

  // endpoints for help
  router->options(multiple_endpoint_,
                  bind(&ListResource::options_multiple, this));

  std::string endpoint(multiple_endpoint_ + "/");
  for (int i = 0; i < key_params_.size() - 1; i++) {
    endpoint += key_params_.at(i).Name() + "/";
    router->options(endpoint,
                    bind(&ListResource::options_multiple, this));
  }

  if (configuration_ && !init_only_config) {
    router->post(multiple_endpoint_, bind(&ListResource::post_multiple, this));
    router->put(multiple_endpoint_, bind(&ListResource::put_multiple, this));
    router->patch(multiple_endpoint_,
                  bind(&ListResource::patch_multiple, this));
    router->del(multiple_endpoint_, bind(&ListResource::del_multiple, this));
  }
}

ListResource::~ListResource() {
  using Pistache::Http::Method;
  auto router = core_->get_rest_server()->get_router();
  router->removeRoute(Method::Get, multiple_endpoint_);
  router->removeRoute(Method::Options, multiple_endpoint_);

  std::string endpoint(multiple_endpoint_ + "/");
  for (int i = 0; i < key_params_.size() - 1; i++) {
    endpoint += key_params_.at(i).Name() + "/";
    router->removeRoute(Method::Options, endpoint);
  }

  if (configuration_ && !init_only_config_) {
    router->removeRoute(Method::Post, multiple_endpoint_);
    router->removeRoute(Method::Put, multiple_endpoint_);
    router->removeRoute(Method::Patch, multiple_endpoint_);
    router->removeRoute(Method::Delete, multiple_endpoint_);
  }
}

std::vector<Response> ListResource::RequestValidate(
    const Pistache::Rest::Request &request,
    const std::string &caller_name) const {
  auto errors = ParentResource::RequestValidate(request, caller_name);
  for (const auto &key_param : key_params_) {
    auto error = key_param.Validate(request);
    if (error != ErrorTag::kOk) {
      errors.push_back({error, ::strdup(key_param.Name().data())});
    }
  }
  return errors;
}

void ListResource::Keys(const Pistache::Rest::Request &request,
                        ListKeyValues &parsed) const {
  for (const auto &k : keys_) {
    parsed.push_back(
        {k.Name(), k.Type(), request.param(':' + k.Name()).as<std::string>()});
  }
  dynamic_cast<const ParentResource *const>(parent_)->Keys(request, parsed);
}

void ListResource::GetListKeys(const Pistache::Rest::Request &request,
                               ListKeyValues &parsed) const {
  for (const auto &k : keys_) {
    if (request.hasParam(':' + k.Name())) {
      parsed.push_back(
          {k.Name(), k.Type(), request.param(':' + k.Name()).as<std::string>()});
    }
  }
}

void ListResource::CreateReplaceUpdateWhole(
    const Pistache::Rest::Request &request, ResponseWriter response,
    bool update, bool initialization) {
  std::vector<Response> errors;
  if (parent_ != nullptr) {
    auto rerrors =
        dynamic_cast<const ParentResource *const>(parent_)->RequestValidate(
            request, name_);
    errors.reserve(rerrors.size());
    std::move(std::begin(rerrors), std::end(rerrors),
              std::back_inserter(errors));
  }

  nlohmann::json jbody;
  if (request.body().empty()) {
    jbody = nlohmann::json::parse("[]");
  } else {
    jbody = nlohmann::json::parse(request.body());
  }

  if (jbody.type() != nlohmann::detail::value_t::array) {
    Server::ResponseGenerator::Generate(
        std::vector<Response>{{ErrorTag::kInvalidValue, nullptr}},
        std::move(response));
    return;
  }

  const auto cube_name = Service::Cube(request);
  ListKeyValues keys{};
  dynamic_cast<const ParentResource *const>(parent_)->Keys(request, keys);
  for (auto &elem : jbody) {
    if (initialization) {
      SetDefaultIfMissing(elem);
    }
    auto body = BodyValidate(cube_name, keys, elem, initialization);
    errors.reserve(errors.size() + body.size());
    std::copy(std::begin(body), std::end(body), std::back_inserter(errors));
  }
  if (errors.empty()) {
    auto op = OperationType(update, initialization);
    auto resp = WriteWhole(cube_name, jbody, keys, op);
    if (resp.error_tag == ErrorTag::kOk) {
      errors.push_back({ErrorTag::kCreated, nullptr});
    } else {
      errors.push_back(resp);
    }
  }
  Server::ResponseGenerator::Generate(std::move(errors), std::move(response));
}

void ListResource::get_multiple(const Request &request,
                                ResponseWriter response) {
  std::vector<Response> errors;
  if (parent_ != nullptr) {
    auto rerrors =
        dynamic_cast<const ParentResource *const>(parent_)->RequestValidate(
            request, name_);
    errors.reserve(rerrors.size());
    std::copy(std::begin(rerrors), std::end(rerrors),
              std::back_inserter(errors));
  }
  if (errors.empty()) {
    const auto &cube_name = Service::Cube(request);
    ListKeyValues keys{};
    dynamic_cast<const ParentResource *const>(parent_)->Keys(request, keys);
    errors.push_back(ReadWhole(cube_name, keys));
  }
  Server::ResponseGenerator::Generate(std::move(errors), std::move(response));
}

void ListResource::post_multiple(const Request &request,
                                 ResponseWriter response) {
  CreateReplaceUpdateWhole(request, std::move(response), false, true);
}

void ListResource::put_multiple(const Request &request,
                                ResponseWriter response) {
  CreateReplaceUpdateWhole(request, std::move(response), true, true);
}

void ListResource::patch_multiple(const Request &request,
                                  ResponseWriter response) {
  CreateReplaceUpdateWhole(request, std::move(response), true, false);
}

void ListResource::del_multiple(const Request &request,
                                ResponseWriter response) {
  std::vector<Response> errors;
  if (parent_ != nullptr) {
    auto rerrors =
        dynamic_cast<const ParentResource *const>(parent_)->RequestValidate(
            request, name_);
    errors.reserve(rerrors.size());
    std::copy(std::begin(rerrors), std::end(rerrors),
              std::back_inserter(errors));
  }
  if (errors.empty()) {
    const auto &cube_name = Service::Cube(request);
    ListKeyValues keys{};
    dynamic_cast<const ParentResource *const>(parent_)->Keys(request, keys);
    errors.push_back(DeleteWhole(cube_name, keys));
  }
  Server::ResponseGenerator::Generate(std::move(errors), std::move(response));
}

void ListResource::options_multiple(const Request &request,
                                    ResponseWriter response) {
  const auto &query_param = request.query();
  if (!query_param.has("help")) {
    Server::ResponseGenerator::Generate({{kBadRequest, nullptr}},
                                        std::move(response));
    return;
  }

  auto help = query_param.get("help").get();
  if (help == "NO_HELP") {
    Server::ResponseGenerator::Generate({{kOk, nullptr}}, std::move(response));
  }

  HelpType type;
  if (help == "SHOW") {
    type = SHOW;
  } else if (help == "ADD") {
    type = ADD;
  } else if (help == "DEL") {
    type = DEL;
  } else if (help == "NONE") {
    type = NONE;
  } else {
    Server::ResponseGenerator::Generate({{kBadRequest, nullptr}},
                                        std::move(response));
    return;
  }
  ListKeyValues keys{};

  dynamic_cast<const ParentResource *const>(parent_)->Keys(request, keys);

  if (!query_param.has("completion")) {
    auto resp = Help(Service::Cube(request), type, keys);
    Server::ResponseGenerator::Generate({resp}, std::move(response));
  } else {
    ListKeyValues list_keys{};
    GetListKeys(request, list_keys);
    auto resp = Completion(Service::Cube(request), type, keys, list_keys);
    Server::ResponseGenerator::Generate({resp}, std::move(response));
  }
}

Response ListResource::Help(const std::string &cube_name, HelpType type,
                            const ListKeyValues &keys) {
  nlohmann::json val = nlohmann::json::object();

  switch (type) {
  case HelpType::SHOW:
    val["params"] = helpKeys();
    val["elements"] =
        nlohmann::json::parse(GetElementsList(cube_name, keys).message);
    break;
  case HelpType::ADD:
    val["params"] = helpKeys();
    val["optional-params"] = helpWritableLeafs(true);
    break;
  case HelpType::DEL:
    val["params"] = helpKeys();
    val["elements"] =
        nlohmann::json::parse(GetElementsList(cube_name, keys).message);
    break;
  case HelpType::NONE:
    val["commands"] = {"add", "del", "show"};
    val["params"] = helpKeys();
    val["elements"] =
        nlohmann::json::parse(GetElementsList(cube_name, keys).message);
    break;
  default:
    return {kBadRequest, nullptr};
  }

  return {kOk, ::strdup(val.dump().c_str())};
}

nlohmann::json ListResource::helpKeys() const {
  nlohmann::json val = nlohmann::json::object();

  for (auto &i : children_) {
    if (i->IsKey()) {
      val[i->Name()] = i->ToHelpJson();
    }
  }

  return val;
}

Response ListResource::Completion(const std::string &cube_name, HelpType type,
                                  const ListKeyValues &keys,
                                  const ListKeyValues &list_keys) {
  nlohmann::json elements_json =
      nlohmann::json::parse(GetElementsList(cube_name, keys).message);

  auto size_keys = list_keys.size();
  std::string keyname = keys_[size_keys].Name();
  std::string original_keyname = keys_[size_keys].OriginalName();

  nlohmann::json val = nlohmann::json::array();

  if (type == HelpType::ADD) {
    val = {"<" + keyname + ">"};
    return {kOk, ::strdup(val.dump().c_str())};
  }

  // This is a nice library but a silly approach to convert it back.
  std::vector<nlohmann::fifo_map<std::string, std::string>> elements = elements_json;

  for (auto &item : elements) {
    // check if element matches giving key
    bool found = true;
    for (auto &key : list_keys) {
      if (item.at(key.name) != key.value) {
        found = false;
        break;
      }
    }

    if (found) {
      val += item.at(original_keyname);
    }
  }

  switch (type) {
  case HelpType::SHOW:
    break;
  case HelpType::DEL:
    break;
  case HelpType::NONE:
    // suggest these commands only if we are not in the middle of a list
    if (list_keys.size() == 0) {
      if (configuration_) {
        val += "add";
        val += "del";
      }
      val += "show";
    }
    break;
  default:
    return {kBadRequest, nullptr};
  }

  return {kOk, ::strdup(val.dump().c_str())};
}

}  // namespace polycube::polycubed::Rest::Resources::Endpoint
