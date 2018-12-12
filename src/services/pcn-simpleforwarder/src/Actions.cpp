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

#include "Actions.h"
#include "Simpleforwarder.h"

Actions::Actions(Simpleforwarder &parent, const ActionsJsonObject &conf)
    : parent_(parent) {
  logger()->info("Creating Actions instance");
  inport = conf.getInport();
}

Actions::~Actions() {}

void Actions::update(const ActionsJsonObject &conf) {
  // This method updates all the object/parameter in Actions object specified in
  // the conf JsonObject.

  if (conf.actionIsSet()) {
    setAction(conf.getAction());
  }

  if (conf.outportIsSet()) {
    setOutport(conf.getOutport());
  }
}

ActionsJsonObject Actions::toJsonObject() {
  ActionsJsonObject conf;

  conf.setAction(getAction());

  conf.setOutport(getOutport());

  conf.setInport(getInport());

  return conf;
}

void Actions::create(Simpleforwarder &parent, const std::string &inport,
                     const ActionsJsonObject &conf) {
  // This method creates the actual Actions object given thee key param.

  if (conf.getAction() == ActionsActionEnum::FORWARD) {
    if (!conf.outportIsSet())
      throw std::runtime_error("Outport is mandatory for FORWARD action");
  }

  uint16_t inPortId;
  uint16_t outPortId;
  try {
    inPortId = parent.get_port(conf.getInport())->index();
  } catch (std::exception &e) {
    throw std::runtime_error("Port " + conf.getInport() + " does not exist");
  }
  if (conf.outportIsSet()) {
    try {
      outPortId = parent.get_port(conf.getOutport())->index();
    } catch (std::exception &e) {
      throw std::runtime_error("Port " + conf.getOutport() + " does not exist");
    }
  }

  action map_value {
    .action = actionEnumToNumber(conf.getAction()),
    .port = outPortId,
  };

  auto actions_table = parent.get_hash_table<uint16_t, action>("actions");
  actions_table.set(inPortId, map_value);
}

std::shared_ptr<Actions> Actions::getEntry(Simpleforwarder &parent,
                                           const std::string &inport) {
  uint16_t inPortId;
  try {
    inPortId = parent.get_port(inport)->index();
  } catch (std::exception &e) {
    throw std::runtime_error("Port " + inport + " does not exist");
  }

  auto actions_table = parent.get_hash_table<uint16_t, action>("actions");
  action map_value;

  try {
    map_value = actions_table.get(inPortId);
  } catch (std::exception &e) {
    throw std::runtime_error("No entry found for the port " + inport);
  }

  uint16_t actionId = map_value.action;
  uint16_t outPortId = map_value.port;

  ActionsJsonObject actionsEntry;
  actionsEntry.setAction(actionNumberToEnum(actionId));
  actionsEntry.setInport(inport);
  try {
    if (actionsEntry.getAction() == ActionsActionEnum::FORWARD)
      actionsEntry.setOutport(parent.get_port(actionId)->name());
  } catch (std::exception &e) {
    parent.logger()->info("This port has no outport set yet!");
  }

  return std::make_shared<Actions>(parent, actionsEntry);
}

void Actions::removeEntry(Simpleforwarder &parent, const std::string &inport) {
  // This method removes the single Actions object specified by its keys.
  uint16_t inPortId;
  try {
    inPortId = parent.get_port(inport)->index();
  } catch (std::exception &e) {
    throw std::runtime_error("Port " + inport + " does not exist");
  }

  try {
    auto actions_table = parent.get_hash_table<uint16_t, action>("actions");
    actions_table.remove(inPortId);
  } catch (std::exception &e) {
    throw std::runtime_error("No entry found for the port " + inport);
  }
}

std::vector<std::shared_ptr<Actions>> Actions::get(Simpleforwarder &parent) {
  std::vector<std::shared_ptr<Actions>> actionsPtr;

  auto actions_table = parent.get_hash_table<uint16_t, action>("actions");
  auto entries = actions_table.get_all();

  for (auto &entry: entries) {
    actionsPtr.push_back(getEntry(parent, std::to_string(entry.first)));
  }

  return actionsPtr;
}

