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
#include "Simplebridge.h"

Ports::Ports(polycube::service::Cube<Ports> &parent,
             std::shared_ptr<polycube::service::PortIface> port,
             const PortsJsonObject &conf)
  : Port(port), parent_(static_cast<Simplebridge&>(parent)) {
  logger()->info("Creating Ports instance");
  if(conf.peerIsSet()) {
    setPeer(conf.getPeer());
  }

  //This MAC address is not used in the datapath. We set only the variable here
  if(conf.macIsSet()) {
    mac_ = conf.getMac();
  } else {
    mac_ = utils::get_random_mac();
    logger()->info("[Ports] New port add with random mac: {0}", mac_);
  }

}

Ports::~Ports() {
  try {
    auto fwdtable = parent_.get_hash_table<uint64_t, fwd_entry>("fwdtable");
    auto fdb = fwdtable.get_all();

    for(auto &pair : fdb) {
      auto map_key = pair.first;
      auto map_entry = pair.second;
      if (map_entry.port == index()) {
        fwdtable.remove(map_key);
        std::string mac_address = utils::be_uint_to_mac_string(map_key);
        logger()->debug("Removing entry {0} associated to port {1}", mac_address, name());
      }
    }
  } catch (std::exception &e) {
    logger()->error("Error while trying to get the Filtering database table: {0}",
                     e.what());
  }
}

void Ports::update(const PortsJsonObject &conf) {
  //This method updates all the object/parameter in Ports object specified in the conf JsonObject.
  //You can modify this implementation.

  if(conf.peerIsSet()) {
    setPeer(conf.getPeer());
  }
}

PortsJsonObject Ports::toJsonObject(){
  PortsJsonObject conf;

  conf.setStatus(getStatus());
  conf.setPeer(getPeer());
  conf.setMac(getMac());
  conf.setName(getName());
  conf.setUuid(getUuid());

  return conf;
}


void Ports::create(Simplebridge &parent, const std::string &name, const PortsJsonObject &conf){

  //This method creates the actual Ports object given thee key param.
  //Please remember to call here the create static method for all sub-objects of Ports.

  parent.add_port<PortsJsonObject>(name, conf);


  parent.logger()->info("New port created with name {0}", name);
}

std::shared_ptr<Ports> Ports::getEntry(Simplebridge &parent, const std::string &name){
  //This method retrieves the pointer to Ports object specified by its keys.
  //logger()->info("Called getEntry with name: {0}", name);
  return parent.get_port(name);
}

void Ports::removeEntry(Simplebridge &parent, const std::string &name){
  //This method removes the single Ports object specified by its keys.
  //Remember to call here the remove static method for all-sub-objects of Ports.
  parent.remove_port(name);
}

std::vector<std::shared_ptr<Ports>> Ports::get(Simplebridge &parent){
  //This methods get the pointers to all the Ports objects in Simplebridge.
  return parent.get_ports();
}

void Ports::remove(Simplebridge &parent){
  //This method removes all Ports objects in Simplebridge.
  //Remember to call here the remove static method for all-sub-objects of Ports.
  auto ports = parent.get_ports();
  for (auto it : ports) {
    removeEntry(parent, it->name());
  }
}

std::string Ports::getMac(){
  //This method retrieves the mac value.
  return mac_;
}

std::shared_ptr<spdlog::logger> Ports::logger() {
  return parent_.logger();
}


