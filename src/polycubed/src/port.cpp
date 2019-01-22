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
#include "netlink.h"

#include "service_controller.h"

namespace polycube {
namespace polycubed {

Port::Port(CubeIface &parent, const std::string &name, uint16_t index)
    : PeerIface(port_mutex_),
      parent_(parent),
      name_(name),
      path_(parent.get_name() + ":" + name),
      index_(index),
      uuid_(GuidGenerator().newGuid()),
      peer_port_(nullptr),
      logger(spdlog::get("polycubed")) {
  netlink_notification_index = Netlink::getInstance().registerObserver(
      Netlink::Event::LINK_DELETED,
      std::bind(&Port::netlink_notification, this, std::placeholders::_1,
                std::placeholders::_2));
}

Port::~Port() {
  Netlink::getInstance().unregisterObserver(Netlink::Event::LINK_DELETED,
                                            netlink_notification_index);
}

uint16_t Port::get_port_id() const {
  return index_;  // TODO: rename this variable
}

// TODO: to be renamed
uint16_t Port::index() const {
  return index_;
}

uint16_t Port::get_index() const {
  return parent_.get_index(ProgramType::INGRESS);
}

void Port::set_next_index(uint16_t index) {
  parent_.update_forwarding_table(get_port_id(), index);
}

void Port::set_peer_iface(PeerIface *peer) {
  peer_port_ = peer;
  if (peer) {
    set_next_index(peer->get_index());
  }
}

PeerIface *Port::get_peer_iface() {
  return peer_port_;
}

const Guid &Port::uuid() const {
  return uuid_;
}

uint16_t Port::get_egress_index() const {
  return parent_.get_index(ProgramType::EGRESS);
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

void Port::netlink_notification(int ifindex, const std::string &ifname) {
  if (peer_ == ifname) {
    set_peer("");
  }
}

void Port::set_peer(const std::string &peer) {
  {
    std::lock_guard<std::mutex> guard(port_mutex_);
    if (peer_ == peer) {
      return;
    }
  }
  ServiceController::set_port_peer(*this, peer);

  std::lock_guard<std::mutex> guard(port_mutex_);
  peer_ = peer;
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

  if (recirculate) {
    module = parent_.get_index(ProgramType::INGRESS);
    port = index();
  } else if (peer_port_) {
    port = peer_port_->get_port_id();
    module = peer_port_->get_index();
  }
  c.send_packet_to_cube(module, port, packet);
}

void Port::update_indexes() {}

int Port::calculate_cube_index(int index) {}

void Port::connect(PeerIface &p1, PeerIface &p2) {
  p1.set_peer_iface(&p2);
  p2.set_peer_iface(&p1);
}

void Port::unconnect(PeerIface &p1, PeerIface &p2) {
  p1.set_peer_iface(nullptr);
  p2.set_peer_iface(nullptr);
}

}  // namespace polycubed
}  // namespace polycube
