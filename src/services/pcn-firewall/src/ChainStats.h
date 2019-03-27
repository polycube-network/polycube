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

#include "../interface/ChainStatsInterface.h"

#include <spdlog/spdlog.h>

class Chain;

using namespace io::swagger::server::model;

class ChainStats : public ChainStatsInterface {
  friend class ChainRule;
  friend class Chain;

 public:
  ChainStats(Chain &parent, const ChainStatsJsonObject &conf);
  virtual ~ChainStats();

  std::shared_ptr<spdlog::logger> logger();
  void update(const ChainStatsJsonObject &conf) override;
  ChainStatsJsonObject toJsonObject() override;

  /// <summary>
  /// Source IP Address.
  /// </summary>
  std::string getSrc() override;

  /// <summary>
  /// Description of the rule.
  /// </summary>
  std::string getDescription() override;

  /// <summary>
  /// Destination IP Address.
  /// </summary>
  std::string getDst() override;

  /// <summary>
  /// Number of bytes matching the rule
  /// </summary>
  uint64_t getBytes() override;

  /// <summary>
  /// Number of packets matching the rule
  /// </summary>
  uint64_t getPkts() override;

  /// <summary>
  /// Action if the rule matches. Default is DROP.
  /// </summary>
  ActionEnum getAction() override;

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
  /// Destination L4 Port
  /// </summary>
  uint16_t getDport() override;

  /// <summary>
  /// Source L4 Port
  /// </summary>
  uint16_t getSport() override;

  /// <summary>
  /// Rule Identifier
  /// </summary>
  uint32_t getId() override;

 private:
  Chain &parent_;
  ChainStatsJsonObject counter;

  static std::shared_ptr<ChainStats> getDefaultActionCounters(Chain &parent);
  static void fetchCounters(const Chain &parent, const uint32_t &id,
                            uint64_t &pkts, uint64_t &bytes);
};
