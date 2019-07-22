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

#include "../base/ArpTableBase.h"

class Router;

using namespace polycube::service::model;

struct arp_entry {
  uint64_t mac;
  uint32_t port;
} __attribute__((packed));

class ArpTable : public ArpTableBase {
 public:
  ArpTable(Router &parent, const ArpTableJsonObject &conf);
  ArpTable(Router &parent, const std::string &mac, const std::string &ip,
          const std::string &interface);
  virtual ~ArpTable();

  /// <summary>
  /// Destination IP address
  /// </summary>
  std::string getAddress() override;

  /// <summary>
  /// Destination MAC address
  /// </summary>
  std::string getMac() override;
  void setMac(const std::string &value) override;

  /// <summary>
  /// Outgoing interface
  /// </summary>
  std::string getInterface() override;
  void setInterface(const std::string &value) override;

 private:
  std::string mac_;
  std::string ip_;
  std::string interface_;
};
