/*
 * Copyright 2019 The Polycube Authors
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

#include "Ports.h"
#include "Bridge.h"

Ports::Ports(polycube::service::Cube<Ports> &parent,
             std::shared_ptr<polycube::service::PortIface> port,
             const PortsJsonObject &conf)
    : PortsBase(parent, port) {
  logger()->info("Creating Ports instance");

  // This MAC address is not used in the datapath. We set only the variable here
  if (conf.macIsSet()) {
    mac_ = conf.getMac();
  } else {
    mac_ = polycube::service::utils::get_random_mac();
    logger()->debug("[Ports] New port add with random mac: {0}", mac_);
  }

  if (conf.stpIsSet()) {
    insertStpList(conf.getStp());
  }

  mode_ = conf.getMode();

  // add entry in ports table
  auto ports_table = parent.get_hash_table<uint32_t, struct port>("ports");

  struct port ports_value {
    .mode = static_cast<uint16_t>(mode_), .native_vlan = 1,
    .native_vlan_enabled = true
  };

  ports_table.set(index(), ports_value);

  if (mode_ == PortsModeEnum::ACCESS)
    insertAccess(conf.getAccess());
  else
    insertTrunk(conf.getTrunk());
}

Ports::~Ports() {
  logger()->info("Destroying Ports instance");
  logger()->debug("name: {0}", name());

  if (mode_ == PortsModeEnum::ACCESS)
    removeAccess();
  else
    removeTrunk();

  auto ports_table = parent_.get_hash_table<uint32_t, port>("ports");
  ports_table.remove(index());

  // delete entries in filtering database with this port
  if (parent_.fdb_ != nullptr) {
    parent_.fdb_->flushByPort(index());
  }
}

std::string Ports::getMac() {
  return mac_;
}

PortsModeEnum Ports::getMode() {
  return mode_;
}

void Ports::setMode(const PortsModeEnum &value) {
  if (mode_ == value)
    return;

  if (value == PortsModeEnum::ACCESS) {
    removeTrunk();

    auto conf = PortsAccessJsonObject();
    conf.setVlanid(1);
    insertAccess(conf);
  } else {
    removeAccess();

    auto conf = PortsTrunkJsonObject();
    conf.setNativeVlan(1);
    insertTrunk(conf);
  }

  auto ports_table = parent_.get_hash_table<uint32_t, port>("ports");

  port ports_value{
      .mode = static_cast<uint16_t>(value),
      .native_vlan = 1,
      .native_vlan_enabled = true,
  };

  ports_table.set(index(), ports_value);

  mode_ = value;
}

std::shared_ptr<PortsAccess> Ports::getAccess() {
  if (mode_ != PortsModeEnum::ACCESS)
    throw std::runtime_error("Port is not in access mode");

  return access_;
}

void Ports::addAccess(const PortsAccessJsonObject &value) {
  throw std::runtime_error("Cannot add Access container manually");
}

void Ports::replaceAccess(const PortsAccessJsonObject &conf) {
  throw std::runtime_error("Cannot replace Access container manually");
}

void Ports::delAccess() {
  throw std::runtime_error("Cannot remove Access container manually");
}

void Ports::insertAccess(const PortsAccessJsonObject &value) {
  access_ = std::make_shared<PortsAccess>(*this, value);
}

void Ports::removeAccess() {
  access_.reset();
}

std::shared_ptr<PortsTrunk> Ports::getTrunk() {
  if (mode_ != PortsModeEnum::TRUNK)
    throw std::runtime_error("Port is not in trunk mode");

  return trunk_;
}

void Ports::addTrunk(const PortsTrunkJsonObject &value) {
  throw std::runtime_error("Cannot add Trunk container manually");
}

void Ports::replaceTrunk(const PortsTrunkJsonObject &conf) {
  throw std::runtime_error("Cannot replace Trunk container manually");
}

void Ports::delTrunk() {
  throw std::runtime_error("Cannot remove Trunk container manually");
}

void Ports::insertTrunk(const PortsTrunkJsonObject &value) {
  trunk_ = std::make_shared<PortsTrunk>(*this, value);
}

void Ports::removeTrunk() {
  trunk_.reset();
}

std::shared_ptr<PortsStp> Ports::getStp(const uint16_t &vlan) {
  std::lock_guard<std::mutex> guard(map_mutex_);

  if (stps_.count(vlan) == 0) {
    throw std::runtime_error("STP instance does not exist");
  }

  return stps_.at(vlan);
}

std::vector<std::shared_ptr<PortsStp>> Ports::getStpList() {
  std::lock_guard<std::mutex> guard(map_mutex_);

  std::vector<std::shared_ptr<PortsStp>> stpsv;

  stpsv.reserve(stps_.size());

  for (auto const &value : stps_)
    stpsv.push_back(value.second);

  return stpsv;
}

void Ports::addStp(const uint16_t &vlan, const PortsStpJsonObject &conf) {
  throw std::runtime_error("Cannot add a STP instance for a port manually");
}

void Ports::addStpList(const std::vector<PortsStpJsonObject> &conf) {
  throw std::runtime_error("Cannot add STP instances for a port manually");
}

void Ports::replaceStp(const uint16_t &vlan, const PortsStpJsonObject &conf) {
  throw std::runtime_error("Cannot replace a STP instance for a port manually");
}

void Ports::delStp(const uint16_t &vlan) {
  throw std::runtime_error("Cannot remove a STP instance for a port manually");
}

void Ports::delStpList() {
  throw std::runtime_error("Cannot remove STP instances for a port manually");
}

void Ports::insertStp(const uint16_t &vlan, const PortsStpJsonObject &conf) {
  std::lock_guard<std::mutex> guard(map_mutex_);

  if (!parent_.getStpEnabled()) {
    throw std::runtime_error("STP is disabled");
  }

  if (stps_.count(vlan) != 0) {
    throw std::runtime_error(
        "STP instance for this vlan in this port already exists");
  }

  stps_[vlan] = std::make_shared<PortsStp>(*this, conf);
}

void Ports::insertStpList(const std::vector<PortsStpJsonObject> &conf) {
  for (auto &i : conf) {
    uint16_t vlan_ = i.getVlan();
    insertStp(vlan_, i);
  }
}

void Ports::removeStp(const uint16_t &vlan) {
  std::lock_guard<std::mutex> guard(map_mutex_);

  if (!parent_.getStpEnabled()) {
    throw std::runtime_error("STP is disabled");
  }

  if (stps_.count(vlan) == 0) {
    throw std::runtime_error(
        "STP instance for this vlan in this port does not exist");
  }

  stps_.erase(vlan);
}

void Ports::removeStpList() {
  std::lock_guard<std::mutex> guard(map_mutex_);
  stps_.clear();
}

void Ports::setStpEnabled() {
  if (mode_ == PortsModeEnum::ACCESS) {
    uint16_t vlan = access_->getVlanid();

    PortsStpJsonObject conf = PortsStpJsonObject();
    conf.setVlan(vlan);
    insertStp(vlan, conf);
  } else {
    for (auto allowed : trunk_->getAllowedList()) {
      uint16_t vlan = allowed->getVlanid();
      if (vlan == 0)
        continue;

      PortsStpJsonObject conf = PortsStpJsonObject();
      conf.setVlan(vlan);
      insertStp(vlan, conf);
    }
  }
}

void Ports::setStpDisabled() {
  removeStpList();
}

PortsJsonObject Ports::toJsonObject() {
  PortsJsonObject conf;
  conf.setBase(Port::to_json());

  conf.setName(getName());
  conf.setMac(getMac());
  conf.setMode(getMode());

  if (mode_ == PortsModeEnum::ACCESS)
    conf.setAccess(getAccess()->toJsonObject());
  else
    conf.setTrunk(getTrunk()->toJsonObject());

  for (auto &i : getStpList()) {
    conf.addPortsStp(i->toJsonObject());
  }

  return conf;
}

void Ports::send_packet_out(const std::vector<uint8_t> &packet, bool tagged,
                            uint16_t vlan) {
  if (parent_.getStpEnabled() &&
      !parent_.getStp(vlan)->portShouldForward(index())) {
    return;
  }

  EthernetII p(&packet[0], packet.size());

  bool send_tagged;

  // vlan aware broadcast implementation
  switch (mode_) {
  case PortsModeEnum::ACCESS:
    if (vlan != access_->getVlanid()) {
      logger()->trace("port({0}) vlan({1}) != access_vlan({2})", name(), vlan,
                      access_->getVlanid());
      return;
    }
    send_tagged = false;
    break;
  case PortsModeEnum::TRUNK:
    if (!trunk_->isAllowed(vlan)) {
      logger()->trace("port({0}) vlan({1}) not present in trunk_vlans", name(),
                      vlan);
      return;
    }

    if (trunk_->isNative(vlan)) {
      logger()->trace("port({0}) vlan({1}) packet to native vlan", name(),
                      vlan);
      send_tagged = false;
    } else {
      send_tagged = true;
    }
    break;
  }

  if (tagged == send_tagged) {  // send original
    Port::send_packet_out(p);
  } else if (!tagged && send_tagged) {  // push vlan
    EthernetII tagged_p = EthernetII(p.dst_addr(), p.src_addr());

    Tins::Dot1Q dot1q(vlan);
    dot1q.payload_type(p.payload_type());
    tagged_p /= dot1q;

    Tins::PDU *p_inner = p.inner_pdu();
    tagged_p /= *p_inner;

    Port::send_packet_out(tagged_p);
  } else {  // pop vlan
    Tins::Dot1Q &p_dot1q = p.rfind_pdu<Tins::Dot1Q>();

    EthernetII untagged_p = EthernetII(p.dst_addr(), p.src_addr());
    untagged_p.payload_type(p_dot1q.payload_type());

    Tins::PDU *p_inner = p_dot1q.inner_pdu();
    untagged_p /= *p_inner;

    Port::send_packet_out(untagged_p);
  }
}
