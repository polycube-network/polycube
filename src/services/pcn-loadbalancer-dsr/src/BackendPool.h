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

#include "../interface/BackendPoolInterface.h"

#include "polycube/services/cube.h"
#include "polycube/services/port.h"
#include "polycube/services/utils.h"

#include <spdlog/spdlog.h>

class Backend;

using namespace io::swagger::server::model;
using namespace polycube::service;

class BackendPool : public BackendPoolInterface {
 public:
  BackendPool(Backend &parent, const BackendPoolJsonObject &conf);
  virtual ~BackendPool();

  static void create(Backend &parent, const uint32_t &id,
                     const BackendPoolJsonObject &conf);
  static std::shared_ptr<BackendPool> getEntry(Backend &parent,
                                               const uint32_t &id);
  static void removeEntry(Backend &parent, const uint32_t &id);
  static std::vector<std::shared_ptr<BackendPool>> get(Backend &parent);
  static void remove(Backend &parent);
  nlohmann::fifo_map<std::string, std::string> getKeys();
  std::shared_ptr<spdlog::logger> logger();
  void update(const BackendPoolJsonObject &conf) override;
  BackendPoolJsonObject toJsonObject() override;

  /// <summary>
  /// MAC address of the backend server of the pool
  /// </summary>
  std::string getMac() override;

  /// <summary>
  /// id
  /// </summary>
  uint32_t getId() override;

 private:
  Backend &parent_;
  uint32_t id_;
  std::string mac_;
};
