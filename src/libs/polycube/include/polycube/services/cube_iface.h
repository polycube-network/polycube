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

#include "polycube/services/cube_factory.h"
#include "polycube/services/guid.h"
#include "polycube/services/port_iface.h"
#include "polycube/services/table.h"
#include "polycube/services/types.h"
#include "polycube/services/table_desc.h"

#include <map>
#include <string>

#include "polycube/services/json.hpp"

namespace polycube {
namespace service {

class PortIface;

enum class ProgramType {
  INGRESS,
  EGRESS,
};

enum class Direction {
  INGRESS,
  EGRESS,
};

enum class CubeType {
  TC,
  XDP_SKB,
  XDP_DRV,
};

class BaseCubeIface {
 public:
  virtual void reload(const std::string &code, int index, ProgramType type) = 0;
  virtual int add_program(const std::string &code, int index,
                          ProgramType type) = 0;
  virtual void del_program(int index, ProgramType type) = 0;

  virtual CubeType get_type() const = 0;
  virtual const std::string get_service_name() const = 0;
  // get unique system-wide module id
  // TODO: how is this related to uuid?
  virtual uint32_t get_id() const = 0;
  virtual uint16_t get_index(ProgramType type) const = 0;
  virtual int get_table_fd(const std::string &table_name, int index,
                           ProgramType type) = 0;
  virtual const ebpf::TableDesc &get_table_desc(const std::string &table_name, int index,
                                     ProgramType type) = 0;
                                     
  virtual void set_log_level(LogLevel level) = 0;
  virtual LogLevel get_log_level() const = 0;

  virtual const Guid &uuid() const = 0;
  virtual const std::string get_name() const = 0;

  virtual void set_conf(const nlohmann::json &conf) = 0;
  virtual nlohmann::json to_json() const = 0;
};

class CubeIface : virtual public BaseCubeIface {
 public:
  virtual std::shared_ptr<PortIface> add_port(const std::string &name,
    const nlohmann::json &conf) = 0;
  virtual void remove_port(const std::string &name) = 0;
  virtual std::shared_ptr<PortIface> get_port(const std::string &name) = 0;

  virtual void update_forwarding_table(int index, int value) = 0;

  virtual void set_conf(const nlohmann::json &conf) = 0;
  virtual nlohmann::json to_json() const = 0;

  virtual const bool get_shadow() const = 0;
  virtual const bool get_span() const = 0;
  virtual void set_span(const bool value) = 0;

  virtual const std::string get_veth_name_from_index(const int ifindex) = 0;
};

class TransparentCubeIface : virtual public BaseCubeIface {
 public:
  virtual void set_next(uint16_t next, ProgramType type) = 0;
  virtual void set_parameter(const std::string &parameter,
                             const std::string &value) = 0;
  virtual void send_packet_out(const std::vector<uint8_t> &packet, Direction direction,
                               bool recirculate = false) = 0;

  virtual void set_conf(const nlohmann::json &conf) = 0;
  virtual nlohmann::json to_json() const = 0;

  virtual void subscribe_parent_parameter(const std::string &param_name,
                                        ParameterEventCallback &callback) = 0;
  virtual void unsubscribe_parent_parameter(const std::string &param_name) = 0;
  virtual std::string get_parent_parameter(const std::string &parameter) = 0;
};
}
}
