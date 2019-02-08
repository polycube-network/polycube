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

#include "../interface/RuleSnatInterface.h"

#include <spdlog/spdlog.h>

#include "RuleSnatEntry.h"

class Rule;

using namespace io::swagger::server::model;

class RuleSnat : public RuleSnatInterface {
  friend class RuleSnatEntry;

 public:
  RuleSnat(Rule &parent);
  RuleSnat(Rule &parent, const RuleSnatJsonObject &conf);
  virtual ~RuleSnat();

  static void create(Rule &parent, const RuleSnatJsonObject &conf);
  static std::shared_ptr<RuleSnat> getEntry(Rule &parent);
  static void removeEntry(Rule &parent);
  std::shared_ptr<spdlog::logger> logger();
  void update(const RuleSnatJsonObject &conf) override;
  RuleSnatJsonObject toJsonObject() override;

  /// <summary>
  /// List of Source NAT rules
  /// </summary>
  std::shared_ptr<RuleSnatEntry> getEntry(const uint32_t &id) override;
  std::vector<std::shared_ptr<RuleSnatEntry>> getEntryList() override;
  void addEntry(const uint32_t &id,
                const RuleSnatEntryJsonObject &conf) override;
  void addEntryList(const std::vector<RuleSnatEntryJsonObject> &conf) override;
  void replaceEntry(const uint32_t &id,
                    const RuleSnatEntryJsonObject &conf) override;
  void delEntry(const uint32_t &id) override;
  void delEntryList() override;

  RuleSnatAppendOutputJsonObject append(
      RuleSnatAppendInputJsonObject input) override;

 private:
  Rule &parent_;
  std::vector<std::shared_ptr<RuleSnatEntry>> rules_;
};
