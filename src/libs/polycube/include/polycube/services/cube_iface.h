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

#include <map>
#include <string>

#include "polycube/services/json.hpp"

using json = nlohmann::json;

namespace polycube {
namespace service {

class PortIface;

enum class ProgramType {
  INGRESS,
  EGRESS,
};

enum class CubeType {
  TC,
  XDP_SKB,
  XDP_DRV,
};

class CubeIface {
 public:
  virtual void reload(const std::string &code, int index, ProgramType type) = 0;
  virtual int add_program(const std::string &code, int inde,
                          ProgramType type) = 0;
  virtual void del_program(int index, ProgramType type) = 0;

  virtual std::shared_ptr<PortIface> add_port(const std::string &name) = 0;
  virtual void remove_port(const std::string &name) = 0;
  virtual std::shared_ptr<PortIface> get_port(const std::string &name) = 0;

  virtual CubeType get_type() const = 0;

  // get unique system-wide module id
  // TODO: how is this related to uuid?
  virtual uint32_t get_id() const = 0;

  virtual uint16_t get_index(ProgramType type) const = 0;

  virtual void update_forwarding_table(int index, int value) = 0;

  virtual int get_table_fd(const std::string &table_name, int index,
                           ProgramType type) = 0;

  virtual void set_log_level(LogLevel level) = 0;
  virtual LogLevel get_log_level() const = 0;

  virtual const Guid &uuid() const = 0;
  virtual const std::string get_name() const = 0;

  virtual json toJson(bool include_ports = false) const = 0;
};
}
}
