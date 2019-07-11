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

// TODO: probably this include should not exist
#include "peer_iface.h"
#include "polycube/services/cube_factory.h"
#include "polycube/services/cube_iface.h"
#include "polycube/services/guid.h"
#include "polycube/services/port_iface.h"

#include <cstdint>
#include <functional>
#include <mutex>
#include <string>

using polycube::service::CubeIface;
using polycube::service::PortStatus;
using polycube::service::PortType;
using polycube::service::ParameterEventCallback;

namespace polycube {
namespace polycubed {

class ServiceController;
class ExtIface;
class TransparentCube;

class Port : public polycube::service::PortIface, public PeerIface {
  friend class ServiceController;
  friend class ExtIface;
  friend class TransparentCube;

 public:
  Port(CubeIface &parent, const std::string &name, uint16_t index,
       const nlohmann::json &conf);
  virtual ~Port();
  Port(const Port &p) = delete;
  Port &operator=(const Port &) = delete;

  // PeerIface
  uint16_t get_index() const override;
  uint16_t get_port_id() const override;
  void set_next_index(uint16_t index) override;
  void set_peer_iface(PeerIface *peer) override;
  PeerIface *get_peer_iface() override;

  // TODO: rename this
  uint16_t index() const;
  const Guid &uuid() const;
  uint16_t get_egress_index() const;
  std::string name() const;
  std::string get_path() const; /* cube_name:port_name syntax*/
  bool operator==(const polycube::service::PortIface &rhs) const;
  void set_peer(const std::string &peer);
  const std::string &peer() const;
  void send_packet_out(const std::vector<uint8_t> &packet,
                       bool recirculate = false);
  void send_packet_ns(const std::vector<uint8_t> &packet);
  PortStatus get_status() const;
  PortType get_type() const;

  virtual void set_conf(const nlohmann::json &conf);
  virtual nlohmann::json to_json() const;

  static void connect(PeerIface &p1, PeerIface &p2);
  static void unconnect(PeerIface &p1, PeerIface &p2);

  // Implementation of parameter subscription in PeerIface.
  void subscribe_parameter(const std::string &caller,
                           const std::string &param_name,
                           ParameterEventCallback &callback) override;
  void unsubscribe_parameter(const std::string &caller,
                             const std::string &param_name) override;
  std::string get_parameter(const std::string &param_name) override;
  void set_parameter(const std::string &param_name,
                     const std::string &value) override;

  // API exposed to services (PortIface)
  void subscribe_peer_parameter(const std::string &param_name,
                                ParameterEventCallback &callback);
  void unsubscribe_peer_parameter(const std::string &param_name);
  void set_peer_parameter(const std::string &param_name,
                          const std::string &value);

 protected:
  void update_indexes() override;
  void update_parent_fwd_table(uint16_t next);
  uint16_t get_parent_index() const;

  PortType type_;
  CubeIface &parent_;
  std::string name_;
  std::string path_;
  uint16_t index_;
  Guid uuid_;
  std::string peer_;

  // TODO: I know, a better name is needed
  PeerIface *peer_port_;

  mutable std::mutex port_mutex_;

  std::shared_ptr<spdlog::logger> logger;

 private:
  uint16_t port_index_; // ebpf id used by other modules to call this port
  uint16_t calculate_index() const;

  std::unordered_map<std::string, ParameterEventCallback> subscription_list;
};

}  // namespace polycubed
}  // namespace polycube
