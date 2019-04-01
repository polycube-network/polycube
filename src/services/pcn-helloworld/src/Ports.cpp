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
#include "Helloworld.h"

using namespace polycube::service;

Ports::Ports(polycube::service::Cube<Ports> &parent,
             std::shared_ptr<polycube::service::PortIface> port,
             const PortsJsonObject &conf)
    : Port(port), parent_(static_cast<Helloworld &>(parent)) {
  logger()->info("Creating Ports instance");
}

Ports::~Ports() {}

void Ports::update(const PortsJsonObject &conf) {
  Port::set_conf(conf.getBase());
  // This method updates all the object/parameter in Ports object specified in
  // the conf JsonObject.
  // You can modify this implementation.
}

PortsJsonObject Ports::toJsonObject() {
  PortsJsonObject conf;
  conf.setBase(Port::to_json());

  return conf;
}

void Ports::create(Helloworld &parent, const std::string &name,
                   const PortsJsonObject &conf) {
  // This method creates the actual Ports object given thee key param.
  // Please remember to call here the create static method for all sub-objects
  // of Ports.

  if (parent.get_ports().size() == 2) {
    throw std::runtime_error("maximum number of ports reached");
  }

  auto p = parent.add_port<PortsJsonObject>(name, conf);
  parent.logger()->info("port {0} was connected", name);

  auto ports_table = parent.get_array_table<uint16_t>("ports_map");

  uint32_t port_map_index;
  try {
    // Look for first free entry to save the port id
    if (ports_table.get(0x0) == UINT16_MAX) {
      port_map_index = 0x0;
    } else if (ports_table.get(0x1) == UINT16_MAX) {
      port_map_index = 0x1;
    }
  } catch (std::exception &e) {
    parent.logger()->error("port {0} does not exist", name);
    // TODO: should Cube::remove_port be called?
    throw std::runtime_error("Port does not exist");
  }

  ports_table.set(port_map_index, p->index());
}

std::shared_ptr<Ports> Ports::getEntry(Helloworld &parent,
                                       const std::string &name) {
  // This method retrieves the pointer to Ports object specified by its keys.
  // logger()->info("Called getEntry with name: {0}", name);
  return parent.get_port(name);
}

void Ports::removeEntry(Helloworld &parent, const std::string &name) {
  // This method removes the single Ports object specified by its keys.
  int index = parent.get_port(name)->index();

  auto ports_table = parent.get_array_table<uint16_t>("ports_map");

  uint32_t port_map_index;
  if (ports_table.get(0x0) == index) {
    port_map_index = 0x0;
  } else if (ports_table.get(0x1) == index) {
    port_map_index = 0x1;
  }
  // mark entry as free
  ports_table.set(port_map_index, UINT16_MAX);
  parent.remove_port(name);
  parent.logger()->info("port {0} was removed", name);
}

std::vector<std::shared_ptr<Ports>> Ports::get(Helloworld &parent) {
  // This methods get the pointers to all the Ports objects in Helloworld.
  return parent.get_ports();
}

void Ports::remove(Helloworld &parent) {
  // This method removes all Ports objects in Helloworld.
  auto ports = parent.get_ports();
  for (auto it : ports) {
    parent.remove_port(it->name());
  }
}

std::shared_ptr<spdlog::logger> Ports::logger() {
  return parent_.logger();
}
