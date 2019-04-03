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

#include "RuleDnatEntry.h"
#include "Nat.h"

using namespace polycube::service;

RuleDnatEntry::RuleDnatEntry(RuleDnat &parent,
                             const RuleDnatEntryJsonObject &conf)
    : parent_(parent), id(conf.getId()) {
  logger()->info("Creating RuleDnatEntry instance: {0} -> {1}",
                 conf.getExternalIp(), conf.getInternalIp());
  update(conf);
}

RuleDnatEntry::~RuleDnatEntry() {}

void RuleDnatEntry::update(const RuleDnatEntryJsonObject &conf) {
  // This method updates all the object/parameter in RuleDnatEntry object
  // specified in the conf JsonObject.
  // You can modify this implementation.
  if (conf.externalIpIsSet()) {
    setExternalIp(conf.getExternalIp());
  }
  if (conf.internalIpIsSet()) {
    setInternalIp(conf.getInternalIp());
  }
}

RuleDnatEntryJsonObject RuleDnatEntry::toJsonObject() {
  RuleDnatEntryJsonObject conf;
  conf.setId(getId());
  conf.setExternalIp(getExternalIp());
  conf.setInternalIp(getInternalIp());
  return conf;
}

uint32_t RuleDnatEntry::getId() {
  // This method retrieves the id value.
  return id;
}

std::string RuleDnatEntry::getExternalIp() {
  // This method retrieves the externalIp value.
  struct IpAddr addr = {externalIp, 32};
  return addr.toString();
}

void RuleDnatEntry::setExternalIp(const std::string &value) {
  // This method set the externalIp value.
  struct IpAddr addr;
  addr.fromString(value);
  this->externalIp = addr.ip;
}

std::string RuleDnatEntry::getInternalIp() {
  // This method retrieves the internalIp value.
  struct IpAddr addr = {internalIp, 32};
  return addr.toString();
}

void RuleDnatEntry::setInternalIp(const std::string &value) {
  // This method set the internalIp value.
  struct IpAddr addr;
  addr.fromString(value);
  this->internalIp = addr.ip;
}

std::shared_ptr<spdlog::logger> RuleDnatEntry::logger() {
  return parent_.logger();
}

void RuleDnatEntry::injectToDatapath() {
  auto dp_rules = parent_.parent_.getParent().get_hash_table<dp_k, dp_v>(
      "dp_rules", 0, ProgramType::INGRESS);

  dp_k key{
      .mask = 32, .external_ip = externalIp, .external_port = 0, .proto = 0,
  };

  dp_v value{
      .internal_ip = internalIp,
      .internal_port = 0,
      .entry_type = (uint8_t)NattingTableOriginatingRuleEnum::DNAT,
  };

  dp_rules.set(key, value);
}

void RuleDnatEntry::removeFromDatapath() {
  auto dp_rules = parent_.parent_.getParent().get_hash_table<dp_k, dp_v>(
      "dp_rules", 0, ProgramType::INGRESS);

  dp_k key{
      .mask = 32, .external_ip = externalIp, .external_port = 0, .proto = 0,
  };

  dp_rules.remove(key);
}
