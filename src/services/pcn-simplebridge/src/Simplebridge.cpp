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


#include "Simplebridge.h"
#include "Simplebridge_dp.h"

#include <tins/ethernetII.h>
#include <tins/tins.h>

using namespace Tins;

Simplebridge::Simplebridge(const std::string name, const SimplebridgeJsonObject &conf, CubeType type)
                             : Cube(name, {generate_code()}, {}, type, conf.getPolycubeLoglevel()) {
  logger()->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [Simplebridge] [%n] [%l] %v");
  logger()->info("Creating Simplebridge instance");

  addFdb(conf.getFdb());
  addPortsList(conf.getPorts());
}

Simplebridge::~Simplebridge() {
  // we are destroying this service, prepare for it.
  Cube::dismount();
}

void Simplebridge::update(const SimplebridgeJsonObject &conf) {
  //This method updates all the object/parameter in Simplebridge object specified in the conf JsonObject.
  //You can modify this implementation.

  if(conf.fdbIsSet()) {
    auto m = getFdb();
    m->update(conf.getFdb());
  }

  if(conf.loglevelIsSet()) {
    setLoglevel(conf.getLoglevel());
  }

  if(conf.portsIsSet()) {
    for(auto &i : conf.getPorts()){
      auto name = i.getName();
      auto m = getPorts(name);
      m->update(i);
    }
  }
}

SimplebridgeJsonObject Simplebridge::toJsonObject(){
  SimplebridgeJsonObject conf;

  conf.setFdb(getFdb()->toJsonObject());
  conf.setUuid(getUuid());
  conf.setLoglevel(getLoglevel());
  conf.setType(getType());

  for(auto &i : getPortsList()){
    conf.addPorts(i->toJsonObject());
  }

  conf.setName(getName());
  return conf;
}

std::string Simplebridge::generate_code(){
  return simplebridge_code;
}

std::vector<std::string> Simplebridge::generate_code_vector(){
  throw std::runtime_error("Method not implemented");
}

void Simplebridge::packet_in(Ports &port, polycube::service::PacketInMetadata &md, const std::vector<uint8_t> &packet){
  logger()->debug("Packet received from port {0}", port.name());

  try {
    //logger()->debug("[{0}] packet from port: '{1}' id: '{2}' peer:'{3}'",
    //  get_name(), port.name(), port.index(), port.peer());

    switch (static_cast<SlowPathReason>(md.reason)) {
    case SlowPathReason::FLOODING:
      logger()->debug("Received a pkt in slowpath - reason: FLOODING");
      flood_packet(port, md, packet);
      break;
    default:
      logger()->error("Not valid reason {0} received", md.reason);
    }
  } catch (const std::exception &e) {
    logger()->error("exception during slowpath packet processing: '{0}'", e.what());
  }
}

void Simplebridge::flood_packet(Port &port, PacketInMetadata &md,
                                const std::vector<uint8_t> &packet) {
  EthernetII p(&packet[0], packet.size());

  // avoid adding or removing ports while flooding packet
  std::lock_guard<std::mutex> guard(ports_mutex_);

  for (auto &it : get_ports()) {
    if (it->name() == port.name()) {
      continue;
    }
    it->send_packet_out(p);
    logger()->trace("Packet sent to port {0} as result of flooding", it->name());
  }
}

void Simplebridge::reloadCodeWithAgingtime(uint32_t aging_time) {
  logger()->debug("Reloading code with agingtime: {0}", aging_time);

  std::string aging_time_str("#define AGING_TIME " + std::to_string(aging_time));

  reload(aging_time_str + simplebridge_code);

  logger()->trace("New bridge code reloaded");
}











