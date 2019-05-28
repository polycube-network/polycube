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

#include "PortsStp.h"
#include "Bridge.h"
#include "stp/stp.h"

PortsStp::PortsStp(Ports &parent, const PortsStpJsonObject &conf)
    : PortsStpBase(parent) {
  logger()->debug("Creating PortsStp instance");

  vlan_ = conf.getVlan();

  if (conf.pathCostIsSet()) {
    path_cost_ = conf.getPathCost();
  }

  if (conf.portPriorityIsSet()) {
    priority_ = conf.getPortPriority();
  }

  stp_ = parent.parent_.getStpCreate(vlan_);
  stp_.lock()->incrementCounter();

  port_ = stp_get_port(stp_.lock()->getStpInstance(), parent_.index());

  stp_port_enable(port_);
  stp_port_set_speed(port_, 1000000);
  stp_port_set_name(port_, parent_.name().c_str());
}

PortsStp::~PortsStp() {
  logger()->debug("Destroying PortsStp instance for vlan {0}", vlan_);

  stp_port_disable(port_);

  if (!stp_.expired())
    stp_.lock()->decrementCounter();
}

uint16_t PortsStp::getVlan() {
  return vlan_;
}

PortsStpStateEnum PortsStp::getState() {
  auto state = stp_port_get_state(port_);
  return convertState(state);
}

uint32_t PortsStp::getPathCost() {
  return path_cost_;
}

void PortsStp::setPathCost(const uint32_t &value) {
  if (path_cost_ == value)
    return;

  path_cost_ = value;
  stp_port_set_path_cost(port_, value);
}

uint8_t PortsStp::getPortPriority() {
  return priority_;
}

void PortsStp::setPortPriority(const uint8_t &value) {
  if (priority_ == value)
    return;

  priority_ = value;
  stp_port_set_priority(port_, value);
}

PortsStpStateEnum PortsStp::convertState(enum stp_state state) {
  switch (state) {
  case STP_DISABLED:
    return PortsStpStateEnum::DISABLED;
  case STP_LISTENING:
    return PortsStpStateEnum::LISTENING;
  case STP_LEARNING:
    return PortsStpStateEnum::LEARNING;
  case STP_FORWARDING:
    return PortsStpStateEnum::FORWARDING;
  case STP_BLOCKING:
    return PortsStpStateEnum::BLOCKING;
  }
}
