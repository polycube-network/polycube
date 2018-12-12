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


#include "../interface/PortsSecondaryipInterface.h"


#include <spdlog/spdlog.h>


class Ports;

using namespace io::swagger::server::model;

class PortsSecondaryip : public PortsSecondaryipInterface {
public:
  PortsSecondaryip(Ports &parent, const PortsSecondaryipJsonObject &conf);
  virtual ~PortsSecondaryip();

  static void create(Ports &parent, const std::string &ip, const std::string &netmask, const PortsSecondaryipJsonObject &conf);
  static std::shared_ptr<PortsSecondaryip> getEntry(Ports &parent, const std::string &ip, const std::string &netmask);
  static void removeEntry(Ports &parent, const std::string &ip, const std::string &netmask);
  static std::vector<std::shared_ptr<PortsSecondaryip>> get(Ports &parent);
  static void remove(Ports &parent);
  nlohmann::fifo_map<std::string, std::string> getKeys();
  std::shared_ptr<spdlog::logger> logger();
  void update(const PortsSecondaryipJsonObject &conf) override;
  PortsSecondaryipJsonObject toJsonObject() override;

  /// <summary>
  /// Seconadary IP address of the port
  /// </summary>
  std::string getIp() override;

  /// <summary>
  /// Secondary netmask of the port
  /// </summary>
  std::string getNetmask() override;

  //The following methods have been added manually

  static void createInControlPlane(Ports &parent, const std::string &ip, const std::string &netmask, const PortsSecondaryipJsonObject &conf);

  bool operator<(const PortsSecondaryip& p) const
  {
    return std::tie(ip_,netmask_) < std::tie(p.ip_,p.netmask_);
  }

private:
  Ports &parent_;

  //The following variables have been added manually
  std::string ip_;
  std::string netmask_;

  //The following methods have been added manually
  static void updatePortInDataPath(Ports &parent);
};

