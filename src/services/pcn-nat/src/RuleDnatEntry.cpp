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
    : parent_(parent) {
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

void RuleDnatEntry::create(RuleDnat &parent, const uint32_t &id,
                           const RuleDnatEntryJsonObject &conf) {
  // This method creates the actual RuleDnatEntry object given thee key param.
  // Please remember to call here the create static method for all sub-objects
  // of RuleDnatEntry.

  auto newRule = new RuleDnatEntry(parent, conf);
  if (newRule == nullptr) {
    // Totally useless, but it is needed to avoid the compiler making wrong
    // assumptions and reordering
    throw std::runtime_error("I won't be thrown");
  }

  // Check for duplicates
  for (int i = 0; i < parent.rules_.size(); i++) {
    auto rule = parent.rules_[i];
    if (rule->getExternalIp() == newRule->getExternalIp()) {
      throw std::runtime_error("Cannot insert duplicate mapping");
    }
  }

  parent.rules_.resize(parent.rules_.size() + 1);
  parent.rules_[parent.rules_.size() - 1].reset(newRule);
  parent.rules_[parent.rules_.size() - 1]->id = id;

  // Inject rule in the datapath table
  newRule->injectToDatapath();
}

std::shared_ptr<RuleDnatEntry> RuleDnatEntry::getEntry(RuleDnat &parent,
                                                       const uint32_t &id) {
  // This method retrieves the pointer to RuleDnatEntry object specified by its
  // keys.
  for (int i = 0; i < parent.rules_.size(); i++) {
    if (parent.rules_[i]->id == id) {
      return parent.rules_[i];
    }
  }
  throw std::runtime_error("There is no rule " + id);
}

void RuleDnatEntry::removeEntry(RuleDnat &parent, const uint32_t &id) {
  // This method removes the single RuleDnatEntry object specified by its keys.
  // Remember to call here the remove static method for all-sub-objects of
  // RuleDnatEntry.
  if (parent.rules_.size() < id || !parent.rules_[id]) {
    throw std::runtime_error("There is no rule " + id);
  }
  for (int i = 0; i < parent.rules_.size(); i++) {
    if (parent.rules_[i]->id == id) {
      // Remove rule from data path
      parent.rules_[i]->removeFromDatapath();
      break;
    }
  }
  for (uint32_t i = id; i < parent.rules_.size() - 1; ++i) {
    parent.rules_[i] = parent.rules_[i + 1];
    parent.rules_[i]->id = i;
  }
  parent.rules_.resize(parent.rules_.size() - 1);
  parent.logger()->info("Removed DNAT entry {0}", id);
}

std::vector<std::shared_ptr<RuleDnatEntry>> RuleDnatEntry::get(
    RuleDnat &parent) {
  // This methods get the pointers to all the RuleDnatEntry objects in RuleDnat.
  std::vector<std::shared_ptr<RuleDnatEntry>> rules;
  for (auto it = parent.rules_.begin(); it != parent.rules_.end(); ++it) {
    if (*it) {
      rules.push_back(*it);
    }
  }
  return rules;
}

void RuleDnatEntry::remove(RuleDnat &parent) {
  // This method removes all RuleDnatEntry objects in RuleDnat.
  // Remember to call here the remove static method for all-sub-objects of
  // RuleDnatEntry.
  RuleDnat::removeEntry(parent.parent_);
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
