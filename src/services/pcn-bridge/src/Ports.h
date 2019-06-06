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

#include "../base/PortsBase.h"

#include "PortsAccess.h"
#include "PortsStp.h"
#include "PortsTrunk.h"

#include "ext.h"

class Bridge;

using namespace polycube::service::model;

class Ports : public PortsBase {
  friend class PortsAccess;
  friend class PortsTrunk;
  friend class PortsStp;

 public:
  Ports(polycube::service::Cube<Ports> &parent,
        std::shared_ptr<polycube::service::PortIface> port,
        const PortsJsonObject &conf);
  virtual ~Ports();

  /// <summary>
  /// MAC address of the port
  /// </summary>
  std::string getMac() override;

  /// <summary>
  /// Type of bridge interface: access/trunk
  /// </summary>
  PortsModeEnum getMode() override;
  void setMode(const PortsModeEnum &value) override;

  /// <summary>
  ///
  /// </summary>
  std::shared_ptr<PortsAccess> getAccess() override;
  void addAccess(const PortsAccessJsonObject &value) override;
  void replaceAccess(const PortsAccessJsonObject &conf) override;
  void delAccess() override;

  void insertAccess(const PortsAccessJsonObject &value);
  void removeAccess();

  /// <summary>
  ///
  /// </summary>
  std::shared_ptr<PortsTrunk> getTrunk() override;
  void addTrunk(const PortsTrunkJsonObject &value) override;
  void replaceTrunk(const PortsTrunkJsonObject &conf) override;
  void delTrunk() override;

  void insertTrunk(const PortsTrunkJsonObject &value);
  void removeTrunk();

  /// <summary>
  /// Per-vlan Spanning Tree Protocol Port Configuration
  /// Those methods generate an error, because the user is not allowed to
  /// add/remove instances of PortsStp
  /// </summary>
  std::shared_ptr<PortsStp> getStp(const uint16_t &vlan) override;
  std::vector<std::shared_ptr<PortsStp>> getStpList() override;
  void addStp(const uint16_t &vlan, const PortsStpJsonObject &conf) override;
  void addStpList(const std::vector<PortsStpJsonObject> &conf) override;
  void replaceStp(const uint16_t &vlan,
                  const PortsStpJsonObject &conf) override;
  void delStp(const uint16_t &vlan) override;
  void delStpList() override;

  /// Methods that are used in the code to add/remove instances of PortsStp
  void insertStp(const uint16_t &vlan, const PortsStpJsonObject &conf);
  void insertStpList(const std::vector<PortsStpJsonObject> &conf);
  void removeStp(const uint16_t &vlan);
  void removeStpList();

  void setStpEnabled();
  void setStpDisabled();

  PortsJsonObject toJsonObject() override;

  void send_packet_out(const std::vector<uint8_t> &packet, bool tagged,
                       uint16_t vlan);

 private:
  std::string mac_;
  PortsModeEnum mode_;
  std::shared_ptr<PortsAccess> access_;
  std::shared_ptr<PortsTrunk> trunk_;
  std::unordered_map<uint16_t, std::shared_ptr<PortsStp>> stps_;
  std::mutex map_mutex_;
};
