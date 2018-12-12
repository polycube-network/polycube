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

#include "Bridge.h"

#include <linux/if_ether.h>

const std::string mode_to_string(PortMode mode) {
  switch (mode) {
  case PortMode::ACCESS:
    return "access";
  case PortMode::TRUNK:
    return "trunk";
  default:
    throw std::runtime_error("Bad port mode");
  }
}

PortMode string_to_mode(const std::string &str) {
  if (str == "access")
    return PortMode::ACCESS;
  else if (str == "trunk")
    return PortMode::TRUNK;
  else
    throw std::runtime_error("Port mode is invalid");
}


Bridge::Bridge(const std::string &name, BridgeSchema &conf, CubeType type, const std::string &code)
    : aging_time_(300),
    stp_enabled_(conf.stpenabledIsSet() ? conf.getStpenabled() : false),
    Cube(name, {generate_code(conf.stpenabledIsSet() ? conf.getStpenabled() : false)}, {}, type, polycube::LogLevel::INFO) {
  logger()->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [bridge] [%n] [%l] %v");
  conf.setUuid(get_uuid().str());
  conf.setAgingtime(aging_time_);
}

Bridge::~Bridge() {
  Cube::dismount();
}

bool Bridge::isStpEnabled(){
  return stp_enabled_;
}

PortsSchema Bridge::add_port(const std::string &port_name, const PortsSchema &port) {
  std::lock_guard<std::mutex> guard(ports_mutex_);

  // Add port to the cube
  auto p = Cube::add_port<PortsSchema>(port_name, port);
  PortsSchema portsSchema;
  portsSchema.setName(port_name);
  portsSchema.setUuid(p->uuid().str());

  // TODO: :Fill other parameters in the portSchema object

  return portsSchema;
}

void Bridge::remove_port(const std::string &port_name) {
  std::lock_guard<std::mutex> guard(ports_mutex_);
  // remove port fro the Cube
  Cube::remove_port(port_name);
}

PortsSchema Bridge::read_bridge_ports_by_id(const std::string &portsName) {
  auto && p = get_bridge_port(portsName);
  PortsSchema sch;
  sch.setName(portsName);
  //sch.setStatus(); TODO: what is status?
  //sch.setAddress(); TODO: what is address?
  sch.setMode(mode_to_string(p.mode()));
  if (p.mode() == PortMode::ACCESS) {
    PortsAccessSchema access_sch;
    access_sch.setVlanid(p.access_vlan());
    sch.setAccess(access_sch);
  } else if (p.mode() == PortMode::TRUNK) {
    PortsTrunkSchema trunk_sch;
    trunk_sch.setNativevlan(p.native_vlan());
    sch.setTrunk(trunk_sch);
    // TODO: how to add allowed vlans?
  }

  sch.setPeer(p.peer());
  sch.setUuid(p.uuid().str());
  return sch;
}

std::string Bridge::generate_code(bool stp_enabled) {
  std::string aging_time_str("#define AGING_TIME " +
    std::to_string(aging_time_ ? aging_time_ : 300));
  return aging_time_str + (stp_enabled ? bridge_code : bridge_code_no_stp);
}

void Bridge::packet_in(BridgePort &port, PacketInMetadata &md, const std::vector<uint8_t> &packet) {
  try {
    //logger()->debug("[{0}] packet from port: '{1}' id: '{2}' peer:'{3}'",
    //  get_name(), port.name(), port.index(), port.peer());

    switch (static_cast<SlowPathReason>(md.reason)) {
    case SlowPathReason::BROADCAST:
      logger()->trace("reason: BROADCAST");
      broadcast_packet(port, md, packet);
      break;
    case SlowPathReason::BPDU:
      //logger()->debug("[{0}] reason: BPDU", get_name());
      // TODO: check that the bpdu is allowed
      if (stp_enabled_)
        process_bpdu(port, md, packet);
      else
        broadcast_packet(port, md, packet);
      break;
    default:
      logger()->error("Not valid reason {0} received", md.reason);
    }
  } catch (const std::exception &e) {
    logger()->error("exception during slowpath packet processing: '{0}'", e.what());
  }
}

void Bridge::broadcast_packet(Port &port, PacketInMetadata &md,
                              const std::vector<uint8_t> &packet) {
  bool tagged = md.metadata[1];
  uint16_t vlan = md.metadata[0];
  logger()->trace("metadata vlan:{0} tagged:{1}", vlan, tagged);

  // avoid adding or removing ports while flooding packet
  std::lock_guard<std::mutex> guard(ports_mutex_);

  for (auto &it : get_ports()) {
    if (it->name() == port.name()) {
      continue;
    }
    it->send_packet_out(packet, tagged, vlan);
  }
}

