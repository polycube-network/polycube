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

#include "../interface/RuleDnatInterface.h"

#include <spdlog/spdlog.h>

#include "RuleDnatEntry.h"

class Rule;

using namespace io::swagger::server::model;

class RuleDnat : public RuleDnatInterface {
  friend class RuleDnatEntry;

 public:
  RuleDnat(Rule &parent);
  RuleDnat(Rule &parent, const RuleDnatJsonObject &conf);
  virtual ~RuleDnat();

  std::shared_ptr<spdlog::logger> logger();
  void update(const RuleDnatJsonObject &conf) override;
  RuleDnatJsonObject toJsonObject() override;

  /// <summary>
  /// List of Destination NAT rules
  /// </summary>
  std::shared_ptr<RuleDnatEntry> getEntry(const uint32_t &id) override;
  std::vector<std::shared_ptr<RuleDnatEntry>> getEntryList() override;
  void addEntry(const uint32_t &id,
                const RuleDnatEntryJsonObject &conf) override;
  void addEntryList(const std::vector<RuleDnatEntryJsonObject> &conf) override;
  void replaceEntry(const uint32_t &id,
                    const RuleDnatEntryJsonObject &conf) override;
  void delEntry(const uint32_t &id) override;
  void delEntryList() override;

  RuleDnatAppendOutputJsonObject append(
      RuleDnatAppendInputJsonObject input) override;

 private:
  Rule &parent_;
  std::vector<std::shared_ptr<RuleDnatEntry>> rules_;
};
