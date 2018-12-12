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
#include "netlink.h"

#include <regex>

#include <linux/version.h>
#include <sys/utsname.h>

namespace polycube {
namespace polycubed {

std::mutex ServiceController::service_ctrl_mutex_;

// FIXME: to be defined somewhere else
const static std::string HOST_PEER(":host");

std::unordered_map<std::string, std::shared_ptr<CubeIface>> ServiceController::cubes;
std::unordered_map<Guid, std::unique_ptr<Node>> ServiceController::ports_to_ifaces;
std::unordered_map<std::string, std::string> ServiceController::ports_to_ports;
std::unordered_map<std::string, std::string> ServiceController::cubes_x_service;

ServiceController::ServiceController(const std::string &name,
                                     const std::string &path)
    : l(spdlog::get("polycubed")), name_(name), servicecontroller_(path), factory_(name) {

  // TODO improve validation format
  std::size_t found;
  found = servicecontroller_.find(":");
  if (found != std::string::npos) {
    type_ = ServiceControllerType::DAEMON;
  }

  found = servicecontroller_.find(".so");
  if (found != std::string::npos) {
    type_ = ServiceControllerType::LIBRARY;
  }
}

ServiceController::~ServiceController() {
  // TODO: destroy all cubes created by this service
  switch(type_) {
    case ServiceControllerType::LIBRARY:
      managementInterface.reset();
    break;
  }
}

ServiceControllerType ServiceController::get_type() const {
  return type_;
}

std::string ServiceController::get_description() const
{
  if(!service_md_.description.empty())
    return service_md_.description;
  else {
    std::string upper_service_name = get_name();
    upper_service_name[0] = toupper(upper_service_name[0]);
    return upper_service_name + " Service";
  }
}

std::string ServiceController::get_version() const
{
  if(!service_md_.version.empty())
    return service_md_.version;

  return "Unknown";
}

std::string ServiceController::get_pyang_git_repo_id() const
{
  if(!service_md_.pyangGitRepoId.empty())
    return service_md_.pyangGitRepoId;

  return "Unknown";
}

std::string ServiceController::get_swagger_codegen_git_repo_id() const
{
  if(!service_md_.swaggerCodegenGitRepoId.empty())
    return service_md_.swaggerCodegenGitRepoId;

  return "Unknown";
}


void ServiceController::connect(std::string polycubeEndpoint) {
  switch(type_) {
  case ServiceControllerType::DAEMON:
    throw std::runtime_error("GRPC support has not been compiled");
  break;

  case ServiceControllerType::LIBRARY:
    managementInterface =
        std::make_shared<ManagementLib>(get_servicecontroller());
    std::shared_ptr<ManagementLib> lib =
        std::dynamic_pointer_cast<ManagementLib>(managementInterface);

    if (!lib->load()) {
      throw std::runtime_error("cannot load library");
    }

    service_md_ = lib->init(&factory_, configuration::config.getLogFile());
    datamodel_ = service_md_.dataModel;

    if (!service_md_.requiredKernelVersion.empty()) {
      if (!utils::check_kernel_version(service_md_.requiredKernelVersion)) {
        throw std::runtime_error("kernel version does not satisfy service requirement");
      }
    }

    break;
  }
}

json ServiceController::to_json() const {
  json j = {{"name", get_name()},
            {"servicecontroller", get_servicecontroller()},
	    {"description", get_description()},
	    {"version", get_version()},
	    {"pyang_repo_id", get_pyang_git_repo_id()},
	    {"swagger_codegen_repo_id", get_swagger_codegen_git_repo_id()}
  };
  return j;
}

std::string ServiceController::to_json_string() const {
  std::string jsonString = to_json().dump(4);
  return jsonString;
}

json ServiceController::to_json_datamodel() const {
  json j = {{"name", get_name()},
            {"servicecontroller", get_servicecontroller()},
            {"datamodel", get_datamodel()}};
  return j;
}

std::string ServiceController::to_json_string_datamodel() const {
  std::string jsonString = to_json_datamodel().dump(4);
  return jsonString;
}

std::string ServiceController::get_name() const {
  return name_;
}

std::string ServiceController::get_servicecontroller() const {
  return servicecontroller_;
}

std::string ServiceController::get_datamodel() const {
  return datamodel_;
}

std::shared_ptr<CubeIface> ServiceController::get_cube(const std::string &name) {
  std::lock_guard<std::mutex> guard(service_ctrl_mutex_);
  try {
    return cubes.at(name);
  } catch (...) {
    return nullptr;
  }
}

/* get cubes for this service */
std::vector<std::shared_ptr<CubeIface>> ServiceController::get_cubes() {
  return factory_.get_cubes();
}

/* get cubes for all services */
std::vector<std::shared_ptr<CubeIface>> ServiceController::get_all_cubes() {
  std::vector<std::shared_ptr<CubeIface>> r;
  r.reserve(cubes.size());

  std::lock_guard<std::mutex> guard(service_ctrl_mutex_);
  for (auto && i: cubes) {
    r.push_back(i.second);
  }

  return r;
}

void ServiceController::register_cube(std::shared_ptr<CubeIface> cube,
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
  if (p.peer_port_ != nullptr) {
    Port::unconnect(p, *p.peer_port_);
    ports_to_ports.erase(p.get_path());
  } else if (p.peer_iface_ != nullptr) {
    Port::unconnect(p, *p.peer_iface_);
    ports_to_ifaces.erase(p.uuid());
  }

  if (peer_name.empty()) {
    return;
  }

  std::lock_guard<std::mutex> guard(service_ctrl_mutex_);

  std::unique_ptr<Node> iface;
  std::string cube_name, port_name; // used if peer is cube:port syntax

  if (peer_name == HOST_PEER) {
    iface.reset(new PortHost(p.get_type()));
    ports_to_ifaces.emplace(std::piecewise_construct,
                            std::forward_as_tuple(p.uuid()),
                            std::forward_as_tuple(std::move(iface)));
    p.logger->info("connecting port {0} to host networking", p.name());
    Port::connect(p, *ports_to_ifaces.at(p.uuid()));
  } else if (Netlink::getInstance().get_available_ifaces().count(peer_name) != 0) {

    switch (p.get_type()) {
    case PortType::TC:
      iface.reset(new ExtIfaceTC(peer_name, static_cast<PortTC&>(p)));
      break;
    case PortType::XDP:
      iface.reset(new ExtIfaceXDP(peer_name, static_cast<PortXDP&>(p)));
      break;
    }
    ports_to_ifaces.emplace(std::piecewise_construct,
                            std::forward_as_tuple(p.uuid()),
                            std::forward_as_tuple(std::move(iface)));
    Port::connect(p, *ports_to_ifaces.at(p.uuid()));
  }  else if (parse_peer_name(peer_name, cube_name, port_name)) {
    auto iter = cubes.find(cube_name);
    if (iter == cubes.end()) {
      throw std::runtime_error("Cube " + cube_name + " does not exist");
    }
    auto cube = iter->second;
    auto port_peer = cube->get_port(port_name);

    if (p.get_type() != port_peer->get_type()) {
      throw std::runtime_error("It is not possible to connect TC and XDP ports");
    }

    // p -> p_peer
    ports_to_ports.emplace(p.get_path(), peer_name);

    // check if p_peer has as peer me
    auto peer_iter = ports_to_ports.find(peer_name);
    if (peer_iter != ports_to_ports.end()) {
      if (p.get_path() == peer_iter->second) {
        Port::connect(p, static_cast<Port&>(*port_peer.get()));
      }
    }
  } else {
    throw std::runtime_error("Interface  " + peer_name + " does not exist or is down");
  }
}

bool ServiceController::parse_peer_name(const std::string &peer,
                                        std::string &cube,
                                        std::string &port) {
  std::smatch match;
  std::regex rule("(\\S+):(\\S+)");
  if (!std::regex_match(peer, match, rule)) {
    return false;
  }

  cube = match[1];
  port = match[2];

  return true;
}

}  // namespace polycubed
}  // namespace polycube
