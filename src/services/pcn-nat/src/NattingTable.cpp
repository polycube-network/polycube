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


#include "NattingTable.h"
#include "Nat.h"

using namespace polycube::service;

NattingTable::NattingTable(Nat &parent, const NattingTableJsonObject &conf): parent_(parent) {
  logger()->info("Creating NattingTable instance");
}

NattingTable::NattingTable(Nat &parent, const std::string srcIp, const std::string dstIp,
              const uint16_t srcPort, const uint16_t dstPort, const uint8_t proto,
              const std::string newIp, const uint16_t newPort, const uint8_t originatingRule): parent_(parent) {
  this->srcIp = srcIp;
  this->dstIp = dstIp;
  this->srcPort = srcPort;
  this->dstPort = dstPort;
  this->newIp = newIp;
  this->newPort = newPort;
  this->originatingRule = (NattingTableOriginatingRuleEnum) originatingRule;
  this->proto = parent.proto_from_int_to_string(proto);
}

NattingTable::~NattingTable() { }

void NattingTable::update(const NattingTableJsonObject &conf) {
  // This method updates all the object/parameter in NattingTable object specified in the conf JsonObject.
  // You can modify this implementation.
  if (conf.originatingRuleIsSet()) {
    setOriginatingRule(conf.getOriginatingRule());
  }
  if (conf.externalIpIsSet()) {
    setExternalIp(conf.getExternalIp());
  }
  if (conf.externalPortIsSet()) {
    setExternalPort(conf.getExternalPort());
  }
}

NattingTableJsonObject NattingTable::toJsonObject(){
  NattingTableJsonObject conf;
  conf.setInternalSrc(getInternalSrc());
  conf.setInternalDst(getInternalDst());
  conf.setInternalSport(getInternalSport());
  conf.setInternalDport(getInternalDport());
  conf.setProto(getProto());
  conf.setOriginatingRule(getOriginatingRule());
  conf.setExternalIp(getExternalIp());
  conf.setExternalPort(getExternalPort());

  return conf;
}


void NattingTable::create(Nat &parent, const std::string &internalSrc, const std::string &internalDst, const uint16_t &internalSport, const uint16_t &internalDport, const std::string &proto, const NattingTableJsonObject &conf){
  // This method creates the actual NattingTable object given thee key param.
  // Please remember to call here the create static method for all sub-objects of NattingTable.
  throw std::runtime_error("Cannot manually create natting table entries");
}

std::shared_ptr<NattingTable> NattingTable::getEntry(Nat &parent,
                                                    const std::string &internalSrc,
                                                    const std::string &internalDst,
                                                    const uint16_t &internalSport,
                                                    const uint16_t &internalDport,
                                                    const std::string &proto){
  // This method retrieves the pointer to NattingTable object specified by its keys.
  try {
    auto table = parent.get_hash_table<st_k, st_v>("egress_session_table");
    st_k map_key {
      .src_ip = utils::ip_string_to_be_uint(internalSrc),
      .dst_ip = utils::ip_string_to_be_uint(internalDst),
      .src_port = htons(internalSport),
      .dst_port = htons(internalDport),
      .proto = std::stol(proto),
    };

    st_v value = table.get(map_key);

    std::string newIp = utils::be_uint_to_ip_string(value.new_ip);
    uint16_t newPort = value.new_port;
    uint8_t originatingRule = value.originating_rule_type;;
    auto entry = std::make_shared<NattingTable>(parent, internalSrc, internalDst,
                                                internalSport, internalDport, parent.proto_from_string_to_int(proto),
                                                newIp, newPort, originatingRule);
    return entry;
  } catch (std::exception &e) {
    throw std::runtime_error("Natting table entry not found");
  }
}

void NattingTable::removeEntry(Nat &parent, const std::string &internalSrc, const std::string &internalDst, const uint16_t &internalSport, const uint16_t &internalDport, const std::string &proto){
  // This method removes the single NattingTable object specified by its keys.
  // Remember to call here the remove static method for all-sub-objects of NattingTable.
  throw std::runtime_error("Cannot manually remove single natting table entries");
}

std::vector<std::shared_ptr<NattingTable>> NattingTable::get(Nat &parent){
  // This methods get the pointers to all the NattingTable objects in Nat.
  std::vector<std::shared_ptr<NattingTable>> entries;
  try {
    auto table = parent.get_hash_table<st_k, st_v>("egress_session_table");
    auto map_entries = table.get_all();
    for (auto &pair : map_entries) {
      auto key = pair.first;
      auto value = pair.second;

      auto entry = std::make_shared<NattingTable>(parent,
        utils::be_uint_to_ip_string(key.src_ip),
        utils::be_uint_to_ip_string(key.dst_ip),
        ntohs(key.src_port),
        ntohs(key.dst_port),
        key.proto,
        utils::be_uint_to_ip_string(value.new_ip),
        ntohs(value.new_port),
        value.originating_rule_type
      );

      entries.push_back(entry);
    }
  } catch (std::exception &e) {
    throw std::runtime_error("Unable to get the natting table");
  }
  return entries;
}

void NattingTable::remove(Nat &parent){
  // This method removes all NattingTable objects in Nat.
  // Remember to call here the remove static method for all-sub-objects of NattingTable.

  // Flush the natting tables in datapath
  auto egress_table = parent.get_hash_table<st_k, st_v>("egress_session_table");
  egress_table.remove_all();
  auto ingress_table = parent.get_hash_table<st_k, st_v>("ingress_session_table");
  ingress_table.remove_all();

  parent.logger()->info("Flushed natting tables");
}

std::string NattingTable::getInternalSrc(){
  // This method retrieves the internalSrc value.
  return srcIp;
}


std::string NattingTable::getInternalDst(){
  // This method retrieves the internalDst value.
  return dstIp;
}


uint16_t NattingTable::getInternalSport(){
  // This method retrieves the internalSport value.
  return srcPort;
}


uint16_t NattingTable::getInternalDport(){
  // This method retrieves the internalDport value.
  return dstPort;
}


std::string NattingTable::getProto(){
  // This method retrieves the proto value.
  return proto;
}


NattingTableOriginatingRuleEnum NattingTable::getOriginatingRule(){
  // This method retrieves the originatingRule value.
  return originatingRule;
}

void NattingTable::setOriginatingRule(const NattingTableOriginatingRuleEnum &value){
  // This method set the originatingRule value.
  throw std::runtime_error("Cannot modify a natting table entry");
}


std::string NattingTable::getExternalIp(){
  // This method retrieves the externalIp value.
  return newIp;
}

void NattingTable::setExternalIp(const std::string &value){
  // This method set the externalIp value.
  throw std::runtime_error("Cannot modify a natting table entry");
}


uint16_t NattingTable::getExternalPort(){
  // This method retrieves the externalPort value.
  return newPort;
}

void NattingTable::setExternalPort(const uint16_t &value){
  // This method set the externalPort value.
  throw std::runtime_error("Cannot modify a natting table entry");
}



std::shared_ptr<spdlog::logger> NattingTable::logger() {
  return parent_.logger();
}
