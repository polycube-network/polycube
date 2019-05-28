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

#include "../base/PortsTrunkBase.h"

#include "PortsTrunkAllowed.h"

class Ports;

using namespace polycube::service::model;

class PortsTrunk : public PortsTrunkBase {
 public:
  PortsTrunk(Ports &parent, const PortsTrunkJsonObject &conf);
  virtual ~PortsTrunk();

  /// <summary>
  /// Allowed vlans
  /// </summary>
  std::shared_ptr<PortsTrunkAllowed> getAllowed(
      const uint16_t &vlanid) override;
  std::vector<std::shared_ptr<PortsTrunkAllowed>> getAllowedList() override;
  void addAllowed(const uint16_t &vlanid,
                  const PortsTrunkAllowedJsonObject &conf) override;
  void addAllowedList(
      const std::vector<PortsTrunkAllowedJsonObject> &conf) override;
  void replaceAllowed(const uint16_t &vlanid,
                      const PortsTrunkAllowedJsonObject &conf) override;
  void delAllowed(const uint16_t &vlanid) override;
  void delAllowedList() override;

  void insertAllowed(const uint16_t vlan);

  /// <summary>
  /// Enable/Disable the native vlan feature in this trunk port
  /// </summary>
  bool getNativeVlanEnabled() override;
  void setNativeVlanEnabled(const bool &value) override;

  /// <summary>
  /// VLAN that is not tagged in this trunk port
  /// </summary>
  uint16_t getNativeVlan() override;
  void setNativeVlan(const uint16_t &value) override;

  PortsTrunkJsonObject toJsonObject() override;

  bool isAllowed(uint16_t vlan);
  bool isNative(uint16_t vlan);

 private:
  bool native_vlan_enabled_;
  uint16_t native_vlan_;
  std::unordered_map<uint16_t, std::shared_ptr<PortsTrunkAllowed>> allowed_;
};
