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

#include "../interface/RuleDnatEntryInterface.h"

#include <spdlog/spdlog.h>

class RuleDnat;

using namespace io::swagger::server::model;

class RuleDnatEntry : public RuleDnatEntryInterface {
 public:
  RuleDnatEntry(RuleDnat &parent, const RuleDnatEntryJsonObject &conf);
  virtual ~RuleDnatEntry();

  static void create(RuleDnat &parent, const uint32_t &id,
                     const RuleDnatEntryJsonObject &conf);
  static std::shared_ptr<RuleDnatEntry> getEntry(RuleDnat &parent,
                                                 const uint32_t &id);
  static void removeEntry(RuleDnat &parent, const uint32_t &id);
  static std::vector<std::shared_ptr<RuleDnatEntry>> get(RuleDnat &parent);
  static void remove(RuleDnat &parent);
  std::shared_ptr<spdlog::logger> logger();
  void update(const RuleDnatEntryJsonObject &conf) override;
  RuleDnatEntryJsonObject toJsonObject() override;

  /// <summary>
  /// Rule identifier
  /// </summary>
  uint32_t getId() override;

  /// <summary>
  /// External destination IP address
  /// </summary>
  std::string getExternalIp() override;
  void setExternalIp(const std::string &value) override;

  /// <summary>
  /// Internal destination IP address
  /// </summary>
  std::string getInternalIp() override;
  void setInternalIp(const std::string &value) override;

  void injectToDatapath();
  void removeFromDatapath();

 private:
  RuleDnat &parent_;
  uint32_t id;
  uint32_t internalIp;
  uint32_t externalIp;
};
