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

#include "../interface/ChainStatsInterface.h"

#include <spdlog/spdlog.h>

class Chain;

using namespace io::swagger::server::model;

class ChainStats : public ChainStatsInterface {
  friend class ChainRule;

 public:
  ChainStats(Chain &parent, const ChainStatsJsonObject &conf);
  virtual ~ChainStats();

  static void create(Chain &parent, const uint32_t &id,
                     const ChainStatsJsonObject &conf);
  static std::shared_ptr<ChainStats> getEntry(Chain &parent,
                                              const uint32_t &id);
  static void removeEntry(Chain &parent, const uint32_t &id);
  static std::vector<std::shared_ptr<ChainStats>> get(Chain &parent);
  static void remove(Chain &parent);
  nlohmann::fifo_map<std::string, std::string> getKeys();
  std::shared_ptr<spdlog::logger> logger();
  void update(const ChainStatsJsonObject &conf) override;
  ChainStatsJsonObject toJsonObject() override;
  static std::shared_ptr<ChainStats> getDefaultActionCounters(Chain &parent);

  /// <summary>
  /// Number of packets matching the rule
  /// </summary>
  uint64_t getPkts() override;

  /// <summary>
  /// Number of bytes matching the rule
  /// </summary>
  uint64_t getBytes() override;

  /// <summary>
  /// Rule Identifier
  /// </summary>
  uint32_t getId() override;

  /// <summary>
  /// Description
  /// </summary>
  std::string getDescription() override;

 private:
  Chain &parent_;
  uint32_t rule;

  ChainStatsJsonObject counter;
  static void fetchCounters(const Chain &parent, const uint32_t &id,
                            uint64_t &pkts, uint64_t &bytes);
};
