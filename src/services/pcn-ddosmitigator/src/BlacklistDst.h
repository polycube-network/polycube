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

#include "./interface/BlacklistDstInterface.h"

#include <spdlog/spdlog.h>
#include "polycube/services/cube.h"
#include "polycube/services/utils.h"

class Ddosmitigator;

using namespace io::swagger::server::model;

class BlacklistDst : public BlacklistDstInterface {
 public:
  BlacklistDst(Ddosmitigator &parent, const BlacklistDstJsonObject &conf);
  virtual ~BlacklistDst();

  std::shared_ptr<spdlog::logger> logger();
  void update(const BlacklistDstJsonObject &conf) override;
  BlacklistDstJsonObject toJsonObject() override;

  /// <summary>
  /// Destination IP Address
  /// </summary>
  std::string getIp() override;

  /// <summary>
  /// Dropped Packets
  /// </summary>
  uint64_t getDropPkts() override;

 private:
  Ddosmitigator &parent_;
  std::string ip_;
};
