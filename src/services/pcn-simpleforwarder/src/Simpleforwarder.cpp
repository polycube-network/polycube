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
