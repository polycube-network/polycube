/*
 * Copyright 2018 The Polycube Authors
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

#include <set>

#include "PortsSecondaryip.h"
#include "Utils.h"

class Router;

/* MAX_SECONDARY_ADDRESSES definition in datapath */
#define MAX_SECONDARY_ADDRESSES 5

/* Router Port definition in datapath*/
struct r_port {
  uint32_t ip;
  uint32_t netmask;
  uint32_t secondary_ip[MAX_SECONDARY_ADDRESSES];
  uint32_t secondary_netmask[MAX_SECONDARY_ADDRESSES];
  uint64_t mac : 48;
} __attribute__((packed));

using namespace polycube::service::model;
using namespace polycube::service::utils;

class Ports : public PortsBase {
  friend class PortsSecondaryip;

 public:
  Ports(polycube::service::Cube<Ports> &parent,
      std::shared_ptr<polycube::service::PortIface> port,
      const PortsJsonObject &conf);
  virtual ~Ports();

  /// <summary>
  /// IP address and prefix of the port
  /// </summary>
  std::string getIp() override;
  void setIp(const std::string &value) override;
  void doSetIp(const std::string &new_ip);

  /// <summary>
  /// Additional IP addresses for the port
  /// </summary>
  std::shared_ptr<PortsSecondaryip> getSecondaryip(const std::string &ip) override;
  std::vector<std::shared_ptr<PortsSecondaryip>> getSecondaryipList() override;
  void addSecondaryip(const std::string &ip, const PortsSecondaryipJsonObject &conf) override;
  void addSecondaryipList(const std::vector<PortsSecondaryipJsonObject> &conf) override;
  void replaceSecondaryip(const std::string &ip, const PortsSecondaryipJsonObject &conf) override;
  void delSecondaryip(const std::string &ip) override;
  void delSecondaryipList() override;

  /// <summary>
  /// MAC address of the port
  /// </summary>
  std::string getMac() override;
  void setMac(const std::string &value) override;
  void doSetMac(const std::string &new_mac);

 protected:
  void updatePortInDataPath();

 private:
  std::string mac_;
  std::string ip_;
  std::set<PortsSecondaryip> secondary_ips_;
};
