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

#include "base_cube.h"
#include "bcc_mutex.h"
#include "id_generator.h"
#include "node.h"
#include "patchpanel.h"
#include "polycube/services/cube_iface.h"
#include "polycube/services/guid.h"
#include "polycube/services/json.hpp"
#include "polycube/services/port_iface.h"
#include "utils/veth.h"

#include <api/BPF.h>
#include <api/BPFTable.h>

#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>

#include <exception>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using polycube::service::CubeIface;
using polycube::service::PortIface;
using polycube::service::ProgramType;
using polycube::service::CubeType;
using polycube::service::PacketIn;
using polycube::service::PortStatus;

namespace polycube {
namespace polycubed {

class Cube : public BaseCube, public CubeIface {
 public:
  explicit Cube(const std::string &name, const std::string &service_name,
                PatchPanel &patch_panel_ingress_,
                PatchPanel &patch_panel_egress_, LogLevel level, CubeType type, bool shadow, bool span);
  virtual ~Cube();

  std::shared_ptr<PortIface> add_port(const std::string &name,
                                      const nlohmann::json &conf);
  void remove_port(const std::string &name);
  std::shared_ptr<PortIface> get_port(const std::string &name);
  std::map<std::string, std::shared_ptr<PortIface>> &get_ports();

  void update_forwarding_table(int index, int value);

  void set_conf(const nlohmann::json &conf);
  nlohmann::json to_json() const;

  const bool get_shadow() const;
  const bool get_span() const;
  void set_span(const bool value);

  const std::string get_veth_name_from_index(const int ifindex);
 protected:
  static std::string get_wrapper_code();
  uint16_t allocate_port_id();
  void release_port_id(uint16_t id);

  std::unique_ptr<ebpf::BPFArrayTable<uint32_t>> forward_chain_;

  std::map<std::string, std::shared_ptr<PortIface>> ports_by_name_;
  std::map<int, std::shared_ptr<PortIface>> ports_by_index_;
  std::set<uint16_t> free_ports_;  // keeps track of available ports

  std::map<std::string, std::shared_ptr<Veth>> veth_by_name_;
  std::map<int, std::string> ifindex_veth_;
  bool shadow_;
  bool span_;

 private:
  // ebpf wrappers
  static const std::string MASTER_CODE;
  static const std::string CUBE_WRAPPER;
};

}  // namespace polycubed
}  // namespace polycube
