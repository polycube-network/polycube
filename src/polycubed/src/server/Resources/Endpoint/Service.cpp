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
#include "Service.h"

#include "polycube/services/cube_factory.h"
#include "polycubed_core.h"
#include "rest_server.h"
#include "../../config.h"
#include "cubes_dump.h"

#include "../../Server/ResponseGenerator.h"
#include "../Body/JsonNodeField.h"
#include "../Body/ListKey.h"

#include <memory>

namespace polycube::polycubed::Rest::Resources::Endpoint {

Service::Service(const std::string &name, const std::string &description,
                 const std::string &cli_example, std::string base_address,
                 const std::string &version, PolycubedCore *core)
    : Body::ParentResource(name, description, cli_example, nullptr, core,
                           std::vector<Body::JsonNodeField>{}, true, false,
                           false),
      ParentResource(name, description, cli_example,
                     base_address + name + "/:name", nullptr, core,
                     std::vector<Body::JsonNodeField>{}, true, false, false,
                     false),
      Body::Service(name, description, cli_example, version, nullptr),
      body_rest_endpoint_(base_address + name + '/'),
      cube_names_{} {
  using Pistache::Rest::Routes::bind;
  auto router = core_->get_rest_server()->get_router();

  router->get(body_rest_endpoint_, bind(&Service::get_body, this));
  router->post(body_rest_endpoint_, bind(&Service::post_body, this));
  router->patch(body_rest_endpoint_, bind(&Service::patch_body, this));
  router->options(body_rest_endpoint_, bind(&Service::options_body, this));
}

Service::~Service() {
  using Pistache::Http::Method;
  auto router = core_->get_rest_server()->get_router();
  router->removeRoute(Method::Get, body_rest_endpoint_);
  router->removeRoute(Method::Post, body_rest_endpoint_);
  router->removeRoute(Method::Patch, body_rest_endpoint_);
  router->removeRoute(Method::Options, body_rest_endpoint_);
}

const std::string Service::Cube(const Pistache::Rest::Request &request) {
  return request.param(":name").as<std::string>();
}

void Service::ClearCubes() {
  auto k = ListKeyValues{};
  for (const auto &cube_name : cube_names_.Values()) {
    DeleteValue(cube_name, k);
  }
}

std::vector<Response>
Service::CreateReplaceUpdate(const std::string &name, nlohmann::json &body,
                             bool update, bool initialization) {
  if (update || !ServiceController::exists_cube(name)) {
    auto op = OperationType(update, initialization);
    auto k = ListKeyValues{};
    if (initialization) {
      SetDefaultIfMissing(body);
    }

    auto jbody = body;
    jbody.erase("name");

    auto body_errors = BodyValidate(name, k, jbody, initialization);
    if (!body_errors.empty()) {
      return std::move(body_errors);
    }

   /*
    * It is possible that a port inside a cube includes a list of
    * transparent cubes to be attached.  Those transparent cubes when are
    * attached can request the port for some configuration, at this point
    * the cube is not ready at that fails.
    * This logic removes the transparent cubes list from the ports and
    * only assigns them when the cube is fully created.
    */
   auto cubes_list = utils::strip_port_tcubes(body);

    auto resp = WriteValue(name, body, k, op);
    // check if the operation completed successfully and in case update the configuration
    if (isOperationSuccessful(resp.error_tag)) {
      if (!update) {
        cube_names_.AddValue(name);
      }
      if (configuration::config.getCubesDumpEnabled()) {
        core_->get_cubes_dump()->UpdateCubesConfig(RestServer::base + this->name_ + "/" + name + "/",
                body, k, op, ResourceType::Service);
      }
    }

    // use  previously calculated list to update ports' transparent cubes
    auto cube_ = ServiceController::get_cube(name);
    auto cube = std::dynamic_pointer_cast<polycube::polycubed::Cube>(cube_);
    for (auto &[k, v] : cubes_list) {
      auto port = cube->get_port(k);
      port->set_conf(cubes_list[k]);
    }
    return std::vector<Response>{resp};
  } else {
    return std::vector<Response>{{ErrorTag::kDataExists, nullptr}};
  }
}

void Service::get_body(const Request &request, ResponseWriter response) {
  if (!request.body().empty()) {
    Server::ResponseGenerator::Generate(
        std::vector<Response>{{ErrorTag::kOperationNotSupported, nullptr}},
        std::move(response));
    return;
  }
  // TODO call get-list
  Server::ResponseGenerator::Generate(
      std::vector<Response>{{ErrorTag::kOk, nullptr}}, std::move(response));
}

void Service::post_body(const Request &request, ResponseWriter response) {
  nlohmann::json body;
  if (request.body().empty()) {
    body = nlohmann::json::parse("{}");
  } else {
    body = nlohmann::json::parse(request.body());
  }

  if (body.count("name") == 0) {
    Server::ResponseGenerator::Generate(
        std::vector<Response>{{ErrorTag::kMissingAttribute, ::strdup("name")}},
        std::move(response));
    return;
  }

  auto resp = CreateReplaceUpdate(body["name"].get<std::string>(), body,
          false, true);
  Server:: ResponseGenerator::Generate(std::move(resp), std::move(response));
}

void Service::post(const Request &request, ResponseWriter response) {
  auto name = request.param(":name").as<std::string>();
  nlohmann::json body;
  if (request.body().empty()) {
    body = nlohmann::json::parse("{}");
  } else {
    body = nlohmann::json::parse(request.body());
  }
  body["name"] = name;

  if (body.count("service-name")) {
    if (body["service-name"] != Name()) {
      Server::ResponseGenerator::Generate({{kInvalidValue, nullptr}},
                                          std::move(response));
      return;
    }
  }

  auto resp = CreateReplaceUpdate(name, body, false, true);
  Server:: ResponseGenerator::Generate(std::move(resp), std::move(response));
}

void Service::put(const Request &request, ResponseWriter response) {
  auto name = request.param(":name").as<std::string>();
  nlohmann::json body;
  if (request.body().empty()) {
    body = nlohmann::json::parse("{}");
  } else {
    body = nlohmann::json::parse(request.body());
  }
  body["name"] = name;
  auto resp = CreateReplaceUpdate(name, body, false, true);
  Server:: ResponseGenerator::Generate(std::move(resp), std::move(response));
}

void Service::patch(const Request &request, ResponseWriter response) {
  auto name = request.param(":name").as<std::string>();
  nlohmann::json body;
  if (request.body().empty()) {
    body = nlohmann::json::parse("{}");
  } else {
    body = nlohmann::json::parse(request.body());
  }
  body["name"] = name;
  auto resp = CreateReplaceUpdate(name, body, true, false);
  Server:: ResponseGenerator::Generate(std::move(resp), std::move(response));
}

void Service::del(const Pistache::Rest::Request &request,
                  Pistache::Http::ResponseWriter response) {
  auto name = request.param(":name").as<std::string>();
  if (!ServiceController::exists_cube(name)) {
      Server::ResponseGenerator::Generate({{kDataMissing, nullptr}},
                                          std::move(response));
      return;
  }

  cube_names_.RemoveValue(name);
  auto k = ListKeyValues{};
  auto res = DeleteValue(name, k);
  Server::ResponseGenerator::Generate(std::vector<Response>{res},
                                      std::move(response));
  if (configuration::config.getCubesDumpEnabled()) {
    core_->get_cubes_dump()->UpdateCubesConfig(
            RestServer::base + this->name_ + "/" + name + "/",nullptr, k,
            Operation::kDelete, ResourceType::Service);
  }
}

void Service::patch_body(const Request &request, ResponseWriter response) {
  nlohmann::json body = nlohmann::json::parse(request.body());

  if (body.count("name") == 0) {
    Server::ResponseGenerator::Generate(
        std::vector<Response>{{ErrorTag::kMissingAttribute, ::strdup("name")}},
        std::move(response));
    return;
  }
  auto resp = CreateReplaceUpdate(body["name"].get<std::string>(), body,
          true, false);
  Server:: ResponseGenerator::Generate(std::move(resp), std::move(response));
}

void Service::options_body(const Request &request, ResponseWriter response) {
  const auto &query_param = request.query();
  if (!query_param.has("help")) {
    Server::ResponseGenerator::Generate({{kBadRequest, nullptr}},
                                        std::move(response));
    return;
  }

  auto help = query_param.get("help").get();
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
  if (!query_param.has("completion")) {
    Server::ResponseGenerator::Generate({Help(type)}, std::move(response));
  } else {
    Server::ResponseGenerator::Generate({CompletionService(type)},
            std::move(response));
  }
  //Server::ResponseGenerator::Generate({Help(type)}, std::move(response));
}

Response Service::Help(HelpType type) {
  nlohmann::json val = nlohmann::json::object();

  switch (type) {
  case HelpType::SHOW:
    val["params"] = getServiceKeys();
    val["elements"] = nlohmann::json::parse(ReadHelp().message);
    break;
  case HelpType::ADD:
    val["params"] = getServiceKeys();
    val["optional-params"] = helpWritableLeafs(true);
    break;
  case HelpType::DEL:
    val["params"] = getServiceKeys();
    val["elements"] = nlohmann::json::parse(ReadHelp().message);
    break;
  case HelpType::NONE:
    val["commands"] = {"add", "del", "show"};
    val["params"] = getServiceKeys();
    val["elements"] = nlohmann::json::parse(ReadHelp().message);
    break;
  default:
    return {kBadRequest, nullptr};
  }

  return {kOk, ::strdup(val.dump().c_str())};
}

nlohmann::json Service::getServiceKeys() const {
  nlohmann::json val = nlohmann::json::object();
  auto description = "Name of the " + Name() + " service";
  auto example = Name() + "1";

  val["name"]["name"] = "name";
  val["name"]["type"] = "key";
  val["name"]["simpletype"] = "string";
  val["name"]["description"] = description;
  val["name"]["example"] = example;

  return val;
}

Response Service::CompletionService(HelpType type) {
  nlohmann::json elements_json = nlohmann::json::parse(ReadHelp().message);

  std::vector<nlohmann::fifo_map<std::string, std::string>> elements = elements_json;

  nlohmann::json val = nlohmann::json::array();

  for (auto &item : elements) {
    for (auto &key: item) {
      val += key.second;
    }
  }

  switch (type) {
  case HelpType::SHOW:
    break;
  case HelpType::ADD:
    val += "<name>"; //nlohmann::json::array();
    break;
  case HelpType::DEL:
    break;
  case HelpType::NONE:
    val += "add";
    val += "del";
    val += "show";
    break;
  default:
    return {kBadRequest, nullptr};
  }

  return {kOk, ::strdup(val.dump().c_str())};
}


}  // namespace polycube::polycubed::Rest::Resources::Endpoint
