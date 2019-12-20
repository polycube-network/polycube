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
#include "config.h"
#include "rest_server.h"
#include "utils/netlink.h"

#include "server/Resources/Body/ListResource.h"
#include "server/Resources/Body/ParentResource.h"
#include "server/Resources/Body/LeafResource.h"

#include "server/Resources/Endpoint/ParentResource.h"

#include <pistache/client.h>
#include <regex>

using namespace Pistache::Http;
using namespace configuration;

namespace polycube {
namespace polycubed {

PolycubedCore::PolycubedCore(BaseModel *base_model)
    : base_model_(base_model), logger(spdlog::get("polycubed")),
      cubes_dump_(nullptr) {}

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

  return cube->to_json().dump(4) + " provastringa " ;
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
//metrics
json PolycubedCore::get_json_cubes() {
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
  return j;
}

//get info metrics from a yang file.
//name, type, jsonpath to calculate the value of a metric

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
    // Update the peer of a port in the cubes configuration in memory
    if (cubes_dump_) {
      cubes_dump_->UpdatePortPeer(cube->get_name(), port->name(), peer2);
    }
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
  std::shared_ptr<CubeIface> cube2;
  std::shared_ptr<PortIface> port;

  if (std::regex_match(port_name, match, rule)) {
    auto cube2_ = ServiceController::get_cube(match[1]);
    if (cube2_ == nullptr) {
      throw std::runtime_error("Port " + port_name + " does not exist");
    }
    cube2 = std::dynamic_pointer_cast<CubeIface>(cube2_);
    if (!cube2) {
      throw std::runtime_error("Cube " + std::string(match[1]) +
                               " is transparent");
    }

    port = cube2->get_port(match[2]);
    switch (port->get_type()) {
    case PortType::TC:
      if (cube->get_type() != CubeType::TC) {
        throw std::runtime_error(cube_name + " and " + port_name +
                                 " have incompatible types");
      }
      break;
    case PortType::XDP:
      if (cube->get_type() != CubeType::XDP_SKB &&
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

  int insertPosition = peer->add_cube(cube.get(), position, other);
  cube->set_parent(peer.get());

  // if it is a cube's transparent cube,
  // add it to the cubes configuration in memory
  if (!match.empty() && cubes_dump_) {
    cubes_dump_->UpdatePortTCubes(cube2->get_name(), port->name(), cube_name,
            insertPosition);
  }
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
  std::shared_ptr<CubeIface> cube2;
  std::shared_ptr<PortIface> port;

  if (std::regex_match(port_name, match, rule)) {
    auto cube2_ = ServiceController::get_cube(match[1]);
    if (cube2_ == nullptr) {
      throw std::runtime_error("Port " + port_name + " does not exist");
    }
    cube2 = std::dynamic_pointer_cast<CubeIface>(cube2_);
    if (!cube2) {
      throw std::runtime_error("Cube " + std::string(match[1]) +
                               " is transparent");
    }

    port = cube2->get_port(match[2]);
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
  // if it is a cube's transparent cube,
  // remove it from the cubes configuration in memory
  if (!match.empty() && cubes_dump_) {
    cubes_dump_->UpdatePortTCubes(cube2->get_name(), port->name(), cube_name,
            -1);
  }
}

void PolycubedCore::cube_port_parameter_subscribe(
    const std::string &cube, const std::string &port_name,
    const std::string &caller, const std::string &parameter,
    ParameterEventCallback &cb) {
  std::string outer_key(cube + ":" + port_name + ":" + parameter);

  std::lock_guard<std::mutex> lock(cubes_port_event_mutex_);
  cubes_port_event_callbacks_[outer_key][caller] = cb;

  // send a first notification with the initial value
  auto value = get_cube_port_parameter(cube, port_name, parameter);
  cb(parameter, value);
}

void PolycubedCore::cube_port_parameter_unsubscribe(
    const std::string &cube, const std::string &port_name,
    const std::string &caller, const std::string &parameter) {
  std::string outer_key(cube + ":" + port_name + ":" + parameter);

  std::lock_guard<std::mutex> lock(cubes_port_event_mutex_);

  if (cubes_port_event_callbacks_.count(outer_key) > 0 &&
      cubes_port_event_callbacks_[outer_key].count(caller) > 0) {
    cubes_port_event_callbacks_[outer_key].erase(caller);
  } else {
    throw std::runtime_error("there is not subscription for " + parameter);
  }
}

void PolycubedCore::notify_port_subscribers(
    const std::string &cube, const std::string &port_name,
    const std::string &parameter, const std::string &value) {
  std::string outer_key(cube + ":" + port_name + ":" + parameter);

  std::lock_guard<std::mutex> lock(cubes_port_event_mutex_);

  // if there are not subscribers just return
  if (cubes_port_event_callbacks_.count(outer_key) == 0) {
    return;
  }

  // TODO: what are the implications of calling the callbacks while
  // having the lock?, could it be a good idea to create a temporal copy
  // of the callbacks to be invoked and call then with the lock released?

  for (const auto &it: cubes_port_event_callbacks_[outer_key]) {
    it.second(parameter, value);
  }
}

std::string PolycubedCore::get_cube_port_parameter(
    const std::string &cube, const std::string &port_name,
    const std::string &parameter) {
  using namespace Rest::Resources::Body;

  auto service_name = ServiceController::get_cube_service(cube);
  auto &ctrl = get_service_controller(service_name);
  auto res = ctrl.get_management_interface()->get_service()->Child("ports");

  ListKeyValues k{{"ports", "name", "ports_name", ListType::kString,
                   port_name}};

  std::istringstream iss(port_name + "/" + parameter);
  for (std::string segment; std::getline(iss, segment, '/');) {
    auto current = std::dynamic_pointer_cast<ParentResource>(res);
    if (current != nullptr) {
      auto list = std::dynamic_pointer_cast<ListResource>(res);
      if (list != nullptr) {
        for (const auto &key : list->keys_) {
          ListKeyValue v{"ports", key.OriginalName(), key.Name(), key.Type(),
                         segment};
          k.emplace_back(v);
          std::getline(iss, segment, '/');  // if null raise error
        }
      }
      res = current->Child(segment);
      if (res == nullptr) {
        throw std::runtime_error("parameter not found");
      }
    } else {
      throw std::runtime_error("parameter not found");
    }
  }

  // support only leaf elements, maybe in the future we want to extend it
  // to support complex elements
  if (!std::dynamic_pointer_cast<LeafResource>(res)) {
    throw std::runtime_error("Error getting port parameters: " + parameter);
  }

  auto result = res->ReadValue(cube, k);
  std::string val(result.message);
  val = val.substr(1, val.length() - 2); // remove qoutes
  ::free(result.message);

  if (result.error_tag != ErrorTag::kOk) {
    throw std::runtime_error("Error getting port parameters: " + parameter);
  }

  return val;
}

std::string PolycubedCore::set_cube_parameter(const std::string &cube,
                                              const std::string &parameter,
                                              const std::string &value) {
  throw std::logic_error(
      "PolycubedCore::set_cube_parameter is not implemented");
}

std::vector<std::string> PolycubedCore::get_all_ports() {
  std::vector<std::string> ports;
  auto cubes = ServiceController::get_all_cubes();
  for (auto &cube_ : cubes) {
    auto cube = std::dynamic_pointer_cast<Cube>(cube_);
    if (!cube) {
      continue;
    }

    for (auto &it : cube->get_ports()) {
      ports.push_back(cube->get_name() + ":" + it.first);
    }
  }

  return std::move(ports);
}

void PolycubedCore::set_rest_server(RestServer *rest_server) {
  rest_server_ = rest_server;
}

RestServer *PolycubedCore::get_rest_server() {
  return rest_server_;
}

BaseModel *PolycubedCore::base_model() {
  return base_model_;
}

void PolycubedCore::set_cubes_dump(CubesDump *cubes_dump) {
  cubes_dump_= cubes_dump;
}

CubesDump *PolycubedCore::get_cubes_dump() {
  return cubes_dump_;
}

}  // namespace polycubed
}  // namespace polycube
