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


#include "../interface/PortsInterface.h"

#include "polycube/services/cube.h"
#include "polycube/services/port.h"
#include "polycube/services/utils.h"

#include <spdlog/spdlog.h>
#include <set>

#include "PortsSecondaryip.h"

class Router;

#define SECONDARY_ADDRESS 5

/* Router Port definition in datapath*/
struct r_port {
  uint32_t ip;
  uint32_t netmask;
  uint32_t secondary_ip[SECONDARY_ADDRESS];
  uint32_t secondary_netmask[SECONDARY_ADDRESS];
  uint64_t mac : 48;
} __attribute__((packed));

using namespace io::swagger::server::model;

class Ports : public polycube::service::Port, public PortsInterface {
  friend class PortsSecondaryip;
public:
  Ports(polycube::service::Cube<Ports> &parent,
        std::shared_ptr<polycube::service::PortIface> port,
        const PortsJsonObject &conf);
  virtual ~Ports();

  static void create(Router &parent, const std::string &name, const PortsJsonObject &conf);
  static std::shared_ptr<Ports> getEntry(Router &parent, const std::string &name);
  static void removeEntry(Router &parent, const std::string &name);
  static std::vector<std::shared_ptr<Ports>> get(Router &parent);
  static void remove(Router &parent);
  nlohmann::fifo_map<std::string, std::string> getKeys();
  std::shared_ptr<spdlog::logger> logger();
  void update(const PortsJsonObject &conf) override;
  PortsJsonObject toJsonObject() override;

  /// <summary>
  /// Status of the port (UP or DOWN)
  /// </summary>
  PortsStatusEnum getStatus() override;

  /// <summary>
  /// Port Name
  /// </summary>
  std::string getName() override;

  /// <summary>
  /// IP address of the port
  /// </summary>
  std::string getIp() override;
  void setIp(const std::string &value) override;

  /// <summary>
  /// Netmask of the port
  /// </summary>
  std::string getNetmask() override;
  void setNetmask(const std::string &value) override;

  /// <summary>
  /// MAC address of the port
  /// </summary>
  std::string getMac() override;
  void setMac(const std::string &value) override;

  /// <summary>
  /// Peer name, such as a network interfaces (e.g., &#39;veth0&#39;) or another cube (e.g., &#39;br1:port2&#39;)
  /// </summary>
  std::string getPeer() override;
  void setPeer(const std::string &value) override;

  /// <summary>
  /// Secondary IP address for the port
  /// </summary>
  std::shared_ptr<PortsSecondaryip> getSecondaryip(const std::string &ip, const std::string &netmask) override;
  std::vector<std::shared_ptr<PortsSecondaryip>> getSecondaryipList() override;
  void addSecondaryip(const std::string &ip, const std::string &netmask, const PortsSecondaryipJsonObject &conf) override;
  void addSecondaryipList(const std::vector<PortsSecondaryipJsonObject> &conf) override;
  void replaceSecondaryip(const std::string &ip, const std::string &netmask, const PortsSecondaryipJsonObject &conf) override;
  void delSecondaryip(const std::string &ip, const std::string &netmask) override;
  void delSecondaryipList() override;

  /// <summary>
  /// UUID of the port
  /// </summary>
  std::string getUuid() override;

private:
  Router &parent_;

  std::string mac_;
  std::string ip_;
  std::string netmask_;

  std::set<PortsSecondaryip> secondary_ips_;
};

