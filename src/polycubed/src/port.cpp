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
#include "netlink.h"

#include "service_controller.h"

namespace polycube {
namespace polycubed {

Port::Port(CubeIface &parent, const std::string &name, uint16_t index)
    : parent_(parent),
      name_(name),
      path_(parent.get_name() + ":" + name),
      index_(index),
      uuid_(GuidGenerator().newGuid()),
      peer_port_(nullptr),
      peer_iface_(nullptr),
      logger(spdlog::get("polycubed")) {
  netlink_notification_index = Netlink::getInstance().registerObserver(Netlink::Event::LINK_DELETED,
                               std::bind(&Port::netlink_notification, this, std::placeholders::_1, std::placeholders::_2));
}

Port::~Port() {
  Netlink::getInstance().unregisterObserver(Netlink::Event::LINK_DELETED, netlink_notification_index);
}

uint16_t Port::index() const {
  return index_;
}

const Guid &Port::uuid() const {
  return uuid_;
}

uint32_t Port::serialize_ingress() const {
  return index_ << 16 | parent_.get_index(ProgramType::INGRESS);
};

uint32_t Port::serialize_egress() const {
  if (!parent_.get_index(ProgramType::EGRESS))
    return 0;
  return index_ << 16 | parent_.get_index(ProgramType::EGRESS);
};

std::string Port::name() const {
  return name_;
}

std::string Port::get_path() const {
  return path_;
}

bool Port::operator==(const PortIface &rhs) const {
  if(dynamic_cast<const Port *>(&rhs) != NULL)
    return this->uuid() == rhs.uuid();
  else
    return false;
};

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
  if (peer_port_ || peer_iface_)
    return PortStatus::UP;

  return PortStatus::DOWN;
}

PortType Port::get_type() const {
  return type_;
}

void Port::send_packet_out(const std::vector<uint8_t> &packet, Direction direction) {
  if (get_status() != PortStatus::UP) {
    logger->warn("packetout: port {0}:{1} is down", parent_.get_name(), name_);
    return;
  }

  std::lock_guard<std::mutex> guard(port_mutex_);

  Controller &c = (type_ == PortType::TC) ? Controller::get_tc_instance() :
                                            Controller::get_xdp_instance();

  uint16_t port;
  uint16_t module;

  if (direction == Direction::INGRESS) {
    module = parent_.get_index(ProgramType::INGRESS);
    port = index();
  } else if (direction == Direction::EGRESS) {
    if(peer_port_) {
      module = static_cast<Port*>(peer_port_)->parent_.get_index(ProgramType::INGRESS);
      port = peer_port_->index();
    } else if(peer_iface_) {
      module = peer_iface_->get_index();
      port = 0;
    }
  }
  c.send_packet_to_cube(module, port, packet);
}

void Port::connect(Port &p1, Node &iface) {
  std::lock_guard<std::mutex> guard(p1.port_mutex_);
  p1.peer_iface_ = &iface;
  // TODO: provide an accessor for forward chain?
  // Even smarter, should be forward chain a property of Port?
  p1.parent_.update_forwarding_table(p1.index_, iface.get_index());
}

void Port::connect(Port &p1, Port &p2) {
  std::lock_guard<std::mutex> guard1(p1.port_mutex_);
  std::lock_guard<std::mutex> guard2(p2.port_mutex_);
  p1.logger->debug("connecting ports {0} and {1}", p1.name_, p2.name_);
  p1.peer_port_ = &p2;
  p2.peer_port_ = &p1;
  p1.parent_.update_forwarding_table(p1.index_, p2.serialize_ingress());
  p2.parent_.update_forwarding_table(p2.index_, p1.serialize_ingress());
}

void Port::unconnect(Port &p1, Node &iface) {
  std::lock_guard<std::mutex> guard(p1.port_mutex_);
  p1.peer_iface_ = nullptr;
  p1.parent_.update_forwarding_table(p1.index_, 0);
}

void Port::unconnect(Port &p1, Port &p2) {
  std::lock_guard<std::mutex> guard1(p1.port_mutex_);
  std::lock_guard<std::mutex> guard2(p2.port_mutex_);
  p1.peer_port_ = nullptr;
  p2.peer_port_ = nullptr;
  p1.parent_.update_forwarding_table(p1.index_, 0);
  p2.parent_.update_forwarding_table(p2.index_, 0);
}

}  // namespace polycubed
}  // namespace polycube
