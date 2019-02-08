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
#include "Nat.h"

using namespace polycube::service;

Ports::Ports(polycube::service::Cube<Ports> &parent,
             std::shared_ptr<polycube::service::PortIface> port,
             const PortsJsonObject &conf)
    : Port(port), parent_(static_cast<Nat &>(parent)) {
  logger()->info("Creating Ports instance: {0}", conf.getName());
  logger()->debug("Creating a new port with type: {0} and name: {1}",
                  PortsJsonObject::PortsTypeEnum_to_string(conf.getType()),
                  conf.getName());

  if (conf.peerIsSet())
    setPeer(conf.getPeer());

  // Type is mandatory, validation performed by the framework
  port_type_ = conf.getType();

  if (port_type_ == PortsTypeEnum::EXTERNAL) {
    ip_ = conf.getIp();
    parent_.external_port_index_ = index();
    parent_.external_ip_ = ip_;
  } else {
    parent_.internal_port_index_ = index();
  }

  parent_.reloadCode();  // apply changes in port
}

Ports::~Ports() {
  parent_.logger()->debug("[Ports] Port {0} destroyed. Type {1}", getName(),
                          PortsJsonObject::PortsTypeEnum_to_string(getType()));
}

void Ports::update(const PortsJsonObject &conf) {
  // This method updates all the object/parameter in Ports object specified in
  // the conf JsonObject.
  // You can modify this implementation.
  if (port_type_ == PortsTypeEnum::EXTERNAL) {
    if (conf.ipIsSet()) {
      setIp(conf.getIp());
    }
  }

  if (conf.peerIsSet()) {
    setPeer(conf.getPeer());
  }
}

PortsJsonObject Ports::toJsonObject() {
  PortsJsonObject conf;

  conf.setStatus(getStatus());
  conf.setType(getType());

  if (port_type_ == PortsTypeEnum::EXTERNAL) {
    conf.setIp(getIp());
  }

  conf.setName(getName());
  conf.setPeer(getPeer());
  conf.setUuid(getUuid());

  return conf;
}

void Ports::create(Nat &parent, const std::string &name,
                   const PortsJsonObject &conf) {
  // This method creates the actual Ports object given thee key param.
  // Please remember to call here the create static method for all sub-objects
  // of Ports.
  if (parent.get_ports().size() == 2) {
    parent.logger()->warn("Reached maximum number of ports");
    throw std::runtime_error("Reached maximum number of ports");
  }

  try {
    switch (conf.getType()) {
    case PortsTypeEnum::INTERNAL:
      Ports::validateInternalPortParams(parent, conf);
      break;
    case PortsTypeEnum::EXTERNAL:
      Ports::validateExternalPortParams(parent, conf);
      break;
    }
  } catch (std::runtime_error &e) {
    parent.logger()->warn("Error when adding the port {0}", name);
    parent.logger()->warn("Error message: {0}", e.what());
    throw;
  }

  parent.add_port<PortsJsonObject>(name, conf);
  parent.logger()->info("New port created with name {0}", name);
}

std::shared_ptr<Ports> Ports::getEntry(Nat &parent, const std::string &name) {
  // This method retrieves the pointer to Ports object specified by its keys.
  return parent.get_port(name);
}

void Ports::removeEntry(Nat &parent, const std::string &name) {
  // This method removes the single Ports object specified by its keys.
  // Remember to call here the remove static method for all-sub-objects of
  // Ports.

  auto port = Ports::getEntry(parent, name);
  switch (port->getType()) {
  case PortsTypeEnum::INTERNAL:
    parent.internal_port_index_ = 0;
    break;
  case PortsTypeEnum::EXTERNAL:
    parent.external_port_index_ = 0;
    parent.external_ip_.clear();
    break;
  }

  parent.remove_port(port->getName());
  parent.reloadCode();
}

std::vector<std::shared_ptr<Ports>> Ports::get(Nat &parent) {
  // This methods get the pointers to all the Ports objects in Nat.
  return parent.get_ports();
}

void Ports::remove(Nat &parent) {
  // This method removes all Ports objects in Nat.
  // Remember to call here the remove static method for all-sub-objects of
  // Ports.
  auto ports = parent.get_ports();
  for (auto it : ports) {
    removeEntry(parent, it->name());
  }
}

PortsTypeEnum Ports::getType() {
  // I don't need to take this value from the dp because it cannot change
  return port_type_;
}

std::string Ports::getIp() {
  // This method retrieves the ip value.
  if (port_type_ == PortsTypeEnum::INTERNAL) {
    throw std::runtime_error("[Ports]: No IP on INTERNAL interface");
  }
  return ip_;
}

void Ports::setIp(const std::string &value) {
  // This method set the ip value.
  logger()->debug("Setting ip: {0}", value);
  if (port_type_ == PortsTypeEnum::EXTERNAL) {
    ip_ = value;
    parent_.external_ip_ = ip_;
  } else {
    logger()->warn("You cannot set the IP for INTERNAL interface");
    throw std::runtime_error(
        "[Ports]: You cannot set the IP for INTERNAL interface");
  }

  parent_.reloadCode();
}

std::shared_ptr<spdlog::logger> Ports::logger() {
  return parent_.logger();
}

void Ports::validateInternalPortParams(Nat &parent,
                                       const PortsJsonObject &conf) {
  if (conf.ipIsSet()) {
    parent.logger()->warn("INTERNAL interface doesn't support ip");
    throw std::runtime_error(
        "You don't need to set the ip address on the internal interface");
  }

  if (parent.getInternalPort() != nullptr) {
    parent.logger()->warn("There is already an internal port");
    throw std::runtime_error("There is already an internal port");
  }
}

void Ports::validateExternalPortParams(Nat &parent,
                                       const PortsJsonObject &conf) {
  if (!conf.ipIsSet()) {
    parent.logger()->warn("EXTERNAL interface needs an external IP");
    throw std::runtime_error(
        "Please set the external IP for the EXTERNAL interface");
  }

  if (parent.getExternalPort() != nullptr) {
    parent.logger()->warn("There is already an external port");
    throw std::runtime_error("There is already an external port");
  }
}
