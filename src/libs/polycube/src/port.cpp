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

#include "polycube/services/port.h"
#include "polycube/services/port_iface.h"
#include "port_tc.h"
#include "port_xdp.h"

//#include <tins/tins.h>
#include <tins/ethernetII.h>
// using Tins::EthernetII;

namespace polycube {
namespace service {

class Port::impl {
 public:
  impl(std::shared_ptr<PortIface> &port);
  impl(Port &op);
  ~impl();
  void send_packet_out(EthernetII &packet, bool recirculate);
  void send_packet_ns(EthernetII &packet);
  int index() const;
  std::string name() const;
  void set_peer(const std::string &peer);
  const std::string &peer() const;
  const Guid &uuid() const;
  PortStatus get_status() const;
  PortType get_type() const;

  void set_conf(const nlohmann::json &conf);
  nlohmann::json to_json() const;

 private:
  std::shared_ptr<PortIface> port_;  // port in polycubed
  // FIXME: Two different constructor needed. Look at BridgePort example
};

Port::impl::impl(std::shared_ptr<PortIface> &port) : port_(port) {}

Port::impl::~impl() {}

int Port::impl::index() const {
  return port_->index();
}

std::string Port::impl::name() const {
  return port_->name();
}

void Port::impl::set_peer(const std::string &peer) {
  port_->set_peer(peer);
}

const std::string &Port::impl::peer() const {
  return port_->peer();
}

const Guid &Port::impl::uuid() const {
  return port_->uuid();
}

void Port::impl::send_packet_out(EthernetII &packet, bool recirculate) {
  /*
   * Short story:
   *   EthernetII is used instead of an array of bytes to force
   *   the checksums to be recalculated. If the packet is
   *   sent with a bad checksum, it will be dropped by the
   *   OS on the veth interfaces
   * Long story:
   *   When packets travel on veth and checksum offloading is on,
   *   the checksum is in fact never calculated, as it is
   *   impossible to have an error when the packet travels only
   *   within the kernel. So, the packet travels with the wrong
   *   checksum all across the kernel, given that the packet is
   *   *only crossing veth* interfaces in its trip. Otherwise,
   *   the checksum will be calculated.
   *   In this case, we may receive a packet in the ctrl plane
   *   which has the wrong checksum (because it comes from a
   *   veth), and it is re-injected in the kernel through a
   *   tap (we use a tap between ctrl and kernel plane).
   *   So, we have to force the recomputation of the checksum,
   *   otherwise the packet (which has the wrong checksum) will
   *   be discarded in his trip in the kernel, as the target
   *   interface (e.g., a veth again) will apply the following
   *   algorithm: (1) the packet is coming from a non-veth
   *   interface, (2) hence it may come from  the external world,
   *   (3) hence we have to check the checksum, (4) ops... the
   *   checksum is wrong, so this packet may be corrupted,
   *   (5) hence, drop the packet.
   */
  port_->send_packet_out(packet.serialize(), recirculate);
}

void Port::impl::send_packet_ns(EthernetII &packet) {
  port_->send_packet_ns(packet.serialize());
}

PortStatus Port::impl::get_status() const {
  return port_->get_status();
}

PortType Port::impl::get_type() const {
  return port_->get_type();
}

void Port::impl::set_conf(const nlohmann::json &conf) {
  return port_->set_conf(conf);
}

nlohmann::json Port::impl::to_json() const {
  return port_->to_json();
}

// PIMPL
Port::Port(std::shared_ptr<PortIface> port) : pimpl_(new Port::impl(port)) {}

Port::~Port() {}

void Port::send_packet_out(EthernetII &packet, bool recirculate) {
  return pimpl_->send_packet_out(packet, recirculate);
}

void Port::send_packet_ns(EthernetII &packet) {
  return pimpl_->send_packet_ns(packet);
}

int Port::index() const {
  return pimpl_->index();
}

std::string Port::name() const {
  return pimpl_->name();
}

std::string Port::getName() const {
  return pimpl_->name();
}

void Port::set_peer(const std::string &peer) {
  return pimpl_->set_peer(peer);
}

const std::string &Port::peer() const {
  return pimpl_->peer();
}

const Guid &Port::uuid() const {
  return pimpl_->uuid();
}

PortStatus Port::get_status() const {
  return pimpl_->get_status();
}

PortType Port::get_type() const {
  return pimpl_->get_type();
}

void Port::set_conf(const nlohmann::json &conf) {
  return pimpl_->set_conf(conf);
}

nlohmann::json Port::to_json() const {
  return pimpl_->to_json();
}

}  // namespace service
}  // namespace polycube
