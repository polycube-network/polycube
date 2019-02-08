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

#include "../interface/BackendInterface.h"

#include "polycube/services/cube.h"
#include "polycube/services/port.h"
#include "polycube/services/utils.h"

#include <spdlog/spdlog.h>

#include "BackendPool.h"

class Lbdsr;

using namespace io::swagger::server::model;
using namespace polycube::service;

class Backend : public BackendInterface {
  friend class BackendPool;

 public:
  Backend(Lbdsr &parent, const BackendJsonObject &conf);
  Backend(Lbdsr &parent);
  virtual ~Backend();

  static void create(Lbdsr &parent, const BackendJsonObject &conf);
  static std::shared_ptr<Backend> getEntry(Lbdsr &parent);
  static void removeEntry(Lbdsr &parent);
  nlohmann::fifo_map<std::string, std::string> getKeys();
  std::shared_ptr<spdlog::logger> logger();
  void update(const BackendJsonObject &conf) override;
  BackendJsonObject toJsonObject() override;

  /// <summary>
  /// pool of backend servers serving requests
  /// </summary>
  std::shared_ptr<BackendPool> getPool(const uint32_t &id) override;
  std::vector<std::shared_ptr<BackendPool>> getPoolList() override;
  void addPool(const uint32_t &id, const BackendPoolJsonObject &conf) override;
  void addPoolList(const std::vector<BackendPoolJsonObject> &conf) override;
  void replacePool(const uint32_t &id,
                   const BackendPoolJsonObject &conf) override;
  void delPool(const uint32_t &id) override;
  void delPoolList() override;

 private:
  Lbdsr &parent_;
  std::unordered_map<uint32_t, std::string> pools_;
};
