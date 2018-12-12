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

#include "node.h"

// TODO: probably this include should not exist
#include "polycube/services/cube_factory.h"
#include "polycube/services/cube_iface.h"
#include "polycube/services/port_iface.h"
#include "polycube/services/guid.h"

#include <cstdint>
#include <functional>
#include <string>
#include <mutex>

using polycube::service::CubeIface;
using polycube::service::Direction;
using polycube::service::PortStatus;
using polycube::service::PortType;

namespace polycube {
namespace polycubed {

class ServiceController;

class Port : public polycube::service::PortIface {
  friend class ServiceController;
 public:
  Port(CubeIface &parent, const std::string &name, uint16_t index);
  ~Port();
  Port(const Port &p) = delete;
  Port &operator=(const Port &) = delete;

  uint16_t index() const;
  const Guid &uuid() const;
  uint32_t serialize_ingress() const;
  uint32_t serialize_egress() const;
  std::string name() const;
  std::string get_path() const; /* cube_name:port_name syntax*/
  bool operator==(const polycube::service::PortIface &rhs) const;
  void set_peer(const std::string &peer);
  const std::string &peer() const;
  void send_packet_out(const std::vector<uint8_t> &packet,
                       Direction direction = Direction::EGRESS);
  PortStatus get_status() const;
  PortType get_type() const;

  void netlink_notification(int ifindex, const std::string &ifname);

  static void connect(Port &p1, Node &iface);
  static void connect(Port &p1, Port &p2);
  static void unconnect(Port &p1, Node &iface);
  static void unconnect(Port &p1, Port &p2);

 protected:
  PortType type_;
  CubeIface &parent_;
  std::string name_;
  std::string path_;
  uint16_t index_;
  Guid uuid_;
  std::string peer_;
  int netlink_notification_index;

  // TODO: is it possible to merge these two into a single one?
  Port *peer_port_;
  Node *peer_iface_;

  mutable std::mutex port_mutex_;

  std::shared_ptr<spdlog::logger> logger;
};

}  // namespace polycubed
}  // namespace polycube
