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

#include "../base/BridgeBase.h"

#include "Fdb.h"
#include "Ports.h"
#include "Stp.h"

#include <tins/dot1q.h>
#include <tins/ethernetII.h>
#include <tins/tins.h>

#include "polycube/services/utils.h"

enum class SlowPathReason { BPDU = 1, BROADCAST };

using namespace polycube::service::model;

class Bridge : public BridgeBase {
  friend class Fdb;
  friend class FdbEntry;
  friend class Ports;
  friend class PortsAccess;
  friend class PortsTrunk;

 public:
  Bridge(const std::string name, const BridgeJsonObject &conf);
  virtual ~Bridge();

  void packet_in(Ports &port, polycube::service::PacketInMetadata &md,
                 const std::vector<uint8_t> &packet) override;

  /// <summary>
  /// Entry of the ports table
  /// </summary>
  std::shared_ptr<Ports> getPorts(const std::string &name) override;
  std::vector<std::shared_ptr<Ports>> getPortsList() override;
  void addPorts(const std::string &name, const PortsJsonObject &conf) override;
  void addPortsList(const std::vector<PortsJsonObject> &conf) override;
  void replacePorts(const std::string &name,
                    const PortsJsonObject &conf) override;
  void delPorts(const std::string &name) override;
  void delPortsList() override;

  /// <summary>
  ///
  /// </summary>
  std::shared_ptr<Fdb> getFdb() override;
  void addFdb(const FdbJsonObject &value) override;
  void replaceFdb(const FdbJsonObject &conf) override;
  void delFdb() override;

  void insertFdb(const FdbJsonObject &value);
  void removeFdb();

  /// <summary>
  /// Enable/Disable the STP protocol of the bridge
  /// </summary>
  bool getStpEnabled() override;
  void setStpEnabled(const bool &value) override;

  /// <summary>
  /// Main MAC address of the bridge used by the STP
  /// </summary>
  std::string getMac() override;
  void setMac(const std::string &value) override;

  /// <summary>
  /// Per-vlan Spanning Tree Protocol Configuration
  /// Those methods generate an error, because the user is not allowed to
  /// add/remove instances of Stp
  /// </summary>
  std::shared_ptr<Stp> getStp(const uint16_t &vlan) override;
  std::vector<std::shared_ptr<Stp>> getStpList() override;
  void addStp(const uint16_t &vlan, const StpJsonObject &conf) override;
  void addStpList(const std::vector<StpJsonObject> &conf) override;
  void replaceStp(const uint16_t &vlan, const StpJsonObject &conf) override;
  void delStp(const uint16_t &vlan) override;
  void delStpList() override;

  /// Methods that are used in the code to add/remove instances of Stp
  void insertStp(const uint16_t &vlan, const StpJsonObject &conf);
  void updateStpList(const std::vector<StpJsonObject> &conf);
  void removeStp(const uint16_t &vlan);

  void reloadCodeWithAgingTime(uint32_t aging_time);
  std::shared_ptr<Stp> getStpCreate(const uint16_t &vlan);

 private:
  std::string generate_code(bool stp_enabled, uint32_t aging_time);
  void updatePorts(bool enable_stp);
  void reloadCodeWithStp(bool enabled);
  void processBPDU(Ports &port, polycube::service::PacketInMetadata &md,
                   const std::vector<uint8_t> &packet);
  void broadcastPacket(Ports &port, polycube::service::PacketInMetadata &md,
                       const std::vector<uint8_t> &packet);

  void quitAndJoin();
  void updateTimestampTimer();
  void updateTimestamp();

  std::string mac_;
  bool stp_enabled_;
  std::shared_ptr<Fdb> fdb_;
  std::unordered_map<uint16_t, std::shared_ptr<Stp>> stps_;
  std::thread timestamp_update_thread_;
  std::atomic<bool> quit_thread_;
};
