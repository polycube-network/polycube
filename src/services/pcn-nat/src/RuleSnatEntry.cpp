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

#include "RuleSnatEntry.h"
#include "Nat.h"

using namespace polycube::service;

RuleSnatEntry::RuleSnatEntry(RuleSnat &parent, const RuleSnatEntryJsonObject &conf): parent_(parent) {
  logger()->info("Creating RuleSnatEntry instance: {0} -> {1}", conf.getInternalNet(), conf.getExternalIp());
  update(conf);
}

RuleSnatEntry::~RuleSnatEntry() { }

void RuleSnatEntry::update(const RuleSnatEntryJsonObject &conf) {
  // This method updates all the object/parameter in RuleSnatEntry object specified in the conf JsonObject.
  // You can modify this implementation.
  if (conf.internalNetIsSet()) {
    setInternalNet(conf.getInternalNet());
  }
  if (conf.externalIpIsSet()) {
    setExternalIp(conf.getExternalIp());
  }
}

RuleSnatEntryJsonObject RuleSnatEntry::toJsonObject(){
  RuleSnatEntryJsonObject conf;
  conf.setId(getId());
  conf.setInternalNet(getInternalNet());
  conf.setExternalIp(getExternalIp());
  return conf;
}


void RuleSnatEntry::create(RuleSnat &parent, const uint32_t &id, const RuleSnatEntryJsonObject &conf){
  // This method creates the actual RuleSnatEntry object given thee key param.
  // Please remember to call here the create static method for all sub-objects of RuleSnatEntry.

  auto newRule = new RuleSnatEntry(parent, conf);
  if (newRule == nullptr) {
    //Totally useless, but it is needed to avoid the compiler making wrong assumptions and reordering
    throw std::runtime_error("I won't be thrown");
  }

  // Check for duplicates
  for (int i = 0; i < parent.rules_.size(); i++) {
    auto rule = parent.rules_[i];
    if (rule->getInternalNet() == newRule->getInternalNet()) {
      throw std::runtime_error("Cannot insert duplicate mapping");
    }
  }

  parent.rules_.resize(parent.rules_.size() + 1);
  parent.rules_[parent.rules_.size() - 1].reset(newRule);
  parent.rules_[parent.rules_.size() - 1]->id = id;

  // Inject rule in the datapath table
  newRule->injectToDatapath();
}

std::shared_ptr<RuleSnatEntry> RuleSnatEntry::getEntry(RuleSnat &parent, const uint32_t &id){
  // This method retrieves the pointer to NatRule object specified by its keys.
  for (int i = 0; i < parent.rules_.size(); i++) {
    if (parent.rules_[i]->id == id) {
      return parent.rules_[i];
    }
  }
  throw std::runtime_error("There is no rule " + id);
}

void RuleSnatEntry::removeEntry(RuleSnat &parent, const uint32_t &id){
  // This method removes the single RuleSnatEntry object specified by its keys.
  // Remember to call here the remove static method for all-sub-objects of RuleSnatEntry.
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
  parent.logger()->info("Removed SNAT entry {0}", id);
}

std::vector<std::shared_ptr<RuleSnatEntry>> RuleSnatEntry::get(RuleSnat &parent){
  // This methods get the pointers to all the NatRule objects in Nat.
  std::vector<std::shared_ptr<RuleSnatEntry>> rules;
  for (auto it = parent.rules_.begin(); it != parent.rules_.end(); ++it) {
    if (*it) {
      rules.push_back(*it);
    }
  }
  return rules;
}

void RuleSnatEntry::remove(RuleSnat &parent){
  // This method removes all RuleSnatEntry objects in RuleSnat.
  // Remember to call here the remove static method for all-sub-objects of RuleSnatEntry.
  RuleSnat::removeEntry(parent.parent_);
}

uint32_t RuleSnatEntry::getId(){
  // This method retrieves the id value.
  return id;
}


std::string RuleSnatEntry::getInternalNet(){
  // This method retrieves the internalNet value.
  struct IpAddr addr = {internalIp, internalNetmask};
  return addr.toString();
}

void RuleSnatEntry::setInternalNet(const std::string &value){
  // This method set the internalNet value.
  struct IpAddr addr;
  addr.fromString(value);
  this->internalIp = addr.ip;
  this->internalNetmask = addr.netmask;
}

std::string RuleSnatEntry::getExternalIp(){
  // This method retrieves the externalIp value.
  struct IpAddr addr = {externalIp, 32};
  return addr.toString();
}

void RuleSnatEntry::setExternalIp(const std::string &value){
  // This method set the externalIp value.
  struct IpAddr addr;
  addr.fromString(value);
  this->externalIp = addr.ip;
}

std::shared_ptr<spdlog::logger> RuleSnatEntry::logger() {
  return parent_.logger();
}

void RuleSnatEntry::injectToDatapath() {
  auto sm_rules = parent_.parent_.getParent().get_hash_table<sm_k, sm_v>("sm_rules");
  sm_k key {
    .internal_netmask_len = internalNetmask,
    .internal_ip = internalIp,
  };
  sm_v value {
    .external_ip = externalIp,
    .entry_type = (uint8_t)NattingTableOriginatingRuleEnum::SNAT,
  };

  sm_rules.set(key, value);
}

void RuleSnatEntry::removeFromDatapath() {
  auto sm_rules = parent_.parent_.getParent().get_hash_table<sm_k, sm_v>("sm_rules");
  sm_k key {
    .internal_netmask_len = internalNetmask,
    .internal_ip = internalIp,
  };

  sm_rules.remove(key);
}
