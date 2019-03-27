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

#include "RulePortForwardingEntry.h"
#include "Nat.h"

using namespace polycube::service;

RulePortForwardingEntry::RulePortForwardingEntry(
    RulePortForwarding &parent, const RulePortForwardingEntryJsonObject &conf)
    : parent_(parent), id(conf.getId()) {
  update(conf);
  logger()->info(
      "Creating RulePortForwardingEntry instance: {0}:{1} -> {2}:{3}",
      conf.getExternalIp(), conf.getExternalPort(), conf.getInternalIp(),
      conf.getInternalPort());
}

RulePortForwardingEntry::~RulePortForwardingEntry() {}

void RulePortForwardingEntry::update(
    const RulePortForwardingEntryJsonObject &conf) {
  // This method updates all the object/parameter in RulePortForwardingEntry
  // object specified in the conf JsonObject.
  // You can modify this implementation.
  if (conf.externalIpIsSet()) {
    setExternalIp(conf.getExternalIp());
  }
  if (conf.externalPortIsSet()) {
    setExternalPort(conf.getExternalPort());
  }
  if (conf.protoIsSet()) {
    setProto(conf.getProto());
  } else {
    proto = ProtoEnum::ALL;
  }
  if (conf.internalIpIsSet()) {
    setInternalIp(conf.getInternalIp());
  }
  if (conf.internalPortIsSet()) {
    setInternalPort(conf.getInternalPort());
  }
}

RulePortForwardingEntryJsonObject RulePortForwardingEntry::toJsonObject() {
  RulePortForwardingEntryJsonObject conf;
  conf.setId(getId());
  conf.setExternalIp(getExternalIp());
  conf.setExternalPort(getExternalPort());
  conf.setProto(getProto());
  conf.setInternalIp(getInternalIp());
  conf.setInternalPort(getInternalPort());
  return conf;
}

uint32_t RulePortForwardingEntry::getId() {
  // This method retrieves the id value.
  return id;
}

std::string RulePortForwardingEntry::getExternalIp() {
  // This method retrieves the externalIp value.
  struct IpAddr addr = {externalIp, 32};
  return addr.toString();
}

void RulePortForwardingEntry::setExternalIp(const std::string &value) {
  // This method set the externalIp value.
  struct IpAddr addr;
  addr.fromString(value);
  this->externalIp = addr.ip;
}

uint16_t RulePortForwardingEntry::getExternalPort() {
  // This method retrieves the externalPort value.
  return externalPort;
}

void RulePortForwardingEntry::setExternalPort(const uint16_t &value) {
  // This method set the externalPort value.
  externalPort = value;
}

std::string RulePortForwardingEntry::getProto() {
  // This method retrieves the proto value.
  switch (proto) {
  case ProtoEnum::TCP:
    return "tcp";
  case ProtoEnum::UDP:
    return "udp";
  case ProtoEnum::ALL:
    return "all";
  }
}

void RulePortForwardingEntry::setProto(const std::string &value) {
  // This method set the proto value.
  if (value.empty()) {
    proto = ProtoEnum::ALL;
    return;
  }

  std::string v = value;
  for_each(v.begin(), v.end(),
           [](char &c) { c = std::tolower(static_cast<unsigned char>(c)); });
  if (v == "tcp") {
    proto = ProtoEnum::TCP;
    return;
  }
  if (v == "udp") {
    proto = ProtoEnum::UDP;
    return;
  }
  if (v == "all" || v.empty()) {
    proto = ProtoEnum::ALL;
    return;
  }

  throw std::runtime_error("Invalid protocol");
}

std::string RulePortForwardingEntry::getInternalIp() {
  // This method retrieves the internalIp value.
  struct IpAddr addr = {internalIp, 32};
  return addr.toString();
}

void RulePortForwardingEntry::setInternalIp(const std::string &value) {
  // This method set the internalIp value.
  struct IpAddr addr;
  addr.fromString(value);
  this->internalIp = addr.ip;
}

uint16_t RulePortForwardingEntry::getInternalPort() {
  // This method retrieves the internalPort value.
  return internalPort;
}

void RulePortForwardingEntry::setInternalPort(const uint16_t &value) {
  // This method set the internalPort value.
  internalPort = value;
}

std::shared_ptr<spdlog::logger> RulePortForwardingEntry::logger() {
  return parent_.logger();
}

void RulePortForwardingEntry::injectToDatapath() {
  auto dp_rules = parent_.parent_.getParent().get_hash_table<dp_k, dp_v>(
      "dp_rules", 0, ProgramType::INGRESS);

  dp_k key{
      .mask = 0, /* will be set later on */
      .external_ip = externalIp,
      .external_port = htons(externalPort),
      .proto = 0, /* will be set later on */
  };

  dp_v value{
      .internal_ip = internalIp,
      .internal_port = htons(internalPort),
      .entry_type = (uint8_t)NattingTableOriginatingRuleEnum::PORTFORWARDING,
  };

  if (proto == ProtoEnum::ALL) {
    key.mask = 48;
    key.proto = 0;
  } else if (proto == ProtoEnum::TCP) {
    key.mask = 56;
    key.proto = (uint8_t)IPPROTO_TCP;
  } else if (proto == ProtoEnum::UDP) {
    key.mask = 56;
    key.proto = (uint8_t)IPPROTO_UDP;
  } else {
    return;  // TODO: is this case possible?
  }

  dp_rules.set(key, value);
}

void RulePortForwardingEntry::removeFromDatapath() {
  auto dp_rules = parent_.parent_.getParent().get_hash_table<dp_k, dp_v>(
      "dp_rules", 0, ProgramType::INGRESS);

  dp_k key{
      .mask = 0, /* will be set later on */
      .external_ip = externalIp,
      .external_port = htons(externalPort),
      .proto = 0, /* will be set later on */
  };

  if (proto == ProtoEnum::ALL) {
    key.mask = 48;
    key.proto = 0;
  } else if (proto == ProtoEnum::TCP) {
    key.mask = 56;
    key.proto = (uint8_t)IPPROTO_TCP;
  } else if (proto == ProtoEnum::UDP) {
    key.mask = 56;
    key.proto = (uint8_t)IPPROTO_UDP;
  } else {
    return;  // TODO: is this case possible?
  }

  dp_rules.remove(key);
}
