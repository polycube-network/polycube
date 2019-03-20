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

#include "../interface/ArpEntryInterface.h"
#include "polycube/services/utils.h"

#include <spdlog/spdlog.h>

class Router;

using namespace io::swagger::server::model;

struct arp_entry {
  uint64_t mac;
  uint32_t port;
} __attribute__((packed));

class ArpEntry : public ArpEntryInterface {
 public:
  ArpEntry(Router &parent, const ArpEntryJsonObject &conf);
  ArpEntry(Router &parent, const std::string &mac, const std::string &ip,
           const std::string &interface);
  virtual ~ArpEntry();

  static void create(Router &parent, const std::string &address,
                     const ArpEntryJsonObject &conf);
  static std::shared_ptr<ArpEntry> getEntry(Router &parent,
                                            const std::string &address);
  static void removeEntry(Router &parent, const std::string &address);
  static std::vector<std::shared_ptr<ArpEntry>> get(Router &parent);
  static void remove(Router &parent);
  std::shared_ptr<spdlog::logger> logger();
  void update(const ArpEntryJsonObject &conf) override;
  ArpEntryJsonObject toJsonObject() override;

  /// <summary>
  /// Outgoing interface
  /// </summary>
  std::string getInterface() override;
  void setInterface(const std::string &value) override;

  /// <summary>
  /// Destination MAC address
  /// </summary>
  std::string getMac() override;
  void setMac(const std::string &value) override;

  /// <summary>
  /// Destination IP address
  /// </summary>
  std::string getAddress() override;

 private:
  Router &parent_;

  std::string mac_;
  std::string ip_;
  std::string interface_;
};
