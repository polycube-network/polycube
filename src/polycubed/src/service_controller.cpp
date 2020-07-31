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

#include "service_controller.h"
#include "management_lib.h"

#include "config.h"
#include "cube_factory_impl.h"
#include "port_xdp.h"
#include "utils/netlink.h"

#include <regex>

#include <linux/version.h>
#include <sys/utsname.h>

namespace polycube {
namespace polycubed {

std::mutex ServiceController::service_ctrl_mutex_;

std::unordered_map<std::string, std::shared_ptr<BaseCubeIface>>
    ServiceController::cubes;
std::unordered_map<std::string, std::shared_ptr<ExtIface>>
    ServiceController::ports_to_ifaces;
std::unordered_map<std::string, std::string> ServiceController::ports_to_ports;
std::unordered_map<std::string, std::string> ServiceController::cubes_x_service;

ServiceController::ServiceController(const std::string &name,
                                     const std::string &path,
                                     const std::string &base_url,
                                     const ServiceControllerType type)
    : l(spdlog::get("polycubed")),
      management_interface_{},
      name_(name),
      servicecontroller_(path),
      factory_(name),
      base_url_(base_url),
      type_(type) {}

ServiceController::~ServiceController() {
  l->info("delete service {0}", name_);
  management_interface_.reset();

  // destroy instances created by this
  for (auto it = cubes_x_service.begin(); it != cubes_x_service.end();) {
    if (it->second == name_) {
      it = cubes_x_service.erase(it);
    } else {
      it++;
    }
  }
}

ServiceControllerType ServiceController::get_type() const {
  return type_;
}

const std::shared_ptr<ManagementInterface>
ServiceController::get_management_interface() const {
  return management_interface_;
}

std::string ServiceController::get_description() const {
  if (service_md_.description.empty())
    return "Unknown";

  return service_md_.description;
}

std::string ServiceController::get_version() const {
  if (service_md_.version.empty())
    return "Unknown";
  return service_md_.version;
}

std::string ServiceController::get_pyang_git_repo_id() const {
  if (service_md_.pyangGitRepoId.empty())
    return "Unknown";
  return service_md_.pyangGitRepoId;
}

std::string ServiceController::get_swagger_codegen_git_repo_id() const {
  if (service_md_.swaggerCodegenGitRepoId.empty())
    return "Unknown";
  return service_md_.swaggerCodegenGitRepoId;
}

void ServiceController::connect(PolycubedCore *core) {
  switch (type_) {
  case ServiceControllerType::DAEMON:
    throw std::runtime_error("GRPC support has not been compiled");
    break;

  case ServiceControllerType::LIBRARY: {
    management_interface_ = std::make_shared<ManagementLib>(
        servicecontroller_, base_url_, name_, core);
    std::shared_ptr<ManagementLib> lib =
        std::dynamic_pointer_cast<ManagementLib>(management_interface_);

    service_md_ = lib->init(&factory_, configuration::config.getLogFile());
    break;
  }
  }
}

json ServiceController::to_json() const {
  json j = {{"name", get_name()},
            {"servicecontroller", get_servicecontroller()},
            {"description", get_description()},
            {"version", get_version()},
            {"pyang_repo_id", get_pyang_git_repo_id()},
            {"swagger_codegen_repo_id", get_swagger_codegen_git_repo_id()}};
  return j;
}

json ServiceController::to_json_datamodel() const {
  json j = {{"name", get_name()},
            {"servicecontroller", get_servicecontroller()},
            {"datamodel", get_datamodel()}};
  return j;
}

std::string ServiceController::get_name() const {
  return name_;
}

std::string ServiceController::get_servicecontroller() const {
  return servicecontroller_;
}

std::string ServiceController::get_datamodel() const {
  return service_md_.dataModel;
}

std::shared_ptr<BaseCubeIface> ServiceController::get_cube(
    const std::string &name) {
  std::lock_guard<std::mutex> guard(service_ctrl_mutex_);
  try {
    return cubes.at(name);
  } catch (...) {
    return nullptr;
  }
}

/* get cubes for this service */
std::vector<std::shared_ptr<BaseCubeIface>> ServiceController::get_cubes() {
  return factory_.get_cubes();
}

/* get cubes for all services */
std::vector<std::shared_ptr<BaseCubeIface>> ServiceController::get_all_cubes() {
  std::vector<std::shared_ptr<BaseCubeIface>> r;
  r.reserve(cubes.size());

  std::lock_guard<std::mutex> guard(service_ctrl_mutex_);
  for (auto &&i : cubes) {
    r.push_back(i.second);
  }

  return r;
}

bool ServiceController::exists_cube(const std::string &name) {
  return cubes.count(name) == 1;
}

void ServiceController::register_cube(std::shared_ptr<BaseCubeIface> cube,
                                      const std::string &service) {
  auto &&name = cube->get_name();
  std::lock_guard<std::mutex> guard(service_ctrl_mutex_);
  cubes.emplace(name, std::move(cube));
  cubes_x_service.emplace(name, service);
}

void ServiceController::unregister_cube(const std::string &name) {
  std::lock_guard<std::mutex> guard(service_ctrl_mutex_);
  cubes_x_service.erase(name);
  cubes.erase(name);
}

std::string ServiceController::get_cube_service(const std::string &name) {
  std::lock_guard<std::mutex> guard(service_ctrl_mutex_);
  try {
    return cubes_x_service.at(name);
  } catch (...) {
    return std::string();
  }
}

void ServiceController::set_port_peer(Port &p, const std::string &peer_name) {
  p.logger->debug("setting port peer {0} -> {1}", p.get_path(), peer_name);

  // Remove previous connection, if any
  PeerIface *peer_iface = p.get_peer_iface();
  if (peer_iface != nullptr) {
    Port::unconnect(p, *p.get_peer_iface());
    ExtIface *iface = dynamic_cast<ExtIface *>(peer_iface);
    if (iface && !iface->is_used()) {
      ports_to_ifaces.erase(p.peer());
    }
  }

  if (peer_name.empty()) {
    return;
  }

  std::lock_guard<std::mutex> guard(service_ctrl_mutex_);

  std::shared_ptr<ExtIface> iface;
  std::string cube_name, port_name;  // used if peer is cube:port syntax

  if (Netlink::getInstance().get_available_ifaces().count(peer_name) != 0) {
    auto it = ports_to_ifaces.find(peer_name);
    // if there is not entry for that iface create it
    if (it == ports_to_ifaces.end()) {
      switch (p.get_type()) {
      case PortType::TC:
        iface = std::make_shared<ExtIfaceTC>(peer_name);
        break;
      case PortType::XDP:
        PortXDP &port_xdp = dynamic_cast<PortXDP &>(p);
        iface = std::make_shared<ExtIfaceXDP>(peer_name, port_xdp.get_attach_flags());
        break;
      }
      ports_to_ifaces.emplace(std::piecewise_construct,
                              std::forward_as_tuple(peer_name),
                              std::forward_as_tuple(std::move(iface)));
    } else { // if the iface exist, check if this is used
      if (it->second->get_peer_iface()) {
        throw std::runtime_error("Iface already in use");
      }
    }
    Port::connect(p, *ports_to_ifaces.at(peer_name));
  } else if (parse_peer_name(peer_name, cube_name, port_name)) {
    auto iter = cubes.find(cube_name);
    if (iter == cubes.end()) {
      throw std::runtime_error("Cube " + cube_name + " does not exist");
    }
    auto cube_ = iter->second;
    auto cube = std::dynamic_pointer_cast<CubeIface>(cube_);
    if (!cube) {
      throw std::runtime_error("Bad cube type");
    }
    auto port_peer = cube->get_port(port_name);

    if (p.get_type() != port_peer->get_type()) {
      throw std::runtime_error(
          "It is not possible to connect TC and XDP ports");
    }

    // p -> p_peer
    ports_to_ports.emplace(p.get_path(), peer_name);

    // check if p_peer has as peer me
    auto peer_iter = ports_to_ports.find(peer_name);
    if (peer_iter != ports_to_ports.end()) {
      if (p.get_path() == peer_iter->second) {
        Port::connect(p, static_cast<Port &>(*port_peer.get()));
      }
    }
  } else {
    throw std::runtime_error("Interface  " + peer_name +
                             " does not exist or is down");
  }
}

bool ServiceController::parse_peer_name(const std::string &peer,
                                        std::string &cube, std::string &port) {
  std::smatch match;
  std::regex rule("(\\S+):(\\S+)");
  if (!std::regex_match(peer, match, rule)) {
    return false;
  }

  cube = match[1];
  port = match[2];

  return true;
}

/*
 * This function in on charge of setting the peer of ports
 * connected to netdevs when they are removed.
 * This function also removes the ExtIface object if this is
 * not already removed.
 */
void ServiceController::netlink_notification(int ifindex,
                                             const std::string &ifname) {
  std::lock_guard<std::mutex> guard(service_ctrl_mutex_);
  if (ports_to_ifaces.count(ifname) == 0) {
    return; // nothing to do here
  }

  auto iface = ports_to_ifaces.at(ifname);

  auto peer = iface->get_peer_iface();
  if (peer) {
    auto port = dynamic_cast<Port*>(peer);
    if (port) {
      port->set_peer("");
    }
  }

  // try to remove it if existed
  ports_to_ifaces.erase(ifname);
}

// For each service, the associated InfoMetric vector is returned
std::vector<InfoMetric> ServiceController::get_infoMetrics() const {
  if (service_md_.Metrics.empty()) {
    std::vector<InfoMetric> emptyMap;
    return emptyMap;
  }
  return service_md_.Metrics;
}

std::map <std::string,InfoMetric> ServiceController::get_mapInfoMetrics() const {
  if(service_md_.Metrics.empty()) {
    std::map<std::string,InfoMetric> emptyMap;
    return emptyMap;
  }
  std::map<std::string,InfoMetric> retMap;
  for(int i=0;i<service_md_.Metrics.size(); i++) {
    retMap.emplace(service_md_.Metrics[i].nameMetric, service_md_.Metrics[i]);
  }
  return retMap;
}

//return all names of running cubes of a certain service
std::vector<std::string> ServiceController::get_names_cubes() const {
    return factory_.get_names_cubes();
}


}  // namespace polycubed
}  // namespace polycube
