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
#include "K8switch.h"

Ports::Ports(polycube::service::Cube<Ports> &parent,
             std::shared_ptr<polycube::service::PortIface> port,
             const PortsJsonObject &conf)
    : Port(port), parent_(static_cast<K8switch &>(parent)) {
  logger()->info("Creating Ports instance");
  port_type_ = conf.getType();
}

Ports::~Ports() {}

void Ports::update(const PortsJsonObject &conf) {
  // This method updates all the object/parameter in Ports object specified in
  // the conf JsonObject.
  // You can modify this implementation.
  Port::set_conf(conf.getBase());

  if (conf.typeIsSet()) {
    setType(conf.getType());
  }
}

PortsJsonObject Ports::toJsonObject() {
  PortsJsonObject conf;
  conf.setBase(Port::to_json());

  conf.setType(getType());

  return conf;
}

void Ports::create(K8switch &parent, const std::string &name,
                   const PortsJsonObject &conf) {
  bool reload = false;

  try {
    switch (conf.getType()) {
    case PortsTypeEnum::NODEPORT:
      if (parent.getNodePortPort() != nullptr) {
        parent.logger()->warn("There is already a NODEPORT port");
        throw std::runtime_error("There is already a NODEPORT port");
      }
      reload = true;
      break;
    }
  } catch (std::runtime_error &e) {
    parent.logger()->warn("Error when adding the port {0}", name);
    parent.logger()->warn("Error message: {0}", e.what());
    throw;
  }

  parent.add_port<PortsJsonObject>(name, conf);

  if (reload) {
    parent.logger()->info("Nodeport added, reloading code");
    parent.reloadConfig();
  }

  parent.logger()->info("New port created with name {0}", name);
}

std::shared_ptr<Ports> Ports::getEntry(K8switch &parent,
                                       const std::string &name) {
  // This method retrieves the pointer to Ports object specified by its keys.
  // logger()->info("Called getEntry with name: {0}", name);
  return parent.get_port(name);
}

void Ports::removeEntry(K8switch &parent, const std::string &name) {
  // This method removes the single Ports object specified by its keys.
  // Remember to call here the remove static method for all-sub-objects of
  // Ports.
  parent.remove_port(name);
}

std::vector<std::shared_ptr<Ports>> Ports::get(K8switch &parent) {
  // This methods get the pointers to all the Ports objects in K8switch.
  return parent.get_ports();
}

void Ports::remove(K8switch &parent) {
  // This method removes all Ports objects in K8switch.
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
  throw std::runtime_error("Error: Port type cannot be changed at runtime.");
}

std::shared_ptr<spdlog::logger> Ports::logger() {
  return parent_.logger();
}
