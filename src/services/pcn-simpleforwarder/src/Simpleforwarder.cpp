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
    : Cube(conf.getBase(), {simpleforwarder_code}, {}),
      SimpleforwarderBase(name) {
  logger()->info("Creating Simpleforwarder instance");
  addPortsList(conf.getPorts());
  addActionsList(conf.getActions());
}

Simpleforwarder::~Simpleforwarder() {
  logger()->info("Destroying Simpleforwarder instance");
}

void Simpleforwarder::packet_in(Ports &port,
                                polycube::service::PacketInMetadata &md,
                                const std::vector<uint8_t> &packet) {
  logger()->info("Packet received from port {0}", port.name());
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