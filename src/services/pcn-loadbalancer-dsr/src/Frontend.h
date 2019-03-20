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

#include "../interface/FrontendInterface.h"

#include "polycube/services/cube.h"
#include "polycube/services/port.h"
#include "polycube/services/utils.h"

#include <spdlog/spdlog.h>

class Lbdsr;

using namespace io::swagger::server::model;
using namespace polycube::service;

class Frontend : public FrontendInterface {
 public:
  Frontend(Lbdsr &parent, const FrontendJsonObject &conf);
  Frontend(Lbdsr &parent);
  virtual ~Frontend();

  static void create(Lbdsr &parent, const FrontendJsonObject &conf);
  static std::shared_ptr<Frontend> getEntry(Lbdsr &parent);
  static void removeEntry(Lbdsr &parent);
  std::shared_ptr<spdlog::logger> logger();
  void update(const FrontendJsonObject &conf) override;
  FrontendJsonObject toJsonObject() override;

  /// <summary>
  /// IP address of the loadbalancer frontend
  /// </summary>
  std::string getVip() override;
  void setVip(const std::string &value) override;

  /// <summary>
  /// MAC address of the port
  /// </summary>
  std::string getMac() override;
  void setMac(const std::string &value) override;

 private:
  Lbdsr &parent_;
  std::string vip_ = "10.0.0.100";
  std::string mac_ = "01:01:01:aa:bb:cc";
};
