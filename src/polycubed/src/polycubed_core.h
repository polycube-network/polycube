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

#pragma once

#include <spdlog/spdlog.h>

#include <algorithm>
#include <list>
#include <string>
#include <unordered_map>

#include "base_model.h"
#include "polycube/services/guid.h"
#include "polycube/services/json.hpp"
#include "service_controller.h"

#include "cube_factory_impl.h"
#include "extiface_info.h"
#include "netlink.h"

using json = nlohmann::json;

namespace polycube {
namespace polycubed {

class RestServer;

class PolycubedCore {
 public:
  PolycubedCore(BaseModel *base_model);
  ~PolycubedCore();

  void add_servicectrl(const std::string &name, ServiceControllerType type,
                       const std::string &base_url, const std::string &path);
  std::string get_servicectrl(const std::string &name);
  const ServiceController &get_service_controller(
      const std::string &name) const;
  std::string get_servicectrls();
  std::list<std::string> get_servicectrls_names();
  std::list<ServiceController const *> get_servicectrls_list() const;
  void delete_servicectrl(const std::string &name);
  void clear_servicectrl_list();

  std::string get_cube(const std::string &name);
  std::string get_cubes();

  std::string get_netdev(const std::string &name);
  std::string get_netdevs();

  std::string topology();

  void connect(const std::string &peer1, const std::string &peer2);
  void disconnect(const std::string &peer1, const std::string &peer2);
  void attach(const std::string &cube_name, const std::string &port_name,
              const std::string &position, const std::string &other);
  void detach(const std::string &cube_name, const std::string &port_name);

  void set_polycubeendpoint(std::string &polycube);
  std::string get_polycubeendpoint();

  std::string get_cube_port_parameter(const std::string &cube,
                                      const std::string &port_name,
                                      const std::string &parameter);
  std::string set_cube_parameter(const std::string &cube,
                                 const std::string &parameter,
                                 const std::string &value);

  void set_rest_server(RestServer *rest_server);
  RestServer *get_rest_server();
  BaseModel *base_model();

 private:
  bool try_to_set_peer(const std::string &peer1, const std::string &peer2);

  std::unordered_map<std::string, ServiceController> servicectrls_map_;
  std::string polycubeendpoint_;
  std::shared_ptr<spdlog::logger> logger;
  RestServer *rest_server_;
  BaseModel *base_model_;
};

}  // namespace polycubed
}  // namespace polycube
