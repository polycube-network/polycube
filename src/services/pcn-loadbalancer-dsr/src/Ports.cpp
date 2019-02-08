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

#include "Ports.h"
#include "Lbdsr.h"

Ports::Ports(polycube::service::Cube<Ports> &parent,
             std::shared_ptr<polycube::service::PortIface> port,
             const PortsJsonObject &conf)
    : Port(port), parent_(static_cast<Lbdsr &>(parent)) {
  logger()->info("Creating Ports instance {0} ", port->name());
  logger()->debug("Creating a new port with type: {0} and name: {1} ",
                  PortsJsonObject::PortsTypeEnum_to_string(conf.getType()),
                  port->name());

  // Type is mandatory, validation performed by the framework
  port_type_ = conf.getType();

  switch (port_type_) {
  case PortsTypeEnum::FRONTEND:
    parent_.setFrontendPort(port->name());
    break;

  case PortsTypeEnum::BACKEND:
    parent_.setBackendPort(port->name());
    break;
  }

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

  if (conf.typeIsSet()) {
    setType(conf.getType());
  }
}

PortsJsonObject Ports::toJsonObject() {
  PortsJsonObject conf;

  conf.setStatus(getStatus());

  conf.setPeer(getPeer());

  conf.setType(getType());

  conf.setName(getName());

  conf.setUuid(getUuid());

  return conf;
}

void Ports::create(Lbdsr &parent, const std::string &name,
                   const PortsJsonObject &conf) {
  // This method creates the actual Ports object given thee key param.
  // Please remember to call here the create static method for all sub-objects
  // of Ports.

  parent.add_port<PortsJsonObject>(name, conf);
}

std::shared_ptr<Ports> Ports::getEntry(Lbdsr &parent, const std::string &name) {
  // This method retrieves the pointer to Ports object specified by its keys.
  // logger()->info("Called getEntry with name: {0}", name);
  return parent.get_port(name);
}

void Ports::removeEntry(Lbdsr &parent, const std::string &name) {
  // This method removes the single Ports object specified by its keys.
  // Remember to call here the remove static method for all-sub-objects of
  // Ports.
  parent.rmPort(name);
  parent.remove_port(name);
}

std::vector<std::shared_ptr<Ports>> Ports::get(Lbdsr &parent) {
  // This methods get the pointers to all the Ports objects in Lbdsr.
  return parent.get_ports();
}

void Ports::remove(Lbdsr &parent) {
  // This method removes all Ports objects in Lbdsr.
  // Remember to call here the remove static method for all-sub-objects of
  // Ports.
  auto ports = parent.get_ports();
  for (auto it : ports) {
    removeEntry(parent, it->name());
  }
}

PortsTypeEnum Ports::getType() {
  // This method retrieves the type value.
  return port_type_;
}

void Ports::setType(const PortsTypeEnum &value) {
  // This method set the type value.
  throw std::runtime_error(
      "[Ports]: Type can be set only during port creation");
}

std::shared_ptr<spdlog::logger> Ports::logger() {
  return parent_.logger();
}
