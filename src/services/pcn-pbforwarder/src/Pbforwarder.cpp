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

#include "Pbforwarder.h"
#include "Pbforwarder_dp_action.h"
#include "Pbforwarder_dp_matching.h"
#include "Pbforwarder_dp_parsing.h"

Pbforwarder::Pbforwarder(const std::string name,
                         const PbforwarderJsonObject &conf)
    : Cube(conf.getBase(), generate_code_vector(), {}) {
  logger()->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [Pbforwarder] [%n] [%l] %v");
  logger()->info("Creating Pbforwarder instance");

  addRulesList(conf.getRules());

  addPortsList(conf.getPorts());
}

Pbforwarder::~Pbforwarder() {
  del_program(2);
  del_program(1);
}

void Pbforwarder::update(const PbforwarderJsonObject &conf) {
  Cube::set_conf(conf.getBase());

  if (conf.rulesIsSet()) {
    for (auto &i : conf.getRules()) {
      auto id = i.getId();
      auto m = getRules(id);
      m->update(i);
    }
  }

  if (conf.portsIsSet()) {
    for (auto &i : conf.getPorts()) {
      auto name = i.getName();
      auto m = getPorts(name);
      m->update(i);
    }
  }
}

PbforwarderJsonObject Pbforwarder::toJsonObject() {
  PbforwarderJsonObject conf;
  conf.setBase(Cube::to_json());

  for (auto &i : getRulesList()) {
    conf.addRules(i->toJsonObject());
  }

  for (auto &i : getPortsList()) {
    conf.addPorts(i->toJsonObject());
  }

  return conf;
}

std::string Pbforwarder::generate_code() {
  throw std::runtime_error("Method not implemented");
}

// overriden method to allow code generation at initialization
std::string Pbforwarder::generate_code_matching(bool bootstrap) {
  std::string init_define;
  if (bootstrap) {
    init_define = std::string("#define RULES 0 \n#define LEVEL 2\n");
  } else {
    init_define = std::string("#define RULES ") + std::to_string(nr_rules) +
                  std::string("\n#define LEVEL ") + std::to_string(match_level);
  }

  return init_define + pbforwarder_code_matching;
}

std::string Pbforwarder::generate_code_parsing(bool bootstrap) {
  std::string init_define;
  if (bootstrap) {
    init_define = std::string("#define RULES 0 \n#define LEVEL 2\n");
  } else {
    init_define = std::string("#define RULES ") + std::to_string(nr_rules) +
                  std::string("\n#define LEVEL ") + std::to_string(match_level);
  }

  return init_define + pbforwarder_code_parsing;
}

std::vector<std::string> Pbforwarder::generate_code_vector() {
  std::vector<std::string> codes;
  codes.push_back(generate_code_parsing(true));
  codes.push_back(generate_code_matching(true));
  codes.push_back(pbforwarder_code_action);
  return codes;
}

void Pbforwarder::packet_in(Ports &port,
                            polycube::service::PacketInMetadata &md,
                            const std::vector<uint8_t> &packet) {
  logger()->info("Packet received from port {0}", port.name());
}
