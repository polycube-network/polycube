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

#include "RuleSnatEntry.h"
#include "Nat.h"

using namespace polycube::service;

RuleSnatEntry::RuleSnatEntry(RuleSnat &parent,
                             const RuleSnatEntryJsonObject &conf)
    : parent_(parent), id(conf.getId()) {
  logger()->info("Creating RuleSnatEntry instance: {0} -> {1}",
                 conf.getInternalNet(), conf.getExternalIp());
  update(conf);
}

RuleSnatEntry::~RuleSnatEntry() {}

void RuleSnatEntry::update(const RuleSnatEntryJsonObject &conf) {
  // This method updates all the object/parameter in RuleSnatEntry object
  // specified in the conf JsonObject.
  // You can modify this implementation.
  if (conf.internalNetIsSet()) {
    setInternalNet(conf.getInternalNet());
  }
  if (conf.externalIpIsSet()) {
    setExternalIp(conf.getExternalIp());
  }
}

RuleSnatEntryJsonObject RuleSnatEntry::toJsonObject() {
  RuleSnatEntryJsonObject conf;
  conf.setId(getId());
  conf.setInternalNet(getInternalNet());
  conf.setExternalIp(getExternalIp());
  return conf;
}

uint32_t RuleSnatEntry::getId() {
  // This method retrieves the id value.
  return id;
}

std::string RuleSnatEntry::getInternalNet() {
  // This method retrieves the internalNet value.
  struct IpAddr addr = {internalIp, internalNetmask};
  return addr.toString();
}

void RuleSnatEntry::setInternalNet(const std::string &value) {
  // This method set the internalNet value.
  struct IpAddr addr;
  addr.fromString(value);
  this->internalIp = addr.ip;
  this->internalNetmask = addr.netmask;
}

std::string RuleSnatEntry::getExternalIp() {
  // This method retrieves the externalIp value.
  struct IpAddr addr = {externalIp, 32};
  return addr.toString();
}

void RuleSnatEntry::setExternalIp(const std::string &value) {
  // This method set the externalIp value.
  struct IpAddr addr;
  addr.fromString(value);
  this->externalIp = addr.ip;
}

std::shared_ptr<spdlog::logger> RuleSnatEntry::logger() {
  return parent_.logger();
}

void RuleSnatEntry::injectToDatapath() {
  auto sm_rules = parent_.parent_.getParent().get_hash_table<sm_k, sm_v>(
      "sm_rules", 0, ProgramType::EGRESS);
  sm_k key{
      .internal_netmask_len = internalNetmask, .internal_ip = internalIp,
  };
  sm_v value{
      .external_ip = externalIp,
      .entry_type = (uint8_t)NattingTableOriginatingRuleEnum::SNAT,
  };

  sm_rules.set(key, value);
}

void RuleSnatEntry::removeFromDatapath() {
  auto sm_rules = parent_.parent_.getParent().get_hash_table<sm_k, sm_v>(
      "sm_rules", 0, ProgramType::EGRESS);
  sm_k key{
      .internal_netmask_len = internalNetmask, .internal_ip = internalIp,
  };

  sm_rules.remove(key);
}
