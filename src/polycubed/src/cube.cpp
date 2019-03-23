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

#include "cube.h"

#include "port_tc.h"
#include "port_xdp.h"

namespace polycube {
namespace polycubed {

Cube::Cube(const std::string &name, const std::string &service_name,
           PatchPanel &patch_panel_ingress, PatchPanel &patch_panel_egress,
           LogLevel level, CubeType type)
    : BaseCube(name, service_name, MASTER_CODE, patch_panel_ingress,
               patch_panel_egress, level, type) {
  std::lock_guard<std::mutex> guard(bcc_mutex);

  auto forward_ = master_program_->get_array_table<uint32_t>("forward_chain_");
  forward_chain_ = std::unique_ptr<ebpf::BPFArrayTable<uint32_t>>(
      new ebpf::BPFArrayTable<uint32_t>(forward_));

  // add free ports
  for (uint16_t i = 0; i < _POLYCUBE_MAX_PORTS; i++)
    free_ports_.insert(i);
}

Cube::~Cube() {
  for (auto &it : ports_by_name_) {
    it.second->set_peer("");
  }
}

std::string Cube::get_wrapper_code() {
  return BaseCube::get_wrapper_code() + CUBE_WRAPPER;
}

void Cube::set_conf(const nlohmann::json &conf) {
  // Ports are handled in the service implementation directly
  return BaseCube::set_conf(conf);
}

nlohmann::json Cube::to_json() const {
  nlohmann::json j;
  j.update(BaseCube::to_json());

  json ports_json = json::array();

  for (auto &it : ports_by_name_) {
    json port_json = json::object();
    port_json["name"] = it.second->name();
    auto peer = it.second->peer();

    if (peer.size() > 0) {
      port_json["peer"] = peer;
    }

    ports_json += port_json;
  }

  if (ports_json.size() > 0) {
    j["ports"] = ports_json;
  }

  return j;
}

uint16_t Cube::allocate_port_id() {
  if (free_ports_.size() < 1) {
    logger->error("there are not free ports in cube '{0}'", name_);
    throw std::runtime_error("No free ports");
  }
  uint16_t p = *free_ports_.begin();
  free_ports_.erase(p);
  return p;
}

void Cube::release_port_id(uint16_t id) {
  free_ports_.insert(id);
}

std::shared_ptr<PortIface> Cube::add_port(const std::string &name,
                                          const nlohmann::json &conf) {
  std::lock_guard<std::mutex> cube_guard(cube_mutex_);
  if (ports_by_name_.count(name) != 0) {
    throw std::runtime_error("Port " + name + " already exists");
  }
  auto id = allocate_port_id();

  std::shared_ptr<PortIface> port;

  switch (type_) {
  case CubeType::TC:
    port = std::make_shared<PortTC>(*this, name, id, conf);
    break;
  case CubeType::XDP_SKB:
  case CubeType::XDP_DRV:
    port = std::make_shared<PortXDP>(*this, name, id, conf);
    break;
  }

  ports_by_name_.emplace(name, port);
  ports_by_index_.emplace(id, port);

  // TODO: is this valid?
  cube_mutex_.unlock();

  if (conf.count("peer")) {
    port->set_peer(conf.at("peer").get<std::string>());
  }

  return std::move(port);
}

void Cube::remove_port(const std::string &name) {
  if (ports_by_name_.count(name) == 0) {
    throw std::runtime_error("Port " + name + " does not exist");
  }

  // set_peer("") has to be done before acquiring cube_guard because
  // a dead lock is possible when destroying the port:
  // lock(cube_mutex_) -> ~Port::Port() -> Port::set_peer() ->
  // ServiceController::set_{tc,xdp}_port_peer() ->
  // Port::unconnect() -> Cube::update_forwarding_table -> lock(cube_mutex_)

  ports_by_name_.at(name)->set_peer("");

  std::lock_guard<std::mutex> cube_guard(cube_mutex_);
  auto index = ports_by_name_.at(name)->index();
  ports_by_name_.erase(name);
  ports_by_index_.erase(index);
  release_port_id(index);
}

std::shared_ptr<PortIface> Cube::get_port(const std::string &name) {
  std::lock_guard<std::mutex> cube_guard(cube_mutex_);
  if (ports_by_name_.count(name) == 0) {
    throw std::runtime_error("Port " + name + " does not exist");
  }
  return ports_by_name_.at(name);
}

std::map<std::string, std::shared_ptr<PortIface>> &Cube::get_ports() {
  return ports_by_name_;
}

void Cube::update_forwarding_table(int index, int value) {
  std::lock_guard<std::mutex> cube_guard(cube_mutex_);
  if (forward_chain_)  // is the forward chain still active?
    forward_chain_->update_value(index, value);
}

const std::string Cube::MASTER_CODE = R"(
// table used to save ports to endpoint relation
BPF_TABLE_SHARED("array", int, u32, forward_chain_, _POLYCUBE_MAX_PORTS);
)";

const std::string Cube::CUBE_WRAPPER = R"(
BPF_TABLE("extern", int, u32, forward_chain_, _POLYCUBE_MAX_PORTS);
)";

}  // namespace polycubed
}  // namespace polycube
