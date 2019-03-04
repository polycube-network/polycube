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

#include "../interface/SimpleforwarderInterface.h"

#include "polycube/services/cube.h"
#include "polycube/services/fifo_map.hpp"
#include "polycube/services/port.h"
#include "polycube/services/utils.h"

#include <spdlog/spdlog.h>

#include "Actions.h"
#include "Ports.h"

using namespace io::swagger::server::model;

class Simpleforwarder : public polycube::service::Cube<Ports>,
                        public SimpleforwarderInterface {
  friend class Ports;

 public:
  Simpleforwarder(const std::string name, const SimpleforwarderJsonObject &conf);
  virtual ~Simpleforwarder();
  std::string generate_code();
  std::vector<std::string> generate_code_vector();
  void packet_in(Ports &port, polycube::service::PacketInMetadata &md,
                 const std::vector<uint8_t> &packet) override;

  void update(const SimpleforwarderJsonObject &conf) override;
  SimpleforwarderJsonObject toJsonObject() override;

  /// <summary>
  /// Entry of the ports table
  /// </summary>
  std::shared_ptr<Ports> getPorts(const std::string &name) override;
  std::vector<std::shared_ptr<Ports>> getPortsList() override;
  void addPorts(const std::string &name, const PortsJsonObject &conf) override;
  void addPortsList(const std::vector<PortsJsonObject> &conf) override;
  void replacePorts(const std::string &name,
                    const PortsJsonObject &conf) override;
  void delPorts(const std::string &name) override;
  void delPortsList() override;

  /// <summary>
  /// Entry of the Actions table
  /// </summary>
  std::shared_ptr<Actions> getActions(const std::string &inport) override;
  std::vector<std::shared_ptr<Actions>> getActionsList() override;
  void addActions(const std::string &inport,
                  const ActionsJsonObject &conf) override;
  void addActionsList(const std::vector<ActionsJsonObject> &conf) override;
  void replaceActions(const std::string &inport,
                      const ActionsJsonObject &conf) override;
  void delActions(const std::string &inport) override;
  void delActionsList() override;
};
