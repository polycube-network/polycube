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


//Modify these methods with your own implementation


#include "Ports.h"
#include "Lbrp.h"

Ports::Ports(polycube::service::Cube<Ports> &parent,
             std::shared_ptr<polycube::service::PortIface> port,
             const PortsJsonObject &conf)
  : Port(port), parent_(static_cast<Lbrp&>(parent)) {
  logger()->info("Creating Ports instance");
  if(conf.peerIsSet()) {
    setPeer(conf.getPeer());
  }

  port_type_ = conf.getType();
}

Ports::~Ports() { }

void Ports::update(const PortsJsonObject &conf) {
  //This method updates all the object/parameter in Ports object specified in the conf JsonObject.
  //You can modify this implementation.

  if(conf.peerIsSet()) {
    setPeer(conf.getPeer());
  }

  if(conf.typeIsSet()) {
    setType(conf.getType());
  }
}

PortsJsonObject Ports::toJsonObject(){
  PortsJsonObject conf;

  conf.setStatus(getStatus());
  conf.setPeer(getPeer());
  conf.setType(getType());
  conf.setName(getName());
  conf.setUuid(getUuid());
  return conf;
}


void Ports::create(Lbrp &parent, const std::string &name, const PortsJsonObject &conf){
  if(parent.get_ports().size() == 2) {
    parent.logger()->warn("Reached maximum number of ports");
    throw std::runtime_error("Reached maximum number of ports");
  }

  try {
    switch(conf.getType()) {
      case PortsTypeEnum::FRONTEND:
        if(parent.getFrontendPort() != nullptr) {
          parent.logger()->warn("There is already a FRONTEND port");
          throw std::runtime_error("There is already a FRONTEND port");
        }
        break;
      case PortsTypeEnum::BACKEND:
        if(parent.getBackendPort() != nullptr) {
          parent.logger()->warn("There is already a BACKEND port");
          throw std::runtime_error("There is already a BACKEND port");
        }
        break;
    }
  } catch (std::runtime_error &e) {
    parent.logger()->warn("Error when adding the port {0}", name);
    parent.logger()->warn("Error message: {0}", e.what());
    throw;
  }

  parent.add_port<PortsJsonObject>(name, conf);

  if(parent.get_ports().size() == 2) {
    parent.logger()->info("Reloading code because of the new port");
    parent.reloadCodeWithNewPorts();
  }

  parent.logger()->info("New port created with name {0}", name);
}

std::shared_ptr<Ports> Ports::getEntry(Lbrp &parent, const std::string &name){
  //This method retrieves the pointer to Ports object specified by its keys.
  //logger()->info("Called getEntry with name: {0}", name);
  return parent.get_port(name);
}

void Ports::removeEntry(Lbrp &parent, const std::string &name){
  //This method removes the single Ports object specified by its keys.
  //Remember to call here the remove static method for all-sub-objects of Ports.
  parent.remove_port(name);
}

std::vector<std::shared_ptr<Ports>> Ports::get(Lbrp &parent){
  //This methods get the pointers to all the Ports objects in Lbrp.
  return parent.get_ports();
}

void Ports::remove(Lbrp &parent){
  //This method removes all Ports objects in Lbrp.
  //Remember to call here the remove static method for all-sub-objects of Ports.
  auto ports = parent.get_ports();
  for (auto it : ports) {
    removeEntry(parent, it->name());
  }
}


PortsTypeEnum Ports::getType(){
  //This method retrieves the type value.
  return port_type_;
}

void Ports::setType(const PortsTypeEnum &value){
  //This method set the type value.
  throw std::runtime_error("Error: Port type cannot be changed at runtime.");
}

std::shared_ptr<spdlog::logger> Ports::logger() {
  return parent_.logger();
}
