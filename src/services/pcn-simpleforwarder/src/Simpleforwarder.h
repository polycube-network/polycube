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

#include "../base/SimpleforwarderBase.h"

#include "Actions.h"
#include "Ports.h"

using namespace polycube::service::model;

class Simpleforwarder : public SimpleforwarderBase {
  friend class Ports;

 public:
  Simpleforwarder(const std::string name,
                  const SimpleforwarderJsonObject &conf);
  virtual ~Simpleforwarder();
  void packet_in(Ports &port, polycube::service::PacketInMetadata &md,
                 const std::vector<uint8_t> &packet) override;

  /// <summary>
  /// Entry of the Actions table
  /// </summary>
  std::shared_ptr<Actions> getActions(const std::string &inport) override;
  std::vector<std::shared_ptr<Actions>> getActionsList() override;
  void addActions(const std::string &inport,
                  const ActionsJsonObject &conf) override;
  void delActions(const std::string &inport) override;
  void delActionsList() override;
};
