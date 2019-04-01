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

#include "../interface/NattingTableInterface.h"

#include <spdlog/spdlog.h>

class Nat;

using namespace io::swagger::server::model;

/* definitions copied from datapath */
struct st_k {
  uint32_t src_ip;
  uint32_t dst_ip;
  uint16_t src_port;
  uint16_t dst_port;
  uint8_t proto;
} __attribute__((packed));

struct st_v {
  uint32_t new_ip;
  uint16_t new_port;
  uint8_t originating_rule_type;
} __attribute__((packed));

class NattingTable : public NattingTableInterface {
 public:
  NattingTable(Nat &parent, const NattingTableJsonObject &conf);
  NattingTable(Nat &parent, const std::string srcIp, const std::string dstIp,
               const uint16_t srcPort, const uint16_t dstPort,
               const uint8_t proto, const std::string newIp,
               const uint16_t newPort, const uint8_t originatingRule);
  virtual ~NattingTable();

  static void create(Nat &parent, const std::string &internalSrc,
                     const std::string &internalDst,
                     const uint16_t &internalSport,
                     const uint16_t &internalDport, const std::string &proto,
                     const NattingTableJsonObject &conf);
  static std::shared_ptr<NattingTable> getEntry(Nat &parent,
                                                const std::string &internalSrc,
                                                const std::string &internalDst,
                                                const uint16_t &internalSport,
                                                const uint16_t &internalDport,
                                                const std::string &proto);
  static void removeEntry(Nat &parent, const std::string &internalSrc,
                          const std::string &internalDst,
                          const uint16_t &internalSport,
                          const uint16_t &internalDport,
                          const std::string &proto);
  static std::vector<std::shared_ptr<NattingTable>> get(Nat &parent);
  static void remove(Nat &parent);
  std::shared_ptr<spdlog::logger> logger();
  void update(const NattingTableJsonObject &conf) override;
  NattingTableJsonObject toJsonObject() override;

  /// <summary>
  /// Source IP address
  /// </summary>
  std::string getInternalSrc() override;

  /// <summary>
  /// Destination IP address
  /// </summary>
  std::string getInternalDst() override;

  /// <summary>
  /// Source L4 port number
  /// </summary>
  uint16_t getInternalSport() override;

  /// <summary>
  /// Destination L4 port number
  /// </summary>
  uint16_t getInternalDport() override;

  /// <summary>
  /// L4 protocol
  /// </summary>
  std::string getProto() override;

  /// <summary>
  /// The set of rules that created this mapping
  /// </summary>
  NattingTableOriginatingRuleEnum getOriginatingRule() override;
  void setOriginatingRule(
      const NattingTableOriginatingRuleEnum &value) override;

  /// <summary>
  /// Translated IP address
  /// </summary>
  std::string getExternalIp() override;
  void setExternalIp(const std::string &value) override;

  /// <summary>
  /// Translated L4 port number
  /// </summary>
  uint16_t getExternalPort() override;
  void setExternalPort(const uint16_t &value) override;

 private:
  Nat &parent_;
  std::string srcIp;
  std::string dstIp;
  uint16_t srcPort;
  uint16_t dstPort;
  std::string proto;
  std::string newIp;
  uint16_t newPort;
  NattingTableOriginatingRuleEnum originatingRule;
};
