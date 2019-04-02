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

std::shared_ptr<Ports> Pbforwarder::getPorts(const std::string &name) {
  return get_port(name);
}

std::vector<std::shared_ptr<Ports>> Pbforwarder::getPortsList() {
  return get_ports();
}

void Pbforwarder::addPorts(const std::string &name,
                           const PortsJsonObject &conf) {
  add_port<PortsJsonObject>(name, conf);
}

void Pbforwarder::addPortsList(const std::vector<PortsJsonObject> &conf) {
  for (auto &i : conf) {
    std::string name_ = i.getName();
    addPorts(name_, i);
  }
}

void Pbforwarder::replacePorts(const std::string &name,
                               const PortsJsonObject &conf) {
  delPorts(name);
  std::string name_ = conf.getName();
  addPorts(name_, conf);
}

void Pbforwarder::delPorts(const std::string &name) {
  remove_port(name);
}

void Pbforwarder::delPortsList() {
  auto ports = get_ports();
  for (auto it : ports) {
    delPorts(it->name());
  }
}

std::shared_ptr<Rules> Pbforwarder::getRules(const uint32_t &id) {
  return std::shared_ptr<Rules>(&rules_.at(id), [](Rules *) {});
}

std::vector<std::shared_ptr<Rules>> Pbforwarder::getRulesList() {
  std::vector<std::shared_ptr<Rules>> rules_vect;

  for (auto &it : rules_) {
    rules_vect.push_back(getRules(it.first));
  }

  return rules_vect;
}

void Pbforwarder::addRules(const uint32_t &id, const RulesJsonObject &conf) {
  if (!conf.actionIsSet()) {
    throw std::runtime_error("Action is mandatory\n");
  }

  if (conf.getAction() == RulesActionEnum::FORWARD) {
    if (!conf.outPortIsSet()) {
      throw std::runtime_error("Port is mandatory to FORWARD\n");
    }
  }

  Rules newRule(*this, conf);
  // newRule.id = id;
  newRule.update(conf);
  // TODO: add this to the rule list?
}

void Pbforwarder::addRulesList(const std::vector<RulesJsonObject> &conf) {
  for (auto &i : conf) {
    uint32_t id_ = i.getId();
    addRules(id_, i);
  }
}

void Pbforwarder::replaceRules(const uint32_t &id,
                               const RulesJsonObject &conf) {
  delRules(id);
  uint32_t id_ = conf.getId();
  addRules(id_, conf);
}

void Pbforwarder::delRules(const uint32_t &id) {
  rules_.erase(id);
  auto rules_table = get_hash_table<uint32_t, rule>("rules", 1);
  rules_table.remove(id);
  if (nr_rules == id) {
    nr_rules--;
    reload(generate_code_parsing(), 1);
    reload(generate_code_matching(), 1);
  }
}

void Pbforwarder::delRulesList() {
  rules_.clear();
  auto rules_table = get_hash_table<uint32_t, rule>("rules", 1);
  rules_table.remove_all();

  match_level = 2;
  nr_rules = 0;
  reload(generate_code_parsing(), 1);
  reload(generate_code_matching(), 1);
}