void Actions::remove(Simpleforwarder &parent) {
  // This method removes all Actions objects in Simpleforwarder.
  auto actions_table = parent.get_hash_table<uint16_t, action>("actions");
  actions_table.remove_all();
}

ActionsActionEnum Actions::getAction() {
  uint16_t inPortId;
  try {
    inPortId = parent_.get_port(inport)->index();
  } catch (std::exception &e) {
    throw std::runtime_error("Port " + inport + " does not exist");
  }

  std::vector<std::string> mapValue;

  try {
    auto actions_table = parent_.get_hash_table<uint16_t, action>("actions");
    action value = actions_table.get(inPortId);
    return actionNumberToEnum(value.action);
  } catch (std::exception &e) {
    throw std::runtime_error("No entry found for the port " + inport);
  }
}

void Actions::setAction(const ActionsActionEnum &value) {
  // This method set the action value.

  uint16_t inPortId;
  try {
    inPortId = parent_.get_port(inport)->index();
  } catch (std::exception &e) {
    throw std::runtime_error("Port " + inport + " does not exist");
  }

  auto actions_table = parent_.get_hash_table<uint16_t, action>("actions");
  action map_value;

  try {
    map_value = actions_table.get(inPortId);
  } catch (std::exception &e) {
    throw std::runtime_error("No entry found for the port " + inport);
  }

  map_value.action = actionEnumToNumber(value);
  actions_table.set(inPortId, map_value);
}

std::string Actions::getOutport() {
  uint16_t inPortId;
  try {
    inPortId = parent_.get_port(inport)->index();
  } catch (std::exception &e) {
    throw std::runtime_error("Port " + inport + " does not exist");
  }

  action value;

  try {
    auto actions_table = parent_.get_hash_table<uint16_t, action>("actions");
    value = actions_table.get(inPortId);
  } catch (std::exception &e) {
    throw std::runtime_error("No entry found for the port " + inport);
  }

  if (actionNumberToEnum(value.action) == ActionsActionEnum::FORWARD)
    return parent_.get_port(value.port)->name();
  else
    throw std::runtime_error("No outport found for the entry");
}

void Actions::setOutport(const std::string &value) {
  // This method set the outport value.
  uint16_t inPortId;
  try {
    inPortId = parent_.get_port(inport)->index();
  } catch (std::exception &e) {
    throw std::runtime_error("Port " + inport + " does not exist");
  }
  auto actions_table = parent_.get_hash_table<uint16_t, action>("actions");
  action map_value;

  try {
    map_value = actions_table.get(inPortId);
  } catch (std::exception &e) {
    throw std::runtime_error("No entry found for the port " + inport);
  }

  try {
    map_value.port = parent_.get_port(value)->index();
  } catch (std::exception &e) {
    throw std::runtime_error("Port " + value + " does not exist");
  }

  actions_table.set(inPortId, map_value);
}

std::string Actions::getInport() {
  return inport;
}

std::shared_ptr<spdlog::logger> Actions::logger() {
  return parent_.logger();
}

ActionsActionEnum Actions::actionNumberToEnum(uint16_t action) {
  if (action == 0)
    return ActionsActionEnum::DROP;
  if (action == 1)
    return ActionsActionEnum::SLOWPATH;
  if (action == 2)
    return ActionsActionEnum::FORWARD;
  throw std::runtime_error("Action not recognized");
}

uint16_t Actions::actionEnumToNumber(ActionsActionEnum action) {
  if (action == ActionsActionEnum::DROP)
    return 0;
  if (action == ActionsActionEnum::SLOWPATH)
    return 1;
  if (action == ActionsActionEnum::FORWARD)
    return 2;

  throw std::runtime_error("Action not recognized");
}
