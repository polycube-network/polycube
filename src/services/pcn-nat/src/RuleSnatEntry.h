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

#include "../interface/RuleSnatEntryInterface.h"
#include "IpAddr.h"

#include <spdlog/spdlog.h>

class RuleSnat;

using namespace io::swagger::server::model;

class RuleSnatEntry : public RuleSnatEntryInterface {
 public:
  RuleSnatEntry(RuleSnat &parent, const RuleSnatEntryJsonObject &conf);
  virtual ~RuleSnatEntry();

  static void create(RuleSnat &parent, const uint32_t &id,
                     const RuleSnatEntryJsonObject &conf);
  static std::shared_ptr<RuleSnatEntry> getEntry(RuleSnat &parent,
                                                 const uint32_t &id);
  static void removeEntry(RuleSnat &parent, const uint32_t &id);
  static std::vector<std::shared_ptr<RuleSnatEntry>> get(RuleSnat &parent);
  static void remove(RuleSnat &parent);
  nlohmann::fifo_map<std::string, std::string> getKeys();
  std::shared_ptr<spdlog::logger> logger();
  void update(const RuleSnatEntryJsonObject &conf) override;
  RuleSnatEntryJsonObject toJsonObject() override;

  /// <summary>
  /// Rule identifier
  /// </summary>
  uint32_t getId() override;

  /// <summary>
  /// Internal IP address (or subnet)
  /// </summary>
  std::string getInternalNet() override;
  void setInternalNet(const std::string &value) override;

  /// <summary>
  /// Natted source IP address
  /// </summary>
  std::string getExternalIp() override;
  void setExternalIp(const std::string &value) override;

  void injectToDatapath();
  void removeFromDatapath();

 private:
  RuleSnat &parent_;

  uint32_t id;
  uint32_t internalIp;
  uint8_t internalNetmask;
  uint32_t externalIp;
};
