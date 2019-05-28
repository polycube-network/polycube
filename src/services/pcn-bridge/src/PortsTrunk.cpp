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

#include "PortsTrunk.h"
#include "Bridge.h"

PortsTrunk::PortsTrunk(Ports &parent, const PortsTrunkJsonObject &conf)
    : PortsTrunkBase(parent) {
  logger()->debug("Creating PortsTrunk instance");

  if (conf.allowedIsSet()) {
    addAllowedList(conf.getAllowed());
  }

  // set vlan 1 allowed by default
  insertAllowed(1);
  
  setNativeVlanEnabled(conf.getNativeVlanEnabled());

  if (native_vlan_enabled_)
    setNativeVlan(conf.getNativeVlan());
}

PortsTrunk::~PortsTrunk() {
  logger()->debug("Destroying PortsTrunk instance");

  // remove PortsStp instances and entries in port_vlan table
  delAllowedList();
}

std::shared_ptr<PortsTrunkAllowed> PortsTrunk::getAllowed(
    const uint16_t &vlanid) {
  if (!isAllowed(vlanid)) {
    throw std::runtime_error("VLAN is not allowed");
  }

  return allowed_.at(vlanid);
}

std::vector<std::shared_ptr<PortsTrunkAllowed>> PortsTrunk::getAllowedList() {
  std::vector<std::shared_ptr<PortsTrunkAllowed>> allowedv;

  allowedv.reserve(allowed_.size());

  for (auto const &value : allowed_)
    allowedv.push_back(value.second);

  return allowedv;
}

void PortsTrunk::addAllowed(const uint16_t &vlanid,
                            const PortsTrunkAllowedJsonObject &conf) {
  if (isAllowed(vlanid)) {
    throw std::runtime_error(
        "This VLAN is already allowed for this trunk port");
  }

  if (vlanid == 0 || vlanid > 4094) {
    throw std::runtime_error("Not a valid VLAN");
  }

  allowed_[vlanid] = std::make_shared<PortsTrunkAllowed>(*this, conf);

  if (parent_.parent_.getStpEnabled()) {
    // add new stp to list
    PortsStpJsonObject conf_stp = PortsStpJsonObject();
    conf_stp.setVlan(vlanid);
    parent_.insertStp(vlanid, conf_stp);
  }

  // add entry in port_vlan
  uint32_t port_id = parent_.index();

  auto port_vlan_table =
      parent_.parent_.get_hash_table<port_vlan_key, port_vlan_value>(
          "port_vlan");

  port_vlan_key key{
      .port = port_id,
      .vlan = vlanid,
  };

  port_vlan_value value{
      .vlan = vlanid,
      .stp_state = STP_BLOCKING,
  };

  port_vlan_table.set(key, value);
}

void PortsTrunk::addAllowedList(
    const std::vector<PortsTrunkAllowedJsonObject> &conf) {
  PortsTrunkBase::addAllowedList(conf);
}

void PortsTrunk::replaceAllowed(const uint16_t &vlanid,
                                const PortsTrunkAllowedJsonObject &conf) {
  PortsTrunkBase::replaceAllowed(vlanid, conf);
}

void PortsTrunk::delAllowed(const uint16_t &vlanid) {
  if (!isAllowed(vlanid)) {
    throw std::runtime_error("This VLAN was not allowed in this trunk port");
  }

  allowed_.erase(vlanid);

  if (parent_.parent_.stp_enabled_) {
    // remove stp from list
    parent_.removeStp(vlanid);
  }

  // remove entry from port_vlan
  uint16_t port_id = parent_.index();

  auto port_vlan_table =
      parent_.parent_.get_hash_table<port_vlan_key, port_vlan_value>(
          "port_vlan");

  port_vlan_key key{
      .port = port_id,
      .vlan = vlanid,
  };

  port_vlan_table.remove(key);
}

void PortsTrunk::delAllowedList() {
  PortsTrunkBase::delAllowedList();
}

void PortsTrunk::insertAllowed(const uint16_t vlan) {
  try {
    auto conf_allowed = PortsTrunkAllowedJsonObject();
    conf_allowed.setVlanid(vlan);
    addAllowed(vlan, conf_allowed);
  } catch (std::runtime_error e) {
  }
}

bool PortsTrunk::getNativeVlanEnabled() {
  return native_vlan_enabled_;
}

void PortsTrunk::setNativeVlanEnabled(const bool &value) {
  if (native_vlan_enabled_ == value)
    return;

  // update entry in ports table
  auto ports_table = parent_.parent_.get_hash_table<uint32_t, port>("ports");

  port ports_value{
      .mode = static_cast<uint16_t>(PortsModeEnum::TRUNK),
      .native_vlan = native_vlan_,
      .native_vlan_enabled = value,
  };

  ports_table.set(parent_.index(), ports_value);

  native_vlan_enabled_ = value;
}

uint16_t PortsTrunk::getNativeVlan() {
  if (!native_vlan_enabled_)
    throw std::runtime_error("Native vlan is not enabled");

  return native_vlan_;
}

void PortsTrunk::setNativeVlan(const uint16_t &value) {
  if (!native_vlan_enabled_)
    throw std::runtime_error("Native vlan is not enabled");

  if (value == 0 || value > 4094)
    throw std::runtime_error("Not a valid VLAN");

  if (native_vlan_ == value)
    return;

  if (!isAllowed(value))
    logger()->warn("VLAN {0} is not allowed in this trunk port", value);

  // update entry in ports table
  auto ports_table = parent_.parent_.get_hash_table<uint32_t, port>("ports");

  port ports_value{
      .mode = static_cast<uint16_t>(PortsModeEnum::TRUNK),
      .native_vlan = value,
      .native_vlan_enabled = true,
  };

  ports_table.set(parent_.index(), ports_value);

  native_vlan_ = value;
}

bool PortsTrunk::isAllowed(uint16_t vlan) {
  return allowed_.count(vlan) != 0;
}

bool PortsTrunk::isNative(uint16_t vlan) {
  return native_vlan_enabled_ && native_vlan_ == vlan;
}

PortsTrunkJsonObject PortsTrunk::toJsonObject() {
  PortsTrunkJsonObject conf;

  for (auto &i : getAllowedList()) {
    conf.addPortsTrunkAllowed(i->toJsonObject());
  }
  conf.setNativeVlanEnabled(getNativeVlanEnabled());

  if (native_vlan_enabled_)
    conf.setNativeVlan(getNativeVlan());

  return conf;
}
