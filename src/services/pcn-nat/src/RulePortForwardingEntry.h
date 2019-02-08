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

#include "../interface/RulePortForwardingEntryInterface.h"
#include "IpAddr.h"

#include <spdlog/spdlog.h>

class RulePortForwarding;

using namespace io::swagger::server::model;

class RulePortForwardingEntry : public RulePortForwardingEntryInterface {
 public:
  RulePortForwardingEntry(RulePortForwarding &parent,
                          const RulePortForwardingEntryJsonObject &conf);
  virtual ~RulePortForwardingEntry();

  static void create(RulePortForwarding &parent, const uint32_t &id,
                     const RulePortForwardingEntryJsonObject &conf);
  static std::shared_ptr<RulePortForwardingEntry> getEntry(
      RulePortForwarding &parent, const uint32_t &id);
  static void removeEntry(RulePortForwarding &parent, const uint32_t &id);
  static std::vector<std::shared_ptr<RulePortForwardingEntry>> get(
      RulePortForwarding &parent);
  static void remove(RulePortForwarding &parent);
  nlohmann::fifo_map<std::string, std::string> getKeys();
  std::shared_ptr<spdlog::logger> logger();
  void update(const RulePortForwardingEntryJsonObject &conf) override;
  RulePortForwardingEntryJsonObject toJsonObject() override;

  /// <summary>
  /// Rule identifier
  /// </summary>
  uint32_t getId() override;

  /// <summary>
  /// External destination IP address
  /// </summary>
  std::string getExternalIp() override;
  void setExternalIp(const std::string &value) override;

  /// <summary>
  /// External destination L4 port
  /// </summary>
  uint16_t getExternalPort() override;
  void setExternalPort(const uint16_t &value) override;

  /// <summary>
  /// L4 protocol (TCP, UDP, ALL)
  /// </summary>
  std::string getProto() override;
  void setProto(const std::string &value) override;

  /// <summary>
  /// Internal destination IP address
  /// </summary>
  std::string getInternalIp() override;
  void setInternalIp(const std::string &value) override;

  /// <summary>
  /// Internal destination L4 port
  /// </summary>
  uint16_t getInternalPort() override;
  void setInternalPort(const uint16_t &value) override;

  void injectToDatapath();
  void removeFromDatapath();

  enum ProtoEnum { TCP, UDP, ALL };

 private:
  RulePortForwarding &parent_;
  uint32_t id;
  uint32_t internalIp;
  uint32_t externalIp;
  uint16_t internalPort;
  uint16_t externalPort;
  ProtoEnum proto;
};
