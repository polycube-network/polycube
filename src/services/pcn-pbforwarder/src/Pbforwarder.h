/*
 * Copyright 2017 The Polycube Authors
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

#include "../interface/PbforwarderInterface.h"

#include "polycube/services/cube.h"
#include "polycube/services/port.h"
#include "polycube/services/utils.h"

#include <spdlog/spdlog.h>

#include "Ports.h"
#include "Rules.h"

using namespace io::swagger::server::model;
using polycube::service::CubeType;

class Pbforwarder : public polycube::service::Cube<Ports>,
                    public PbforwarderInterface {
  friend class Ports;
  friend class Rules;

 public:
  Pbforwarder(const std::string name, const PbforwarderJsonObject &conf,
              CubeType type = CubeType::TC);
  virtual ~Pbforwarder() override;
  std::string generate_code();
  std::vector<std::string> generate_code_vector();
  void packet_in(Ports &port,
                 polycube::service::PacketInMetadata &md,
                 const std::vector<uint8_t> &packet) override;

  void update(const PbforwarderJsonObject &conf) override;
  PbforwarderJsonObject toJsonObject() override;

  /// <summary>
  /// UUID of the Cube
  /// </summary>
  std::string getUuid() override;

  /// <summary>
  /// Defines the logging level of a service instance, from none (OFF) to the
  /// most verbose (TRACE). Default: OFF
  /// </summary>
  PbforwarderLoglevelEnum getLoglevel() override;
  void setLoglevel(const PbforwarderLoglevelEnum &value) override;

  /// <summary>
  /// Rule that contains all possible matches and the action for a packet
  /// </summary>
  std::shared_ptr<Rules> getRules(const uint32_t &id) override;
  std::vector<std::shared_ptr<Rules>> getRulesList() override;
  void addRules(const uint32_t &id, const RulesJsonObject &conf) override;
  void addRulesList(const std::vector<RulesJsonObject> &conf) override;
  void replaceRules(const uint32_t &id, const RulesJsonObject &conf) override;
  void delRules(const uint32_t &id) override;
  void delRulesList() override;

  /// <summary>
  /// Type of the Cube (TC, XDP_SKB, XDP_DRV)
  /// </summary>
  CubeType getType() override;

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
  /// Name of the pbforwarder service
  /// </summary>
  std::string getName() override;

  std::string generate_code_matching(bool bootstrap = false);
  std::string generate_code_parsing(bool bootstrap = false);

 private:
  std::map<uint32_t, Rules> rules_;

  int match_level = 2;
  int nr_rules = 0;
};
