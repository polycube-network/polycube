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

#include <spdlog/spdlog.h>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <memory>

#include "base/ChainBase.h"

#include "ChainRule.h"
#include "ChainStats.h"
#include "Firewall.h"

class Firewall;
class ChainRule;

using namespace polycube::service::model;

class Chain : public ChainBase {
  friend class ChainRule;
  friend class ChainStats;

 public:
  Chain(Firewall &parent, const ChainJsonObject &conf);
  virtual ~Chain();

  void update(const ChainJsonObject &conf) override;

  /// <summary>
  /// Default action if no rule matches in the ingress chain. Default is DROP.
  /// </summary>
  ActionEnum getDefault() override;
  void setDefault(const ActionEnum &value) override;

  /// <summary>
  ///
  /// </summary>
  std::shared_ptr<ChainStats> getStats(const uint32_t &id) override;
  std::vector<std::shared_ptr<ChainStats>> getStatsList() override;
  void addStats(const uint32_t &id, const ChainStatsJsonObject &conf) override;
  void addStatsList(const std::vector<ChainStatsJsonObject> &conf) override;
  void replaceStats(const uint32_t &id,
                    const ChainStatsJsonObject &conf) override;
  void delStats(const uint32_t &id) override;
  void delStatsList() override;

  /// <summary>
  /// Chain in which the rule will be inserted. Default: INGRESS.
  /// </summary>
  ChainNameEnum getName() override;

  /// <summary>
  ///
  /// </summary>
  std::shared_ptr<ChainRule> getRule(const uint32_t &id) override;
  std::vector<std::shared_ptr<ChainRule>> getRuleList() override;
  void addRule(const uint32_t &id, const ChainRuleJsonObject &conf) override;
  void addRuleList(const std::vector<ChainRuleJsonObject> &conf) override;
  void replaceRule(const uint32_t &id,
                   const ChainRuleJsonObject &conf) override;
  void delRule(const uint32_t &id) override;
  void delRuleList() override;

  ChainAppendOutputJsonObject append(ChainAppendInputJsonObject input) override;
  ChainInsertOutputJsonObject insert(ChainInsertInputJsonObject input) override;
  void deletes(ChainDeleteInputJsonObject input) override;
  ChainResetCountersOutputJsonObject resetCounters() override;
  ChainApplyRulesOutputJsonObject applyRules() override;

  uint32_t getNrRules();

 private:
  ActionEnum defaultAction = ActionEnum::DROP;
  ChainNameEnum name;
  std::vector<std::shared_ptr<ChainRule>> rules_;
  std::vector<std::shared_ptr<ChainStats>> counters_;

  // This keeps track of the chain currently used, primary or secondary.
  uint8_t chainNumber = 0;

  void updateChain();

  /*Like getRuleList, but without the last (fake) default rule*/
  std::vector<std::shared_ptr<ChainRule>> getRealRuleList();

  static bool ipFromRulesToMap(
          const uint8_t &type, std::map<struct IpAddr, std::vector<uint64_t>> &ips,
          const std::vector<std::shared_ptr<ChainRule>> &rules);

  static bool transportProtoFromRulesToMap(
          std::map<int, std::vector<uint64_t>> &protocols,
          const std::vector<std::shared_ptr<ChainRule>> &rules);

  static bool portFromRulesToMap(
          const uint8_t &type, std::map<uint16_t, std::vector<uint64_t>> &ports,
          const std::vector<std::shared_ptr<ChainRule>> &rules);

  static bool flagsFromRulesToMap(
          std::vector<std::vector<uint64_t>> &flags,
          const std::vector<std::shared_ptr<ChainRule>> &rules);

  static bool conntrackFromRulesToMap(
          std::map<uint8_t, std::vector<uint64_t>> &statusMap,
          const std::vector<std::shared_ptr<ChainRule>> &rules);

  static void horusFromRulesToMap(
      std::map<struct HorusRule, struct HorusValue> &horus,
      const std::vector<std::shared_ptr<ChainRule>> &rules);

  static bool fromRuleToHorusKeyValue(std::shared_ptr<ChainRule> rule,
                                      struct HorusRule &key,
                                      struct HorusValue &value);
};
