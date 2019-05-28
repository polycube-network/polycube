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

#pragma once

#include "../base/PortsStpBase.h"
#include "stp/stp.h"

class Stp;
class Ports;

using namespace polycube::service::model;

class PortsStp : public PortsStpBase {
 public:
  PortsStp(Ports &parent, const PortsStpJsonObject &conf);
  virtual ~PortsStp();

  /// <summary>
  /// VLAN identifier for this entry
  /// </summary>
  uint16_t getVlan() override;

  /// <summary>
  /// STP port state
  /// </summary>
  PortsStpStateEnum getState() override;

  /// <summary>
  /// STP cost associated with this interface
  /// </summary>
  uint32_t getPathCost() override;
  void setPathCost(const uint32_t &value) override;

  /// <summary>
  /// Port priority of this interface
  /// </summary>
  uint8_t getPortPriority() override;
  void setPortPriority(const uint8_t &value) override;

 private:
  PortsStpStateEnum convertState(enum stp_state state);

  struct stp_port *port_;
  std::weak_ptr<Stp> stp_;
  uint16_t vlan_;
  uint32_t path_cost_ = 4;
  uint8_t priority_ = 128;
};
