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

#include "polycube/services/guid.h"
#include "polycube/services/port_iface.h"

namespace Tins {
  class EthernetII;
}

using Tins::EthernetII;

namespace polycube {
namespace service {

class PortIface;

class Port {
 public:
  Port(std::shared_ptr<PortIface> port);
  ~Port();
  void send_packet_out(EthernetII &packet, Direction direction = Direction::EGRESS);
  int index() const;
  std::string name() const;
  void set_peer(const std::string &peer);
  const std::string &peer() const;
  const Guid &uuid() const;
  PortStatus get_status() const;
  PortType get_type() const;

 private:
  class impl;
  std::unique_ptr<impl> pimpl_;
};

}  // namespace service
}  // namespace polycube
