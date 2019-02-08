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

#include "../interface/RulePortForwardingInterface.h"

#include <spdlog/spdlog.h>

#include "RulePortForwardingEntry.h"

class Rule;

using namespace io::swagger::server::model;

class RulePortForwarding : public RulePortForwardingInterface {
  friend class RulePortForwardingEntry;

 public:
  RulePortForwarding(Rule &parent);
  RulePortForwarding(Rule &parent, const RulePortForwardingJsonObject &conf);
  virtual ~RulePortForwarding();

  static void create(Rule &parent, const RulePortForwardingJsonObject &conf);
  static std::shared_ptr<RulePortForwarding> getEntry(Rule &parent);
  static void removeEntry(Rule &parent);
  std::shared_ptr<spdlog::logger> logger();
  void update(const RulePortForwardingJsonObject &conf) override;
  RulePortForwardingJsonObject toJsonObject() override;

  /// <summary>
  /// List of port forwarding rules
  /// </summary>
  std::shared_ptr<RulePortForwardingEntry> getEntry(
      const uint32_t &id) override;
  std::vector<std::shared_ptr<RulePortForwardingEntry>> getEntryList() override;
  void addEntry(const uint32_t &id,
                const RulePortForwardingEntryJsonObject &conf) override;
  void addEntryList(
      const std::vector<RulePortForwardingEntryJsonObject> &conf) override;
  void replaceEntry(const uint32_t &id,
                    const RulePortForwardingEntryJsonObject &conf) override;
  void delEntry(const uint32_t &id) override;
  void delEntryList() override;

  RulePortForwardingAppendOutputJsonObject append(
      RulePortForwardingAppendInputJsonObject input) override;

 private:
  Rule &parent_;
  std::vector<std::shared_ptr<RulePortForwardingEntry>> rules_;
};
