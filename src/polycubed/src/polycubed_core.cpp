/*
 * Copyright 2017 The Polycube Authors
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

#include "polycubed_core.h"
#include "rest_server.h"

#include <regex>

namespace polycube {
namespace polycubed {

PolycubedCore::PolycubedCore() : logger(spdlog::get("polycubed")) {}

PolycubedCore::~PolycubedCore() {
  servicectrls_map_.clear();
  ServiceController::ports_to_ifaces.clear();
}

void PolycubedCore::set_polycubeendpoint(std::string &polycube) {
  polycubeendpoint_ = polycube;
}

std::string PolycubedCore::get_polycubeendpoint() {
  return polycubeendpoint_;
}

void PolycubedCore::add_servicectrl(const std::string &name,
                                    const ServiceControllerType type,
                                    const std::string &base_url,
                                    const std::string &path) {
  // logger->debug("PolycubedCore: post servicectrl {0}", name);
  if (servicectrls_map_.count(name) != 0) {
    throw std::runtime_error("Service Controller already exists");
  }

  std::unordered_map<std::string, ServiceController>::iterator iter;
  bool inserted;
  std::tie(iter, inserted) = servicectrls_map_.emplace(
      std::piecewise_construct, std::forward_as_tuple(name),
      std::forward_as_tuple(name, path, base_url, type));
  if (!inserted) {
    throw std::runtime_error("error creating service controller");
  }

  ServiceController &s = iter->second;
  try {
    s.connect(this);
    logger->info("service {0} loaded using {1}", s.get_name(),
                 s.get_servicecontroller());
  } catch (const std::exception &e) {
    // logger->error("cannot load service: {0}", e.what());
    servicectrls_map_.erase(name);
    throw;
  }
}

std::string PolycubedCore::get_servicectrl(const std::string &name) {
  logger->debug("PolycubedCore: get service {0}", name);
  auto iter = servicectrls_map_.find(name);
  if (iter == servicectrls_map_.end()) {
    // logger->warn("no service present with name {0}", name);
    throw std::runtime_error("Service Controller does not exist");
  }

  ServiceController &s = iter->second;
  json j = json::array();
  j += s.to_json_datamodel();
  return j.dump(4);
}

const ServiceController &PolycubedCore::get_service_controller(
    const std::string &name) const {
  if (servicectrls_map_.count(name) == 0) {
    throw std::runtime_error("Service Controller does not exist");
  }
  return servicectrls_map_.at(name);
}

std::list<std::string> PolycubedCore::get_servicectrls_names() {
  std::list<std::string> list;
  for (auto &it : servicectrls_map_) {
    list.push_back(it.first);
  }

  return list;
}

std::list<ServiceController const *> PolycubedCore::get_servicectrls_list()
    const {
  std::list<ServiceController const *> list;
  for (auto &it : servicectrls_map_) {
    list.push_back(&it.second);
  }
  return list;
}

std::string PolycubedCore::get_servicectrls() {
  logger->debug("PolycubedCore: get services");
  json j = json::array();
  for (auto &it : servicectrls_map_) {
    j += it.second.to_json();
  }
  return j.dump(4);
}

void PolycubedCore::delete_servicectrl(const std::string &name) {
  logger->debug("PolycubedCore: delete service {0}", name);
  if (servicectrls_map_.count(name) == 0) {
    logger->warn("no service present with name {0}", name);
    throw std::runtime_error("Service Controller does not exist");
  }

  // automatically destroy management grpc client instance and disconnect
  // TODO Check consistency of currently deployed cubes,
  // is necessary to undeploy them?
  servicectrls_map_.erase(name);
  logger->info("delete service {0}", name);
}

void PolycubedCore::clear_servicectrl_list() {
  servicectrls_map_.clear();
}

std::string PolycubedCore::get_cube(const std::string &name) {
  logger->debug("PolycubedCore: get cube {0}", name);
  auto cube = ServiceController::get_cube(name);
  if (cube == nullptr) {
    logger->warn("no cube present with name {0}", name);
    throw std::runtime_error("Cube does not exist");
  }

  return cube->to_json().dump(4);
}

std::string PolycubedCore::get_cubes() {
  logger->debug("PolycubedCore: get cubes");

  json j = json::object();
  for (auto &it : servicectrls_map_) {
    json j2 = json::array();
    for (auto &it2 : it.second.get_cubes()) {
      j2 += it2->to_json();
    }
    if (j2.size()) {
      j[it.first] = j2;
    }
  }
  return j.dump(4);
}

std::string PolycubedCore::get_netdev(const std::string &name) {
  logger->debug("PolycubedCore: get netdev {0}", name);
  json j = json::array();
  auto ifaces = Netlink::getInstance().get_available_ifaces();
  if (ifaces.count(name) != 0) {
    j += ifaces.at(name).toJson();
    return j.dump(4);
  }

  throw std::runtime_error("netdev " + name + "does not exist");
}

std::string PolycubedCore::get_netdevs() {
  logger->debug("PolycubedCore: get netdevs");
  json j = json::array();
  auto ifaces = Netlink::getInstance().get_available_ifaces();
  for (auto &it : ifaces) {
    j += it.second.toJson();
  }
  return j.dump(4);
}

std::string PolycubedCore::topology() {
  json j = json::array();
  auto cubes = ServiceController::get_all_cubes();

  for (auto &it : cubes) {
    j += it->to_json();
  }

  return j.dump(4);
}

std::string get_port_peer(const std::string &port) {
  std::smatch match;
  std::regex rule("(\\S+):(\\S+)");

  if (std::regex_match(port, match, rule)) {
    auto cube_ = ServiceController::get_cube(match[1]);
    if (cube_ == nullptr) {
      throw std::runtime_error("Cube does not exist");
    }
    auto cube = std::dynamic_pointer_cast<CubeIface>(cube_);
    if (!cube) {
      throw std::runtime_error("Bad cube type");
    }

    auto port = cube->get_port(match[2]);
    return port->peer();
  }

  return std::string();
}

bool PolycubedCore::try_to_set_peer(const std::string &peer1,
                                    const std::string &peer2) {
  std::smatch match;
  std::regex rule("(\\S+):(\\S+)");

  if (std::regex_match(peer1, match, rule)) {
    auto cube_ = ServiceController::get_cube(match[1]);
    if (cube_ == nullptr) {
      throw std::runtime_error("Cube does not exist");
    }
    auto cube = std::dynamic_pointer_cast<CubeIface>(cube_);
    if (!cube) {
      throw std::runtime_error("Bad cube type");
    }
    auto port = cube->get_port(match[2]);
    port->set_peer(peer2);
    return true;
  }

  return false;
}

void PolycubedCore::connect(const std::string &peer1,
                            const std::string &peer2) {
  int count = 0;
  std::string ret;

  ret = get_port_peer(peer1);
  if (!ret.empty()) {
    throw std::runtime_error(peer1 + " already has a peer " + ret);
  }

  ret = get_port_peer(peer2);
  if (!ret.empty()) {
    throw std::runtime_error(peer2 + " already has a peer " + ret);
  }

  if (try_to_set_peer(peer1, peer2)) {
    count++;
  }

  if (try_to_set_peer(peer2, peer1)) {
    count++;
  }

  if (count == 0) {
    throw std::runtime_error("Error setting peer");
  }
}

void PolycubedCore::disconnect(const std::string &peer1,
                               const std::string &peer2) {
  std::string ret1, ret2;
  ret1 = get_port_peer(peer1);
  ret2 = get_port_peer(peer2);

  if (ret1.empty() && ret2.empty()) {
    throw std::runtime_error(peer1 + " is not connected to " + peer2);
  }

  if (!ret1.empty() && ret1 != peer2) {
    throw std::runtime_error(peer1 + " is not connected to " + peer2);
  }

  if (!ret2.empty() && ret2 != peer1) {
    throw std::runtime_error(peer1 + " is not connected to " + peer2);
  }

  try_to_set_peer(peer1, "");
  try_to_set_peer(peer2, "");
}

void PolycubedCore::attach(const std::string &cube_name,
                           const std::string &port_name,
                           const std::string &position,
                           const std::string &other) {
  std::shared_ptr<TransparentCube> cube;
  std::shared_ptr<PeerIface> peer;

  auto cube_ = ServiceController::get_cube(cube_name);
  if (cube_ == nullptr) {
    throw std::runtime_error("Cube " + cube_name + " does not exist");
  }

  cube = std::dynamic_pointer_cast<TransparentCube>(cube_);
  if (!cube) {
    throw std::runtime_error("Cube " + cube_name + " is not transparent");
  }

  if (cube->get_parent()) {
    throw std::runtime_error("Cube " + cube_name + " is already attached");
  }

  std::smatch match;
  std::regex rule("(\\S+):(\\S+)");

  if (std::regex_match(port_name, match, rule)) {
    auto cube2_ = ServiceController::get_cube(match[1]);
    if (cube2_ == nullptr) {
      throw std::runtime_error("Port " + port_name + " does not exist");
    }
    auto cube2 = std::dynamic_pointer_cast<CubeIface>(cube2_);
    if (!cube2) {
      throw std::runtime_error("Cube " + std::string(match[1]) +
                               " is transparent");
    }

    auto port = cube2->get_port(match[2]);
    switch (port->get_type()) {
    case PortType::TC:
      if (cube->get_type() != CubeType::TC) {
        throw std::runtime_error(cube_name + " and " + port_name +
                                 " have incompatible types");
      }
      break;
    case PortType::XDP:
      if (cube->get_type() != CubeType::XDP_DRV &&
          cube->get_type() != CubeType::XDP_DRV) {
        throw std::runtime_error(cube_name + " and " + port_name +
                                 " have incompatible types");
      }
      break;
    }

    peer = std::dynamic_pointer_cast<PeerIface>(port);
  } else {
    std::unique_ptr<ExtIface> iface;
    if (ServiceController::ports_to_ifaces.count(port_name) == 0) {
      switch (cube->get_type()) {
      case CubeType::TC:
        iface.reset(new ExtIfaceTC(port_name));
        break;
      case CubeType::XDP_DRV:
        iface.reset(new ExtIfaceXDP(port_name, 1U << 2));
        break;
      case CubeType::XDP_SKB:
        iface.reset(new ExtIfaceXDP(port_name, 1U << 1));
        break;
      }

      ServiceController::ports_to_ifaces.emplace(
          std::piecewise_construct, std::forward_as_tuple(port_name),
          std::forward_as_tuple(std::move(iface)));
    }

    // peer = dynamic_cast<PeerIface
    // *>(ServiceController::ports_to_ifaces.at(port_name).get());
    peer = std::dynamic_pointer_cast<PeerIface>(
        ServiceController::ports_to_ifaces.at(port_name));
  }

  cube->set_parent(peer.get());
  peer->add_cube(cube.get(), position, other);
}

void PolycubedCore::detach(const std::string &cube_name,
                           const std::string &port_name) {
  std::shared_ptr<TransparentCube> cube;
  std::shared_ptr<PeerIface> peer;

  auto cube_ = ServiceController::get_cube(cube_name);
  if (cube_ == nullptr) {
    throw std::runtime_error("Cube " + cube_name + " does not exist");
  }

  cube = std::dynamic_pointer_cast<TransparentCube>(cube_);
  if (!cube) {
    throw std::runtime_error("Cube " + cube_name + " is not transparent");
  }

  std::smatch match;
  std::regex rule("(\\S+):(\\S+)");

  if (std::regex_match(port_name, match, rule)) {
    auto cube2_ = ServiceController::get_cube(match[1]);
    if (cube2_ == nullptr) {
      throw std::runtime_error("Port " + port_name + " does not exist");
    }
    auto cube2 = std::dynamic_pointer_cast<CubeIface>(cube2_);
    if (!cube2) {
      throw std::runtime_error("Cube " + std::string(match[1]) +
                               " is transparent");
    }

    auto port = cube2->get_port(match[2]);
    peer = std::dynamic_pointer_cast<PeerIface>(port);
    peer->remove_cube(cube->get_name());
  } else {
    if (ServiceController::ports_to_ifaces.count(port_name) == 0) {
      throw std::runtime_error("netdev " + port_name + " not found");
    }
    auto iface = ServiceController::ports_to_ifaces.at(port_name);
    peer = std::dynamic_pointer_cast<PeerIface>(iface);
    peer->remove_cube(cube->get_name());
    if (!iface->is_used()) {
      ServiceController::ports_to_ifaces.erase(port_name);
    }
  }

  cube->set_parent(nullptr);
}

std::string PolycubedCore::get_cube_port_parameter(
    const std::string &cube, const std::string &port,
    const std::string &parameter) {
  std::string service_name = ServiceController::get_cube_service(cube);
  auto iter = servicectrls_map_.find(service_name);
  if (iter == servicectrls_map_.end()) {
    throw std::runtime_error("Service Controller does not exist");
  }

  ServiceController &s = iter->second;

  std::string url("/" + service_name + "/" + cube + "/ports/" + port + "/" +
                  parameter);
  auto req = HttpHandleRequest(polycube::service::Http::Method::Get, url, "");
  auto res = HttpHandleResponse();

  s.managementInterface->control_handler(req, res);

  if (res.code() != polycube::service::Http::Code::Ok) {
    throw std::runtime_error("Error getting port parameter: " + res.body());
  }

  return res.body();
}

std::string PolycubedCore::set_cube_parameter(const std::string &cube,
                                              const std::string &parameter,
                                              const std::string &value) {
  std::string service_name = ServiceController::get_cube_service(cube);
  auto iter = servicectrls_map_.find(service_name);
  if (iter == servicectrls_map_.end()) {
    throw std::runtime_error("Service Controller does not exist");
  }

  ServiceController &s = iter->second;

  std::string url("/" + service_name + "/" + cube + "/" + parameter);
  auto req = HttpHandleRequest(polycube::service::Http::Method::Put, url, "");
  auto res = HttpHandleResponse();

  s.managementInterface->control_handler(req, res);

  if (res.code() != polycube::service::Http::Code::Ok) {
    throw std::runtime_error("Error setting cube parameter: " + res.body());
  }
}

void PolycubedCore::set_rest_server(RestServer *rest_server) {
  rest_server_ = rest_server;
}

RestServer *PolycubedCore::get_rest_server() {
  return rest_server_;
}

}  // namespace polycubed
}  // namespace polycube
