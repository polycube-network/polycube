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

#include "../interface/StatsInterface.h"
#include "polycube/services/utils.h"

#include <thread>

#include <spdlog/spdlog.h>
class Ddosmitigator;

using namespace io::swagger::server::model;

class Stats : public StatsInterface {
 public:
  Stats(Ddosmitigator &parent, const StatsJsonObject &conf);
  virtual ~Stats();

  std::shared_ptr<spdlog::logger> logger();
  void update(const StatsJsonObject &conf) override;
  StatsJsonObject toJsonObject() override;

  /// <summary>
  /// Dropped Packets/s
  /// </summary>
  uint64_t getPps() override;

  /// <summary>
  /// Total Dropped Packets
  /// </summary>
  uint64_t getPkts() override;

 private:
  Ddosmitigator &parent_;
};
