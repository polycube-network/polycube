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

#include "base_model.h"

#include "service_controller.h"

namespace polycube::polycubed {

Response BaseModel::get_type(const std::string &cube_name) const {
  auto cube = ServiceController::get_cube(cube_name);
  if (cube == nullptr) {
    return Response{kNoContent, ::strdup("Cube does not exist")};
  }

  std::string type_str = "\"" + cube_type_to_string(cube->get_type()) + "\"";

  return Response{kOk, ::strdup(type_str.data())};
}

Response BaseModel::get_uuid(const std::string &cube_name) const {
  auto cube = ServiceController::get_cube(cube_name);
  if (cube == nullptr) {
    return Response{kNoContent, ::strdup("Cube does not exist")};
  }

  auto uuid = "\"" + cube->uuid().str() + "\"";

  return Response{kOk, ::strdup(uuid.data())};
}

Response BaseModel::get_loglevel(const std::string &cube_name) const {
  auto cube = ServiceController::get_cube(cube_name);
  if (cube == nullptr) {
    return Response{kNoContent, ::strdup("Cube does not exist")};
  }

  auto loglevel = "\"" + logLevelString(cube->get_log_level()) + "\"";
  return Response{kOk, ::strdup(loglevel.data())};
}

Response BaseModel::set_loglevel(const std::string &cube_name,
                                 const nlohmann::json &json) {
  auto cube = ServiceController::get_cube(cube_name);
  if (cube == nullptr) {
    return Response{kNoContent, ::strdup("Cube does not exist")};
  }

  auto loglevel_str = json.get<std::string>();
  std::transform(loglevel_str.begin(), loglevel_str.end(), loglevel_str.begin(),
                 ::toupper);
  auto loglevel = stringLogLevel(loglevel_str);
  cube->set_log_level(loglevel);

  return Response{kOk, ::strdup("")};
}

Response BaseModel::get_parent(const std::string &cube_name) const {
  auto cube_ = ServiceController::get_cube(cube_name);
  if (cube_ == nullptr) {
    return Response{kNoContent, ::strdup("Cube does not exist")};
  }

  // TODO: is this case even possible?
  auto cube = std::dynamic_pointer_cast<TransparentCube>(cube_);
  if (cube == nullptr) {
    return Response{kNoContent, ::strdup("Cube is not transparent")};
  }
  auto parent = dynamic_cast<Port *>(cube->get_parent());
  if (!parent) {
    return Response{kNoContent, ::strdup("Not connected to a port")};
  }

  auto parent_name = "\"" + parent->get_path() + "\"";

  return Response{kOk, ::strdup(parent_name.data())};
}

Response BaseModel::get_service(const std::string &cube_name) const {
  auto cube_ = ServiceController::get_cube(cube_name);
  if (cube_ == nullptr) {
    return Response{kNoContent, ::strdup("Cube does not exist")};
  }

  auto service_name = "\"" + cube_->get_service_name() + "\"";

  return Response{kOk, ::strdup(service_name.data())};
}

Response BaseModel::get_port_uuid(const std::string &cube_name,
                                  const std::string &port_name) const {
  auto cube_ = ServiceController::get_cube(cube_name);
  if (cube_ == nullptr) {
    return Response{kNoContent, ::strdup("Cube does not exist")};
  }

  // TODO: is this case even possible?
  auto cube = std::dynamic_pointer_cast<CubeIface>(cube_);
  if (cube == nullptr) {
    return Response{kNoContent, ::strdup("Cube is transparent")};
  }

  auto port = cube->get_port(port_name);
  auto uuid = "\"" + port->uuid().str() + "\"";

  return Response{kOk, ::strdup(uuid.data())};
}

Response BaseModel::get_port_status(const std::string &cube_name,
                                    const std::string &port_name) const {
  auto cube_ = ServiceController::get_cube(cube_name);
  if (cube_ == nullptr) {
    return Response{kNoContent, ::strdup("Cube does not exist")};
  }

  // TODO: is this case even possible?
  auto cube = std::dynamic_pointer_cast<CubeIface>(cube_);
  if (cube == nullptr) {
    return Response{kNoContent, ::strdup("Cube is transparent")};
  }
  // TODO: verify if there is such port?
  auto port = cube->get_port(port_name);
  std::string status = "\"" + port_status_to_string(port->get_status()) + "\"";

  return Response{kOk, ::strdup(status.data())};
}

Response BaseModel::get_port_peer(const std::string &cube_name,
                                  const std::string &port_name) const {
  auto cube_ = ServiceController::get_cube(cube_name);
  if (cube_ == nullptr) {
    return Response{kNoContent, ::strdup("Cube does not exist")};
  }

  // TODO: is this case even possible?
  auto cube = std::dynamic_pointer_cast<CubeIface>(cube_);
  if (cube == nullptr) {
    return Response{kNoContent, ::strdup("Cube is transparent")};
  }

  auto port = cube->get_port(port_name);
  auto peer = "\"" + port->peer() + "\"";

  return Response{kOk, ::strdup(peer.data())};
}

Response BaseModel::set_port_peer(const std::string &cube_name,
                                  const std::string &port_name,
                                  const nlohmann::json &json) {
  auto cube_ = ServiceController::get_cube(cube_name);
  if (cube_ == nullptr) {
    return Response{kNoContent, ::strdup("Cube does not exist")};
  }

  // TODO: is this case even possible?
  auto cube = std::dynamic_pointer_cast<CubeIface>(cube_);
  if (cube == nullptr) {
    return Response{kNoContent, ::strdup("Cube is transparent")};
  }

  auto port = cube->get_port(port_name);
  auto peer = json.get<std::string>();

  port->set_peer(peer);

  return Response{kOk, ::strdup("")};
}

Response BaseModel::get_port_tcubes(const std::string &cube_name,
                                    const std::string &port_name) const {
  auto cube_ = ServiceController::get_cube(cube_name);
  if (cube_ == nullptr) {
    return Response{kNoContent, ::strdup("Cube does not exist")};
  }

  // TODO: is this case even possible?
  auto cube = std::dynamic_pointer_cast<CubeIface>(cube_);
  if (cube == nullptr) {
    return Response{kNoContent, ::strdup("Cube is transparent")};
  }

  auto port_ = cube->get_port(port_name);
  auto port = std::dynamic_pointer_cast<Port>(port_);
  nlohmann::json j = port->get_cubes_names();

  return Response{kOk, ::strdup(j.dump().c_str())};
}

}  // namespace polycube::polycubed