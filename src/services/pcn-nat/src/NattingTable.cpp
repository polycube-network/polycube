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

#include "NattingTable.h"
#include "Nat.h"

using namespace polycube::service;

NattingTable::NattingTable(Nat &parent, const NattingTableJsonObject &conf)
    : parent_(parent) {
  logger()->info("Creating NattingTable instance");
}

NattingTable::NattingTable(Nat &parent, const std::string srcIp,
                           const std::string dstIp, const uint16_t srcPort,
                           const uint16_t dstPort, const uint8_t proto,
                           const std::string newIp, const uint16_t newPort,
                           const uint8_t originatingRule)
    : parent_(parent) {
  this->srcIp = srcIp;
  this->dstIp = dstIp;
  this->srcPort = srcPort;
  this->dstPort = dstPort;
  this->newIp = newIp;
  this->newPort = newPort;
  this->originatingRule = (NattingTableOriginatingRuleEnum)originatingRule;
  this->proto = parent.proto_from_int_to_string(proto);
}

NattingTable::~NattingTable() {}

void NattingTable::update(const NattingTableJsonObject &conf) {
  // This method updates all the object/parameter in NattingTable object
  // specified in the conf JsonObject.
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

NattingTableJsonObject NattingTable::toJsonObject() {
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

std::string NattingTable::getInternalSrc() {
  // This method retrieves the internalSrc value.
  return srcIp;
}

std::string NattingTable::getInternalDst() {
  // This method retrieves the internalDst value.
  return dstIp;
}

uint16_t NattingTable::getInternalSport() {
  // This method retrieves the internalSport value.
  return srcPort;
}

uint16_t NattingTable::getInternalDport() {
  // This method retrieves the internalDport value.
  return dstPort;
}

std::string NattingTable::getProto() {
  // This method retrieves the proto value.
  return proto;
}

NattingTableOriginatingRuleEnum NattingTable::getOriginatingRule() {
  // This method retrieves the originatingRule value.
  return originatingRule;
}

void NattingTable::setOriginatingRule(
    const NattingTableOriginatingRuleEnum &value) {
  // This method set the originatingRule value.
  throw std::runtime_error("Cannot modify a natting table entry");
}

std::string NattingTable::getExternalIp() {
  // This method retrieves the externalIp value.
  return newIp;
}

void NattingTable::setExternalIp(const std::string &value) {
  // This method set the externalIp value.
  throw std::runtime_error("Cannot modify a natting table entry");
}

uint16_t NattingTable::getExternalPort() {
  // This method retrieves the externalPort value.
  return newPort;
}

void NattingTable::setExternalPort(const uint16_t &value) {
  // This method set the externalPort value.
  throw std::runtime_error("Cannot modify a natting table entry");
}

std::shared_ptr<spdlog::logger> NattingTable::logger() {
  return parent_.logger();
}
