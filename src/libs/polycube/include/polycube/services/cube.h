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

#include <map>
#include <string>

#include "polycube/services/port.h"
#include "polycube/services/utils.h"

#include "polycube/services/base_cube.h"

namespace polycube {
namespace service {

template <class PortType>
class Cube : public BaseCube {
 public:
  Cube(const nlohmann::json &conf, const std::vector<std::string> &ingress_code,
       const std::vector<std::string> &egress_code);
  virtual ~Cube();

  template <class PortConfigType>
  std::shared_ptr<PortType> add_port(const std::string &port_name,
                                     const PortConfigType &conf);
  void remove_port(const std::string &port_name);
  std::shared_ptr<PortType> get_port(const std::string &port_name);
  std::shared_ptr<PortType> get_port(int index);
  std::vector<std::shared_ptr<PortType>> get_ports() const;

  virtual void packet_in(PortType &port, PacketInMetadata &md,
                         const std::vector<uint8_t> &packet) = 0;

  void set_conf(const nlohmann::json &conf);
  nlohmann::json to_json() const;

 private:
  std::shared_ptr<CubeIface> cube_;  // pointer to the cube in polycubed
  packet_in_cb handle_packet_in;
  std::map<std::string, std::shared_ptr<PortType>> ports_by_name_;
  std::map<int, std::shared_ptr<PortType>> ports_by_id_;
};

template <class PortType>
Cube<PortType>::Cube(const nlohmann::json &conf,
                     const std::vector<std::string> &ingress_code,
                     const std::vector<std::string> &egress_code)
    : BaseCube(conf, ingress_code, egress_code) {
  // TODO: move to function
  handle_packet_in = [&](const PacketIn *md,
                         const std::vector<uint8_t> &packet) -> void {
    // This lock guarantees:
    // - port is not deleted while processing it
    // - service implementation is not deleted wile processing it
    std::lock_guard<std::mutex> guard(cube_mutex);
    if (dismounted_)
      return;

    auto &p = *ports_by_id_.at(md->port_id);
    PacketInMetadata md_;
    md_.reason = md->reason;
    md_.metadata[0] = md->metadata[0];
    md_.metadata[1] = md->metadata[1];
    md_.metadata[2] = md->metadata[2];
    packet_in(p, md_, packet);
  };

  cube_ = factory_->create_cube(
      conf, ingress_code, egress_code, handle_log_msg,
      std::bind(&Cube::set_control_plane_log_level, this, std::placeholders::_1),
      handle_packet_in);
  // TODO: where to keep this reference?, keep a double reference?
  BaseCube::cube_ = cube_;
}

template <class PortType>
Cube<PortType>::~Cube() {
  // just in case
  dismount();

  handle_packet_in = nullptr;

  factory_->destroy_cube(get_name());
}

template <class PortType>
template <class PortConfigType>
std::shared_ptr<PortType> Cube<PortType>::add_port(const std::string &port_name,
                                                   const PortConfigType &conf) {
  if (ports_by_name_.count(port_name) != 0) {
    throw std::runtime_error("Port " + port_name + " already exists");
  }

  auto cube_port = cube_->add_port(port_name, conf.getBase());

  try {
    typename std::map<std::string, std::shared_ptr<PortType>>::iterator iter;
    bool inserted;

    auto port = std::make_shared<PortType>(*this, cube_port, conf);

    std::tie(iter, inserted) = ports_by_name_.emplace(port_name, port);
    ports_by_id_.emplace(port->index(), port);
    return iter->second;
  } catch (const std::exception &ex) {
    cube_->remove_port(port_name);
    throw;
  }
}

template <class PortType>
std::vector<std::shared_ptr<PortType>> Cube<PortType>::get_ports() const {
  std::vector<std::shared_ptr<PortType>> ports;
  for (auto &it : ports_by_name_)
    ports.push_back(it.second);

  return ports;
}

template <class PortType>
void Cube<PortType>::remove_port(const std::string &port_name) {
  // avoid deleting a port while processing a packet_in event
  std::lock_guard<std::mutex> guard(cube_mutex);

  if (ports_by_name_.count(port_name) == 0) {
    throw std::runtime_error("Port " + port_name + " does not exist");
  }

  cube_->remove_port(port_name);
  auto id = ports_by_name_.at(port_name)->index();
  ports_by_name_.erase(port_name);
  ports_by_id_.erase(id);
}

template <class PortType>
std::shared_ptr<PortType> Cube<PortType>::get_port(
    const std::string &port_name) {
  if (ports_by_name_.count(port_name) == 0) {
    throw std::runtime_error("Port " + port_name + " does not exist");
  }
  return ports_by_name_.at(port_name);
}

template <class PortType>
std::shared_ptr<PortType> Cube<PortType>::get_port(int index) {
  if (ports_by_id_.count(index) == 0) {
    throw std::runtime_error("Port id " + std::to_string(index) +
                             " does not exist");
  }
  return ports_by_id_.at(index);
}

template <class PortType>
void Cube<PortType>::set_conf(const nlohmann::json &conf) {
  return cube_->set_conf(conf);
}

template <class PortType>
nlohmann::json Cube<PortType>::to_json() const {
  return cube_->to_json();
}

}  // namespace service
}  // namespace polycube
