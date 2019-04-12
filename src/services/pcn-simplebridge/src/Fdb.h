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

#include "../base/FdbBase.h"

#include "FdbEntry.h"

class Simplebridge;

using namespace polycube::service::model;

class Fdb : public FdbBase {
  friend class FdbEntry;

 public:
  Fdb(Simplebridge &parent, const FdbJsonObject &conf);
  Fdb(Simplebridge &parent);
  virtual ~Fdb();

  /// <summary>
  /// Aging time of the filtering database (in seconds)
  /// </summary>
  uint32_t getAgingTime() override;
  void setAgingTime(const uint32_t &value) override;

  /// <summary>
  /// Entry associated with the filtering database
  /// </summary>
  std::shared_ptr<FdbEntry> getEntry(const std::string &address) override;
  std::vector<std::shared_ptr<FdbEntry>> getEntryList() override;
  void addEntry(const std::string &address, const FdbEntryJsonObject &conf) override;
  void addEntryList(const std::vector<FdbEntryJsonObject> &conf) override;
  void replaceEntry(const std::string &address, const FdbEntryJsonObject &conf) override;
  void delEntry(const std::string &address) override;
  void delEntryList() override;

  FdbFlushOutputJsonObject flush() override;

 private:
  // Default value for agingTime
  uint32_t agingTime_ = 300;
};
