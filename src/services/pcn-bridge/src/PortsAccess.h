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

#include "../base/PortsAccessBase.h"

#include "ext.h"

class Ports;

using namespace polycube::service::model;

class PortsAccess : public PortsAccessBase {
 public:
  PortsAccess(Ports &parent, const PortsAccessJsonObject &conf);
  virtual ~PortsAccess();

  /// <summary>
  /// VLAN associated with this interface
  /// </summary>
  uint16_t getVlanid() override;
  void setVlanid(const uint16_t &value) override;

 private:
  uint16_t vlan_id_ = 0;
};
