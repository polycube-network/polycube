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

#include "PortsAccess.h"
#include "Bridge.h"

PortsAccess::PortsAccess(Ports &parent, const PortsAccessJsonObject &conf)
    : PortsAccessBase(parent) {
  logger()->debug("Creating PortsAccess instance");

  setVlanid(conf.getVlanid());
}

PortsAccess::~PortsAccess() {
  logger()->debug("Destroying PortsAccess instance");

  // remove PortsStp instance and cleanup port_vlan table
  uint32_t port_id = parent_.index();

  if (parent_.parent_.stp_enabled_) {
    parent_.removeStp(vlan_id_);
  }

  port_vlan_key key{
      .port = port_id,
      .vlan = VLAN_WILDCARD,
  };

  auto port_vlan_table =
      parent_.parent_.get_hash_table<port_vlan_key, port_vlan_value>(
          "port_vlan");

  port_vlan_table.remove(key);
}

uint16_t PortsAccess::getVlanid() {
  return vlan_id_;
}

void PortsAccess::setVlanid(const uint16_t &value) {
  if (value == 0 || value > 4094) {
    throw std::runtime_error("Not a valid VLAN");
  }

  if (vlan_id_ == value)
    return;

  if (parent_.parent_.getStpEnabled()) {
    // remove stp from list
    if (vlan_id_ != 0) {
      parent_.removeStp(vlan_id_);
    }

    // add new stp to list
    PortsStpJsonObject conf = PortsStpJsonObject();
    conf.setVlan(value);
    parent_.insertStp(value, conf);
  }

  // add entry in port_vlan table
  uint16_t port_id = parent_.index();

  port_vlan_key key{
      .port = port_id,
      .vlan = VLAN_WILDCARD,
  };

  port_vlan_value port_value{
      .vlan = value,
      .stp_state = STP_BLOCKING,
  };

  auto port_vlan_table =
      parent_.parent_.get_hash_table<port_vlan_key, port_vlan_value>(
          "port_vlan");

  port_vlan_table.set(key, port_value);

  vlan_id_ = value;
}