void Bridge::process_bpdu(Port &port, PacketInMetadata &md,
                          const std::vector<uint8_t> &packet) {
  bool tagged = md.metadata[1];
  unsigned int vlan = md.metadata[0];

  int port_id = port.index();
  if (stps_.count(vlan) == 0) {
    logger()->error("stp instance not present");
    return;
  }

  size_t bpdusize = 0;
  int start = ETH_HEADER_LEN + LLC_HEADER_LEN;

  // build an Ethernet packet to get the STP BPDU later on
  EthernetII ether(&packet[0], packet.size());
  if (tagged) {
    Dot1Q &dot1 = ether.rfind_pdu<Dot1Q>();
    bpdusize = dot1.payload_type() - LLC_HEADER_LEN;
    start += DOT1Q_HEADER_LEN;
  } else {
    bpdusize = ether.payload_type() - LLC_HEADER_LEN;
  }

  // take last part of vector
  std::vector<uint8_t>::const_iterator first = packet.begin() + start;
  std::vector<uint8_t>::const_iterator last = packet.begin() + start + bpdusize;
  std::vector<uint8_t> p(first, last);
  get_stp_instance(vlan).process_bpdu(port_id, p);
}

BridgePort &Bridge::get_bridge_port(const std::string &name) {
  return *Cube::get_port(name);
}

BridgePort &Bridge::get_bridge_port(int port_id) {
  return *Cube::get_port(port_id);
}

BridgeSTP &Bridge::get_stp_instance(uint16_t vlan_id, bool create) {
  if (!stp_enabled_) {
    throw std::runtime_error("STP is disabled");
  }
  if (stps_.count(vlan_id) == 0) {
    if (!create) {
      throw std::runtime_error("STP instance does not exist");
    }
    logger()->debug("creating stp instance for vlan {0}", vlan_id);
    stps_.emplace(std::piecewise_construct,
                  std::forward_as_tuple(vlan_id),
                  std::forward_as_tuple(*this, vlan_id));
  }
  return stps_.at(vlan_id);
}

BridgePort::BridgePort(polycube::service::Cube<BridgePort> &parent_,
             std::shared_ptr<polycube::service::PortIface> port_,
             const PortsSchema &conf)
  : Port(port_), parent(static_cast<Bridge&>(parent_)),
  mode_(PortMode::ACCESS), access_vlan_(0),
  l(spdlog::get("pcn-bridge")) {

  auto ports_table = parent.get_hash_table<uint32_t, port>("ports");
  port value {
    .mode = static_cast<uint16_t>(PortMode::ACCESS),
    .native_vlan = 0,
  };
  ports_table.set(index(), value);

  set_access_vlan(1);
}

BridgePort::~BridgePort() {
  cleanup();
  auto ports_table = parent.get_hash_table<uint32_t, port>("ports");
  ports_table.remove(index());
}

PortMode BridgePort::mode() const {
  return mode_;
}

void BridgePort::set_mode(PortMode mode) {
  if (mode_ == mode)
    return;

  cleanup();

  auto ports_table = parent.get_hash_table<uint32_t, port>("ports");

  port value {
    .mode = static_cast<uint16_t>(mode),
    .native_vlan = 0,
  };

  ports_table.set(index(), value);

  mode_ = mode;
}

uint16_t BridgePort::access_vlan() const {
  if (mode_ != PortMode::ACCESS)
    throw std::runtime_error("Port is not in access mode");
  return access_vlan_;
}

void BridgePort::set_access_vlan(uint16_t vlan) {
  if (mode_ != PortMode::ACCESS)
    throw std::runtime_error("Port is not in access mode");

  if (access_vlan_ == vlan)
    return;

  uint16_t port_id = index();
  if (parent.stp_enabled_ && access_vlan_ != 0) {
    try {
      parent.get_stp_instance(access_vlan_).remove_port(port_id);
    } catch (...){};
  }

  port_vlan_key key {
    .port = port_id,
    .vlan = VLAN_WILDCARD,
  };

  port_vlan_value value {
    .vlan = vlan,
    .stp_state = STP_BLOCKING,
  };

  auto port_vlan_table = parent.get_hash_table<port_vlan_key, port_vlan_value>("port_vlan");
  port_vlan_table.set(key, value);
  if (parent.stp_enabled_)
    parent.get_stp_instance(vlan, true).add_port(port_id);
  access_vlan_ = vlan;
}

void BridgePort::add_trunk_vlan(uint16_t vlan) {
  if (mode_ != PortMode::TRUNK)
    throw std::runtime_error("Port is not in trunk mode");

  if (trunk_vlans_.count(vlan) != 0)
    return; // TODO: should an error be thrown?

  uint16_t port_id = index();

  port_vlan_key key {
    .port = port_id,
    .vlan = vlan,
  };

  port_vlan_value value {
    .vlan = vlan,
    .stp_state = STP_BLOCKING,
  };

  auto port_vlan_table = parent.get_hash_table<port_vlan_key, port_vlan_value>("port_vlan");
  port_vlan_table.set(key, value);

  if (parent.isStpEnabled())
    parent.get_stp_instance(vlan, true).add_port(port_id);

  trunk_vlans_.insert(vlan);
}

