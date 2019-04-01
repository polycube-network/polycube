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

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "polycube/services/cube_iface.h"
#include "polycube/services/guid.h"

namespace polycube {
namespace service {

class CubeIface;

enum class PortStatus { DOWN, UP };

enum class PortType {
  TC,
  XDP,
};

class PortIface {
  friend class CubeIface;

 public:
  virtual void send_packet_out(const std::vector<uint8_t> &packet,
                               bool recirculate = false) = 0;
  virtual uint16_t index() const = 0;
  virtual bool operator==(const PortIface &rhs) const = 0;
  virtual std::string name() const = 0;
  virtual std::string get_path() const = 0;
  virtual void set_peer(const std::string &peer) = 0;
  virtual const std::string &peer() const = 0;
  virtual const Guid &uuid() const = 0;
  virtual PortStatus get_status() const = 0;
  virtual PortType get_type() const = 0;

  virtual void set_conf(const nlohmann::json &conf) = 0;
  virtual nlohmann::json to_json() const = 0;
};
}
}
