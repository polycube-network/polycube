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

#include "../interface/ActionsInterface.h"

#include <spdlog/spdlog.h>

class Simpleforwarder;

using namespace io::swagger::server::model;

struct action {
  uint16_t action;  // which action? see above enum
  uint16_t port;    // in case of redirect, to what port?
} __attribute__((packed));

class Actions : public ActionsInterface {
 public:
  Actions(Simpleforwarder &parent, const ActionsJsonObject &conf);
  virtual ~Actions();

  static void create(Simpleforwarder &parent, const std::string &inport,
                     const ActionsJsonObject &conf);
  static std::shared_ptr<Actions> getEntry(Simpleforwarder &parent,
                                           const std::string &inport);
  static void removeEntry(Simpleforwarder &parent, const std::string &inport);
  static std::vector<std::shared_ptr<Actions>> get(Simpleforwarder &parent);
  static void remove(Simpleforwarder &parent);
  std::shared_ptr<spdlog::logger> logger();
  void update(const ActionsJsonObject &conf) override;
  ActionsJsonObject toJsonObject() override;

  /// <summary>
  /// Action associated to the current table entry (i.e., DROP, SLOWPATH, or
  /// FORWARD; default: DROP)
  /// </summary>
  ActionsActionEnum getAction() override;
  void setAction(const ActionsActionEnum &value) override;

  /// <summary>
  /// Output port (used only when action is FORWARD)
  /// </summary>
  std::string getOutport() override;
  void setOutport(const std::string &value) override;

  /// <summary>
  /// Ingress port
  /// </summary>
  std::string getInport() override;

 private:
  Simpleforwarder &parent_;
  std::string inport;

  static ActionsActionEnum actionNumberToEnum(uint16_t action);
  static uint16_t actionEnumToNumber(ActionsActionEnum action);
};