void BridgePort::remove_trunk_vlan(uint16_t vlan) {
  if (mode_ != PortMode::TRUNK)
    throw std::runtime_error("Port is not in trunk mode");
  if (trunk_vlans_.count(vlan) == 0)
    return; // TODO: exeception?

  uint16_t port_id = index();

  port_vlan_key key {
    .port = port_id,
    .vlan = vlan,
  };

  auto port_vlan_table = parent.get_hash_table<port_vlan_key, port_vlan_value>("port_vlan");
  port_vlan_table.remove(key);

  if (parent.stp_enabled_)
    parent.get_stp_instance(vlan).remove_port(port_id);
  // TODO: remove stp instance if needed
  trunk_vlans_.erase(vlan);
}

uint16_t BridgePort::native_vlan() const {
  if (mode_ != PortMode::TRUNK)
    throw std::runtime_error("Port is not in trunk mode");

  return native_vlan_;
}

void BridgePort::set_native_vlan(uint16_t vlan) {
  if (mode_ != PortMode::TRUNK)
    throw std::runtime_error("Port is not in trunk mode");

  if (native_vlan_ == vlan)
    return;

  if (trunk_vlans_.count(vlan) == 0)
    throw std::runtime_error("Vlan is not allowed in port");

  port value {
    .mode = static_cast<uint16_t>(mode_),
    .native_vlan = vlan,
  };
  auto ports_table = parent.get_hash_table<uint32_t, port>("ports");
  ports_table.set(index(), value);
  native_vlan_ = vlan;
}

bool BridgePort::is_trunk_vlan_allowed(uint16_t vlan) {
  if (mode_ != PortMode::TRUNK)
    throw std::runtime_error("Port is not in trunk mode");
  return trunk_vlans_.count(vlan) != 0;
}

void BridgePort::cleanup() {
  switch(mode_) {
  case PortMode::ACCESS:
    cleanup_access_port();
    break;
  case PortMode::TRUNK:
    cleanup_trunk_port();
    break;
  }
}

void BridgePort::cleanup_trunk_port() {
  uint16_t port_id = index();

  auto port_vlan_table = parent.get_hash_table<port_vlan_key, port_vlan_value>("port_vlan");

  for (auto vlan : trunk_vlans_) {
    port_vlan_key key {
      .port = port_id,
      .vlan = vlan,
    };

    port_vlan_table.remove(key);

    if (parent.stp_enabled_)
      parent.get_stp_instance(vlan).remove_port(port_id);
  }

  trunk_vlans_.clear();
}

void BridgePort::cleanup_access_port() {
  uint16_t port_id = index();
  if (parent.stp_enabled_) {
    auto &stp_instance = parent.get_stp_instance(access_vlan_);
    stp_instance.remove_port(port_id);
  }
  // TODO: remove stp instance if needed
  port_vlan_key key {
    .port = port_id,
    .vlan = VLAN_WILDCARD,
  };

  auto port_vlan_table = parent.get_hash_table<port_vlan_key, port_vlan_value>("port_vlan");

  try {
    port_vlan_table.remove(key);
  } catch (const std::exception&) {}

  access_vlan_ = 0;
}


void BridgePort::send_packet_out(const std::vector<uint8_t> &packet,
                                 bool tagged, uint16_t vlan) {
  if (parent.stp_enabled_ && !parent.get_stp_instance(vlan).port_should_forward(index())) {
    return;
  }

  EthernetII p(&packet[0], packet.size());

  bool send_tagged;

  // vlan aware broadcast implementation
  switch(mode_) {
  case PortMode::ACCESS:
    if (vlan != access_vlan()) {
      parent.logger()->debug("port({0}) vlan({1}) != access_vlan({2})",
        name(), vlan, access_vlan());
      return;
    }
    send_tagged = false;
    break;
  case PortMode::TRUNK:
    if (!is_trunk_vlan_allowed(vlan)) {
      parent.logger()->debug("port({0}) vlan({1}) not present in trunk_vlans",
        name(), vlan);
      return;
    }

    if (vlan == native_vlan()) {
      parent.logger()->debug("port({0}) vlan({1}) packet to native vlan",
        name(), vlan);
      send_tagged = false;
    } else {
      send_tagged = true;
    }
    break;
  }

  if (tagged == send_tagged) { // send original
    Port::send_packet_out(p);
  } else if(!tagged && send_tagged) { //push vlan
    EthernetII tagged_p = EthernetII(p.dst_addr(), p.src_addr());

    Dot1Q dot1q(vlan);
    dot1q.payload_type(p.payload_type());
    tagged_p /= dot1q;

    PDU *p_inner = p.inner_pdu();
    tagged_p /= *p_inner;

    Port::send_packet_out(tagged_p);
  } else { // pop vlan
    Dot1Q &p_dot1q = p.rfind_pdu<Dot1Q>();

    EthernetII untagged_p = EthernetII(p.dst_addr(), p.src_addr());
    untagged_p.payload_type(p_dot1q.payload_type());

    PDU *p_inner = p_dot1q.inner_pdu();
    untagged_p /= *p_inner;

    Port::send_packet_out(untagged_p);
  }
}
