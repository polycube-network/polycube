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

#pragma once

#include <spdlog/spdlog.h>
#include <memory>
#include <mutex>

#include "polycube/services/port_iface.h"

using polycube::service::PortIface;
using polycube::service::PortType;

#include "cube_factory_impl.h"
#include "cube_tc.h"
#include "cube_xdp.h"
#include "extiface_tc.h"
#include "extiface_xdp.h"
#include "management_interface.h"
#include "polycube/services/guid.h"
#include "polycube/services/json.hpp"
#include "port_tc.h"
#include "port_xdp.h"
#include "utils/utils.h"

namespace polycube {
namespace polycubed {

class PolycubedCore;

using json = nlohmann::json;

enum class ServiceControllerType { LIBRARY, DAEMON };

class ServiceController {
 public:
  ServiceController(const std::string &name, const std::string &path,
                    const std::string &base_url, ServiceControllerType type);
  ~ServiceController();

  json to_json() const;
  json to_json_datamodel() const;

  std::string get_name() const;
  std::string get_description() const;
  //std::map <std::string,std::vector<InfoMetric>> get_infoMetrics() const;
  std::vector<InfoMetric> get_infoMetrics() const;
  std::string get_version() const;
  std::string get_pyang_git_repo_id() const;
  std::string get_swagger_codegen_git_repo_id() const;
  std::string get_servicecontroller() const;
  std::string get_datamodel() const;
  std::vector<std::shared_ptr<BaseCubeIface>> get_cubes();

  /// Loads a service controller.
  /// \throws std::invalid_argument if the YANG data model is invalid.
  /// \throws std::runtime_error if the kernel installed version is older than
  /// the one required by the service.
  /// \throws std::domain_error if the service is somewhat malformed.
  void connect(PolycubedCore *core);
  ServiceControllerType get_type() const;
  const std::shared_ptr<ManagementInterface> get_management_interface() const;

  static std::shared_ptr<BaseCubeIface> get_cube(const std::string &name);
  static std::vector<std::shared_ptr<BaseCubeIface>> get_all_cubes();
  static bool exists_cube(const std::string &name);

  static void register_cube(std::shared_ptr<BaseCubeIface> cube,
                            const std::string &service);
  static void unregister_cube(const std::string &name);

  static std::string get_cube_service(const std::string &name);

  static void set_port_peer(Port &p, const std::string &peer_name);

  static std::unordered_map<std::string, std::shared_ptr<ExtIface>>
      ports_to_ifaces;

  static void netlink_notification(int ifindex, const std::string &ifname);

 private:
  std::shared_ptr<spdlog::logger> l;
  std::shared_ptr<ManagementInterface> management_interface_;
  std::string name_;
  std::string servicecontroller_;
  std::string base_url_;
  ServiceMetadata service_md_;
  ServiceControllerType type_;  // daemon|library
  CubeFactoryImpl factory_;

  // returns true if peer is in the cube:port format
  static bool parse_peer_name(const std::string &peer, std::string &cube,
                              std::string &port);

  // these objects save all the common objects accross different services
  static std::unordered_map<std::string, std::shared_ptr<BaseCubeIface>> cubes;
  static std::unordered_map<std::string, std::string> ports_to_ports;

  static std::unordered_map<std::string, std::string> cubes_x_service;

  static std::mutex service_ctrl_mutex_;
};

}  // namespace polycubed
}  // namespace polycube
