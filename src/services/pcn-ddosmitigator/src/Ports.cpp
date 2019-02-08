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

#include "Ports.h"
#include "Ddosmitigator.h"

Ports::Ports(polycube::service::Cube<Ports> &parent,
             std::shared_ptr<polycube::service::PortIface> port,
             const PortsJsonObject &conf)
    : Port(port), parent_(static_cast<Ddosmitigator &>(parent)) {
  logger()->info("Creating Ports instance");
  if (conf.peerIsSet()) {
    setPeer(conf.getPeer());
  }
}

Ports::~Ports() {}

void Ports::update(const PortsJsonObject &conf) {
  // This method updates all the object/parameter in Ports object specified in
  // the conf JsonObject.
  // You can modify this implementation.

  if (conf.peerIsSet()) {
    setPeer(conf.getPeer());
  }
}

PortsJsonObject Ports::toJsonObject() {
  PortsJsonObject conf;

  conf.setStatus(getStatus());

  conf.setPeer(getPeer());

  conf.setName(getName());

  conf.setUuid(getUuid());

  return conf;
}

void Ports::create(Ddosmitigator &parent, const std::string &name,
                   const PortsJsonObject &conf) {
  // This method creates the actual Ports object given thee key param.
  // Please remember to call here the create static method for all sub-objects
  // of Ports.

  // call method in base class
  auto p = parent.add_port<PortsJsonObject>(name, conf);

  parent.addPort(name);
  // logger()->info("New port create with name {0}", name);
}

std::shared_ptr<Ports> Ports::getEntry(Ddosmitigator &parent,
                                       const std::string &name) {
  // This method retrieves the pointer to Ports object specified by its keys.
  // logger()->info("Called getEntry with name: {0}", name);
  return parent.get_port(name);
}

void Ports::removeEntry(Ddosmitigator &parent, const std::string &name) {
  // This method removes the single Ports object specified by its keys.
  // Remember to call here the remove static method for all-sub-objects of
  // Ports.
  // call to base class
  parent.remove_port(name);
  parent.rmPort(name);
}

std::vector<std::shared_ptr<Ports>> Ports::get(Ddosmitigator &parent) {
  // This methods get the pointers to all the Ports objects in Ddosmitigator.
  return parent.get_ports();
}

void Ports::remove(Ddosmitigator &parent) {
  // This method removes all Ports objects in Ddosmitigator.
  // Remember to call here the remove static method for all-sub-objects of
  // Ports.
  auto ports = parent.get_ports();
  for (auto it : ports) {
    removeEntry(parent, it->name());
  }
}

std::shared_ptr<spdlog::logger> Ports::logger() {
  return parent_.logger();
}
