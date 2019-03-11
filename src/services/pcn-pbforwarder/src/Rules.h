/*
 * Copyright 2017 The Polycube Authors
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

#include "../interface/RulesInterface.h"
#include "polycube/services/utils.h"

#include <spdlog/spdlog.h>

#define SET_BIT(word, n) (word = word |= (1ULL << BIT_OFFSET(n)))
#define IS_SET(word, n) ((word & (1ULL << BIT_OFFSET(n))) != 0)
#define BITS_PER_WORD (sizeof(uint32_t) * 8)
#define BIT_OFFSET(b) ((b) % BITS_PER_WORD)

class Pbforwarder;

using namespace io::swagger::server::model;

/* definitions copied from dataplane */
struct rule {
  uint64_t srcMac;
  uint64_t dstMac;
  uint16_t vlanid;
  uint32_t srcIp;
  uint32_t dstIp;
  uint16_t lvl4_proto;
  uint16_t src_port;
  uint16_t dst_port;
  uint16_t inport;
  uint16_t action;
  uint16_t outport;
  uint32_t flags;  // Bitmap. The order is the same of the struct
};

class Rules : public RulesInterface {
 public:
  Rules(Pbforwarder &parent, const RulesJsonObject &conf);
  virtual ~Rules();

  static void create(Pbforwarder &parent, const uint32_t &id,
                     const RulesJsonObject &conf);
  static std::shared_ptr<Rules> getEntry(Pbforwarder &parent,
                                         const uint32_t &id);
  static void removeEntry(Pbforwarder &parent, const uint32_t &id);
  static std::vector<std::shared_ptr<Rules>> get(Pbforwarder &parent);
  static void remove(Pbforwarder &parent);
  nlohmann::fifo_map<std::string, std::string> getKeys();
  std::shared_ptr<spdlog::logger> logger();
  void update(const RulesJsonObject &conf) override;
  RulesJsonObject toJsonObject() override;

  /// <summary>
  /// Destination MAC Address in the form AA:BB:CC:DD:EE:FF
  /// </summary>
  std::string getDstMac() override;
  void setDstMac(const std::string &value) override;

  /// <summary>
  /// Source L4 Port
  /// </summary>
  uint16_t getSrcPort() override;
  void setSrcPort(const uint16_t &value) override;

  /// <summary>
  /// VLAN Identifier
  /// </summary>
  uint32_t getVlan() override;
  void setVlan(const uint32_t &value) override;

  /// <summary>
  /// Output port (used only when action is FORWARD)
  /// </summary>
  std::string getOutPort() override;
  void setOutPort(const std::string &value) override;

  /// <summary>
  /// Rule Identifier
  /// </summary>
  uint32_t getId() override;

  /// <summary>
  /// Source IP Address in the form AA.BB.CC.DD
  /// </summary>
  std::string getSrcIp() override;
  void setSrcIp(const std::string &value) override;

  /// <summary>
  /// Destination L4 Port
  /// </summary>
  uint16_t getDstPort() override;
  void setDstPort(const uint16_t &value) override;

  /// <summary>
  /// Action associated to the rule(i.e., DROP, SLOWPATH, or FORWARD; default:
  /// DROP)
  /// </summary>
  RulesActionEnum getAction() override;
  void setAction(const RulesActionEnum &value) override;

  /// <summary>
  /// Destination IP Address in the form AA.BB.CC.DD
  /// </summary>
  std::string getDstIp() override;
  void setDstIp(const std::string &value) override;

  /// <summary>
  /// Level 4 Protocol (i.e. UDP, TCP)
  /// </summary>
  RulesL4ProtoEnum getL4Proto() override;
  void setL4Proto(const RulesL4ProtoEnum &value) override;

  /// <summary>
  /// Source MAC Address in the form AA:BB:CC:DD:EE:FF
  /// </summary>
  std::string getSrcMac() override;
  void setSrcMac(const std::string &value) override;

  /// <summary>
  /// Ingress port
  /// </summary>
  std::string getInPort() override;
  void setInPort(const std::string &value) override;

  rule to_rule();

 private:
  Pbforwarder &parent_;

  bool updating = false;

  uint32_t flags = 0;

  std::string dstMac = "00:00:00:00:00:00";
  std::string srcMac = "00:00:00:00:00:00";

  std::string srcIp = "0.0.0.0";
  std::string dstIp = "0.0.0.0";

  uint16_t srcPort = 0;
  uint16_t dstPort = 0;
  RulesL4ProtoEnum L4Proto = RulesL4ProtoEnum::TCP;

  std::string outPort = "-1";
  std::string inPort = "0";
  RulesActionEnum action = RulesActionEnum::DROP;

  uint16_t vlan = 0;
  uint32_t id;

  static uint16_t rulesL4ProtoEnum_to_int(RulesL4ProtoEnum proto);
  static uint16_t rulesActionEnum_to_int(RulesActionEnum action);

  void updateRuleMapAndReload();
};
