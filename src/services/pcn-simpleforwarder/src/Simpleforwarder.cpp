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

// Modify these methods with your own implementation

#include "Simpleforwarder.h"
#include "Simpleforwarder_dp.h"

Simpleforwarder::Simpleforwarder(const std::string name,
                                 const SimpleforwarderJsonObject &conf)
    : Cube(conf.getBase(), {generate_code()}, {}) {
  logger()->set_pattern(
      "[%Y-%m-%d %H:%M:%S.%e] [Simpleforwarder] [%n] [%l] %v");
  logger()->info("Creating Simpleforwarder instance");

  addActionsList(conf.getActions());

  addPortsList(conf.getPorts());
}

Simpleforwarder::~Simpleforwarder() {}

void Simpleforwarder::update(const SimpleforwarderJsonObject &conf) {
  // This method updates all the object/parameter in Simpleforwarder object
  // specified in the conf JsonObject.
  // You can modify this implementation.
  Cube::set_conf(conf.getBase());

  if (conf.actionsIsSet()) {
    for (auto &i : conf.getActions()) {
      auto inport = i.getInport();
      auto m = getActions(inport);
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

SimpleforwarderJsonObject Simpleforwarder::toJsonObject() {
  SimpleforwarderJsonObject conf;
  conf.setBase(Cube::to_json());

  // Remove comments when you implement all sub-methods
  for (auto &i : getActionsList()) {
    conf.addActions(i->toJsonObject());
  }

  for (auto &i : getPortsList()) {
    conf.addPorts(i->toJsonObject());
  }

  return conf;
}

std::string Simpleforwarder::generate_code() {
  return simpleforwarder_code;
}

std::vector<std::string> Simpleforwarder::generate_code_vector() {
  throw std::runtime_error("Method not implemented");
}

void Simpleforwarder::packet_in(Ports &port,
                                polycube::service::PacketInMetadata &md,
                                const std::vector<uint8_t> &packet) {
  logger()->info("Packet received from port {0}", port.name());
}

std::shared_ptr<Ports> Simpleforwarder::getPorts(const std::string &name) {
  return get_port(name);
}

std::vector<std::shared_ptr<Ports>> Simpleforwarder::getPortsList() {
  return get_ports();
}

void Simpleforwarder::addPorts(const std::string &name,
                               const PortsJsonObject &conf) {
  add_port<PortsJsonObject>(name, conf);
}

void Simpleforwarder::addPortsList(const std::vector<PortsJsonObject> &conf) {
  for (auto &i : conf) {
    std::string name_ = i.getName();
    addPorts(name_, i);
  }
}

void Simpleforwarder::replacePorts(const std::string &name,
                                   const PortsJsonObject &conf) {
  delPorts(name);
  std::string name_ = conf.getName();
  addPorts(name_, conf);
}

void Simpleforwarder::delPorts(const std::string &name) {
  remove_port(name);
}

void Simpleforwarder::delPortsList() {
  auto ports = get_ports();
  for (auto it : ports) {
    delPorts(it->name());
  }
}

std::shared_ptr<Actions> Simpleforwarder::getActions(
    const std::string &inport) {
  uint16_t inPortId;
  try {
    inPortId = get_port(inport)->index();
  } catch (std::exception &e) {
    throw std::runtime_error("Port " + inport + " does not exist");
  }

  auto actions_table = get_hash_table<uint16_t, action>("actions");
  action map_value;

  try {
    map_value = actions_table.get(inPortId);
  } catch (std::exception &e) {
    throw std::runtime_error("No entry found for the port " + inport);
  }

  uint16_t actionId = map_value.action;
  uint16_t outPortId = map_value.port;

  ActionsJsonObject actionsEntry;
  actionsEntry.setAction(Actions::actionNumberToEnum(actionId));
  actionsEntry.setInport(inport);
  try {
    if (actionsEntry.getAction() == ActionsActionEnum::FORWARD)
      actionsEntry.setOutport(get_port(actionId)->name());
  } catch (std::exception &e) {
    logger()->info("This port has no outport set yet!");
  }

  return std::make_shared<Actions>(*this, actionsEntry);
}

std::vector<std::shared_ptr<Actions>> Simpleforwarder::getActionsList() {
  std::vector<std::shared_ptr<Actions>> actionsPtr;

  auto actions_table = get_hash_table<uint16_t, action>("actions");
  auto entries = actions_table.get_all();

  // TODO: this is not optimal, table has alraedy been read, so this is not
  // needed to read it once again
  for (auto &entry : entries) {
    actionsPtr.push_back(getActions(std::to_string(entry.first)));
  }

  return actionsPtr;
}

void Simpleforwarder::addActions(const std::string &inport,
                                 const ActionsJsonObject &conf) {
  if (conf.getAction() == ActionsActionEnum::FORWARD) {
    if (!conf.outportIsSet())
      throw std::runtime_error("Outport is mandatory for FORWARD action");
  }

  uint16_t inPortId;
  uint16_t outPortId;
  try {
    inPortId = get_port(conf.getInport())->index();
  } catch (std::exception &e) {
    throw std::runtime_error("Port " + conf.getInport() + " does not exist");
  }
  if (conf.outportIsSet()) {
    try {
      outPortId = get_port(conf.getOutport())->index();
    } catch (std::exception &e) {
      throw std::runtime_error("Port " + conf.getOutport() + " does not exist");
    }
  }

  action map_value{
      .action = Actions::actionEnumToNumber(conf.getAction()),
      .port = outPortId,
  };

  auto actions_table = get_hash_table<uint16_t, action>("actions");
  actions_table.set(inPortId, map_value);
}

void Simpleforwarder::addActionsList(
    const std::vector<ActionsJsonObject> &conf) {
  for (auto &i : conf) {
    std::string inport_ = i.getInport();
    addActions(inport_, i);
  }
}

void Simpleforwarder::replaceActions(const std::string &inport,
                                     const ActionsJsonObject &conf) {
  delActions(inport);
  std::string inport_ = conf.getInport();
  addActions(inport_, conf);
}

void Simpleforwarder::delActions(const std::string &inport) {
  // This method removes the single Actions object specified by its keys.
  uint16_t inPortId;
  try {
    inPortId = get_port(inport)->index();
  } catch (std::exception &e) {
    throw std::runtime_error("Port " + inport + " does not exist");
  }

  try {
    auto actions_table = get_hash_table<uint16_t, action>("actions");
    actions_table.remove(inPortId);
  } catch (std::exception &e) {
    throw std::runtime_error("No entry found for the port " + inport);
  }
}

void Simpleforwarder::delActionsList() {
  auto actions_table = get_hash_table<uint16_t, action>("actions");
  actions_table.remove_all();
}