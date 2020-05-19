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

#include "port.h"
#include "controller.h"
#include "extiface.h"
#include "service_controller.h"

// workaround for now
#include "polycubed_core.h"
extern polycube::polycubed::PolycubedCore *core;

const std::string prefix_port = "ns_port_";

namespace polycube {
namespace polycubed {

Port::Port(CubeIface &parent, const std::string &name, uint16_t index,
           const nlohmann::json &conf)
    : PeerIface(port_mutex_),
      parent_(parent),
      name_(name),
      path_(parent.get_name() + ":" + name),
      index_(index),
      uuid_(GuidGenerator().newGuid()),
      peer_port_(nullptr),
      logger(spdlog::get("polycubed")) {}

Port::~Port() {}

uint16_t Port::get_port_id() const {
  return index_;  // TODO: rename this variable
}

// TODO: to be renamed
uint16_t Port::index() const {
  return index_;
}

uint16_t Port::get_index() const {
  return port_index_;
}

uint16_t Port::calculate_index() const {
  // check if there is ingress-enabled transparent cube
  for (auto it = cubes_.rbegin(); it != cubes_.rend(); ++it) {
    auto index = (*it)->get_index(ProgramType::INGRESS);
    if (index) {
      return index;
    }
  }
  // there are not transparent cubes
  return get_parent_index();
}

void Port::set_next_index(uint16_t index) {
  std::lock_guard<std::mutex> guard(port_mutex_);
  // TODO
  if (index == 0) {
    return;
  }

  bool is_netdev = false;
  if (index == 0xffff) {
    // Peer is a netdev, get its index
    index = peer_port_->get_port_id();
    is_netdev = true;
  }

  // it there is loaded an egrees cube, set next on it
  for (auto it = cubes_.rbegin(); it != cubes_.rend(); ++it) {
    if ((*it)->get_index(ProgramType::EGRESS)) {
      (*it)->set_next(index, ProgramType::EGRESS, is_netdev);
      return;
    }
  }

  update_parent_fwd_table(index, is_netdev);
}

void Port::set_peer_iface(PeerIface *peer) {
  std::lock_guard<std::mutex> guard(port_mutex_);
  peer_port_ = peer;
  update_indexes();
}

PeerIface *Port::get_peer_iface() {
  std::lock_guard<std::mutex> guard(port_mutex_);
  return peer_port_;
}

void Port::set_parent_egress_next(uint16_t index) {
  Cube &parent = dynamic_cast<Cube &>(parent_);
  parent.set_egress_next(get_port_id(), index);
}

const Guid &Port::uuid() const {
  return uuid_;
}

// gets index of element that should be called when a packet comes in
uint16_t Port::get_parent_index() const {
  return parent_.get_index(ProgramType::INGRESS);
}

uint16_t Port::get_egress_index() const {
  return parent_.get_index(ProgramType::EGRESS);
}

void Port::update_parent_fwd_table(uint16_t next, bool is_netdev) {
  uint16_t id;

  if (dynamic_cast<Port *>(peer_port_)) {
    // If the peer is a port set the id of that port, it can be used by the next
    // cube to know the ingress port
    id = peer_port_->get_port_id();

  } else {
    // If the peer is a interface preserve the id of this port, it can be used
    // by the optional egress program to know the egress port
    id = get_port_id();
  }

  parent_.update_forwarding_table(index(), next | id << 16, is_netdev);
}

std::string Port::name() const {
  return name_;
}

std::string Port::get_path() const {
  return path_;
}

bool Port::operator==(const PortIface &rhs) const {
  if (dynamic_cast<const Port *>(&rhs) != NULL)
    return this->uuid() == rhs.uuid();
  else
    return false;
}

void Port::set_peer(const std::string &peer) {
  {
    std::lock_guard<std::mutex> guard(port_mutex_);
    if (peer_ == peer) {
      return;
    }

    // if previous peer was an iface, remove subscriptions
    if (auto extiface_old = dynamic_cast<ExtIface*>(peer_port_)) {
      // Subscribe to the list of events
      for (auto &it : subscription_list)
        extiface_old->unsubscribe_parameter(uuid().str(), it.first);
    }
  }

  ServiceController::set_port_peer(*this, peer);

  std::lock_guard<std::mutex> guard(port_mutex_);
  peer_ = peer;

  // Check if peer is a ExtIface
  if (auto extiface = dynamic_cast<ExtIface*>(peer_port_)) {
    // Subscribe to the list of events
    for (auto &it : subscription_list)
      extiface->subscribe_parameter(uuid().str(), it.first, it.second);
  }
}

const std::string &Port::peer() const {
  std::lock_guard<std::mutex> guard(port_mutex_);
  return peer_;
}

PortStatus Port::get_status() const {
  std::lock_guard<std::mutex> guard(port_mutex_);
  return peer_port_ ? PortStatus::UP : PortStatus::DOWN;
}

PortType Port::get_type() const {
  return type_;
}

void Port::set_conf(const nlohmann::json &conf) {
  if (conf.count("peer")) {
    set_peer(conf.at("peer").get<std::string>());
  }

  // attach transparent cubes present on the port
  if (conf.count("tcubes")) {
    for (auto &cube : conf.at("tcubes")) {
      core->attach(cube.get<std::string>(), get_path(), "last", "");
    }
  }
}

nlohmann::json Port::to_json() const {
  nlohmann::json val;

  val["name"] = name();
  val["uuid"] = uuid().str();
  val["status"] = port_status_to_string(get_status());
  val["peer"] = peer();

  const auto &cubes_names = get_cubes_names();
  if (cubes_names.size()) {
    val["tcubes"] = cubes_names;
  }

  return val;
}

void Port::send_packet_out(const std::vector<uint8_t> &packet,
                           bool recirculate) {
  if (get_status() != PortStatus::UP) {
    logger->warn("packetout: port {0}:{1} is down", parent_.get_name(), name_);
    return;
  }

  std::lock_guard<std::mutex> guard(port_mutex_);

  Controller &c = (type_ == PortType::TC) ? Controller::get_tc_instance()
                                          : Controller::get_xdp_instance();

  uint16_t port;
  uint16_t module;
  bool is_netdev = false;

  if (recirculate) {
    module = get_parent_index();
    port = index();
  } else if (peer_port_) {
    module = peer_port_->get_index();
    if (dynamic_cast<ExtIface *>(peer_port_)) {
      if (module == 0xffff) {
        // No cubes to execute, redirect the packet to the netdev
        module = peer_port_->get_port_id();
        is_netdev = true;
      }

      // If peer is an interface use this port anyway, so it can be used by the
      // possible egress program of the parent
      port = get_port_id();

    } else {
      // packet is going, set port to next one
      port = peer_port_->get_port_id();
    }
  }
  c.send_packet_to_cube(module, is_netdev, port, packet);
}

void Port::send_packet_ns(const std::vector<uint8_t> &packet) {
  std::string name_ns_port = prefix_port + name_;
  std::shared_ptr<PortIface> port_ns = parent_.get_port(name_ns_port);
  port_ns->send_packet_out(packet);
}

void Port::update_indexes() {
  uint16_t next;

  // Link cubes of ingress chain
  // port <- cubes[0] <- ... <- cubes[N-1] <- peer
  
  next = get_parent_index();
  for (int i = 0; i < cubes_.size(); i++) {
    if (cubes_[i]->get_index(ProgramType::INGRESS)) {
      cubes_[i]->set_next(next, ProgramType::INGRESS);
      next = cubes_[i]->get_index(ProgramType::INGRESS);
    }
  }

  port_index_ = next;

  if (peer_port_) {
    peer_port_->set_next_index(next);
  }

  // Link cubes of egress chain
  // peer <- cubes[N-1] <- ... <- cubes[0] <- port

  next = peer_port_ ? peer_port_->get_index() : 0;
  bool is_netdev = false;
  if (next == 0xffff) {
    // Next is a netdev, get its index
    next = peer_port_->get_port_id();
    is_netdev = true;
  }
  for (int i = cubes_.size()-1; i >= 0; i--) {
    if (cubes_[i]->get_index(ProgramType::EGRESS)) {
      cubes_[i]->set_next(next, ProgramType::EGRESS, is_netdev);
      next = cubes_[i]->get_index(ProgramType::EGRESS);
      is_netdev = false;
    }
  }

  update_parent_fwd_table(next, is_netdev);
}

void Port::connect(PeerIface &p1, PeerIface &p2) {
  p1.set_peer_iface(&p2);
  p2.set_peer_iface(&p1);
}

void Port::unconnect(PeerIface &p1, PeerIface &p2) {
  p1.set_peer_iface(nullptr);
  p2.set_peer_iface(nullptr);
}

void Port::subscribe_peer_parameter(const std::string &param_name,
                                    ParameterEventCallback &callback) {
  std::lock_guard<std::mutex> lock(port_mutex_);
  // Add event to the list
  subscription_list.emplace(param_name, callback);

  // Check if the port already has a peer (of type ExtIface)
  // Subscribe to the peer parameter only if the peer is an netdev
  // (we are not interested in align different cubes' ports).
  ExtIface* extiface = dynamic_cast<ExtIface*>(peer_port_);
  if (extiface)
    extiface->subscribe_parameter(uuid().str(), param_name, callback);
}

void Port::unsubscribe_peer_parameter(const std::string &param_name) {
  std::lock_guard<std::mutex> lock(port_mutex_);
  // Remove event from the list
  subscription_list.erase(param_name);

  // Only unsubscribe if peer is ExtIface
  ExtIface* extiface = dynamic_cast<ExtIface*>(peer_port_);
  if (extiface)
    extiface->unsubscribe_parameter(uuid().str(), param_name);
}

void Port::set_peer_parameter(const std::string &param_name,
                              const std::string &value) {
  std::lock_guard<std::mutex> lock(port_mutex_);
  // Check if peer is a ExtIface
  ExtIface* extiface = dynamic_cast<ExtIface*>(peer_port_);
  if (extiface)
    extiface->set_parameter(param_name, value);
}

void Port::subscribe_parameter(const std::string &caller,
                               const std::string &param_name,
                               ParameterEventCallback &callback) {
  core->cube_port_parameter_subscribe(parent_.get_name(), name_, caller,
                                      param_name, callback);
}

void Port::unsubscribe_parameter(const std::string &caller,
                                 const std::string &param_name) {
  core->cube_port_parameter_unsubscribe(parent_.get_name(), name_, caller,
                                        param_name);
}

std::string Port::get_parameter(const std::string &param_name) {
  return core->get_cube_port_parameter(parent_.get_name(), name(), param_name);
}

void Port::set_parameter(const std::string &param_name,
                         const std::string &value) {
  // TODO: is this already implemented in core?
  throw std::runtime_error("set_parameter not implemented in port");
}

}  // namespace polycubed
}  // namespace polycube
