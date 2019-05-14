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

#include "defines.h"

#include "base/ChainRuleBase.h"

class Chain;

using namespace polycube::service::model;

class ChainRule : public ChainRuleBase {
  friend class ChainStats;
  friend class Chain;

 public:
  ChainRule(Chain &parent, const ChainRuleJsonObject &conf);
  virtual ~ChainRule();

  void update(const ChainRuleJsonObject &conf) override;
  ChainRuleJsonObject toJsonObject() override;

  bool equal(ChainRule &cmp);

  /// <summary>
  /// Description of the rule.
  /// </summary>
  std::string getDescription() override;

  /// <summary>
  /// Source IP Address.
  /// </summary>
  std::string getSrc() override;

  /// <summary>
  /// Destination IP Address.
  /// </summary>
  std::string getDst() override;

  /// <summary>
  /// Destination L4 Port
  /// </summary>
  uint16_t getDport() override;

  /// <summary>
  /// TCP flags. Allowed values: SYN, FIN, ACK, RST, PSH, URG, CWR, ECE. ! means
  /// set to 0.
  /// </summary>
  std::string getTcpflags() override;

  /// <summary>
  /// Level 4 Protocol.
  /// </summary>
  std::string getL4proto() override;

  /// <summary>
  /// Connection status (NEW, ESTABLISHED, RELATED, INVALID)
  /// </summary>
  ConntrackstatusEnum getConntrack() override;

  /// <summary>
  /// Action if the rule matches. Default is DROP.
  /// </summary>
  ActionEnum getAction() override;

  /// <summary>
  /// Source L4 Port
  /// </summary>
  uint16_t getSport() override;

  /// <summary>
  /// Rule Identifier
  /// </summary>
  uint32_t getId() override;

  static int protocol_from_string_to_int(const std::string &proto);
  static std::string protocol_from_int_to_string(const int &proto);
  static void flags_from_string_to_masks(const std::string &flags,
                                         uint8_t &flagsSet,
                                         uint8_t &flagsNotSet);
  static void flags_from_masks_to_string(std::string &flags,
                                         const uint8_t &flagsSet,
                                         const uint8_t &flagsNotSet);

  static int ActionEnum_to_int(const ActionEnum &action);
  static int ChainRuleConntrackEnum_to_int(const ConntrackstatusEnum &status);

 private:
  uint32_t id;

  ConntrackstatusEnum conntrack;
  bool conntrackIsSet = false;

  struct IpAddr ipSrc;
  bool ipSrcIsSet = false;

  struct IpAddr ipDst;
  bool ipDstIsSet = false;

  uint16_t srcPort;
  bool srcPortIsSet = false;

  uint16_t dstPort;
  bool dstPortIsSet = false;

  int l4Proto;
  bool l4ProtoIsSet = false;

  uint8_t flagsSet = 0, flagsNotSet = 0;
  bool tcpFlagsIsSet = false;

  ActionEnum action;
  bool actionIsSet = false;

  std::string description;
  bool descriptionIsSet = false;
};
