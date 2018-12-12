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


#include "../interface/FwdTableInterface.h"
#include "polycube/services/utils.h"

#include <spdlog/spdlog.h>

/* definitions copied from datapath */
// TODO: does it need padding?
struct pod {
  uint64_t mac;
  uint16_t port;
} __attribute__((packed));

class K8switch;

using namespace io::swagger::server::model;

class FwdTable : public FwdTableInterface {
 public:
  FwdTable(K8switch &parent, const FwdTableJsonObject &conf);
  FwdTable(K8switch &parent, const std::string &ip, const std::string &mac,
           const std::string &port);
  virtual ~FwdTable();

  static void create(K8switch &parent, const std::string &address, const FwdTableJsonObject &conf);
  static std::shared_ptr<FwdTable> getEntry(K8switch &parent, const std::string &address);
  static void removeEntry(K8switch &parent, const std::string &address);
  static std::vector<std::shared_ptr<FwdTable>> get(K8switch &parent);
  static void remove(K8switch &parent);
  nlohmann::fifo_map<std::string, std::string> getKeys();
  std::shared_ptr<spdlog::logger> logger();
  void update(const FwdTableJsonObject &conf) override;
  FwdTableJsonObject toJsonObject() override;

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
  /// Port where this entry is present
  /// </summary>
  std::string getPort() override;
  void setPort(const std::string &value) override;

private:
  static uint32_t get_index(const std::string &ip);
  K8switch   &parent_;
  std::string ip_;
  std::string mac_;
  std::string port_;
};

