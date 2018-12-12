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


#include "RulePortForwarding.h"
#include "Nat.h"

using namespace polycube::service;

RulePortForwarding::RulePortForwarding(Rule &parent): parent_(parent) {}

RulePortForwarding::RulePortForwarding(Rule &parent, const RulePortForwardingJsonObject &conf): parent_(parent) {
  update(conf);
}

RulePortForwarding::~RulePortForwarding() { }

void RulePortForwarding::update(const RulePortForwardingJsonObject &conf) {
  // This method updates all the object/parameter in RulePortForwarding object specified in the conf JsonObject.
  // You can modify this implementation.
  if (conf.entryIsSet()) {
    for(auto &i : conf.getEntry()){
      auto id = i.getId();
      auto m = getEntry(id);
      m->update(i);
    }
  }
}

RulePortForwardingJsonObject RulePortForwarding::toJsonObject(){
  RulePortForwardingJsonObject conf;
  for(auto &i : getEntryList()){
    conf.addRulePortForwardingEntry(i->toJsonObject());
  }
  return conf;
}


void RulePortForwarding::create(Rule &parent, const RulePortForwardingJsonObject &conf){
  // This method creates the actual RulePortForwarding object given thee key param.
  // Please remember to call here the create static method for all sub-objects of RulePortForwarding.
  parent.portforwarding_ = std::make_shared<RulePortForwarding>(parent, conf);
}

std::shared_ptr<RulePortForwarding> RulePortForwarding::getEntry(Rule &parent){
  // This method retrieves the pointer to RulePortForwarding object specified by its keys.
  return parent.portforwarding_;
}

void RulePortForwarding::removeEntry(Rule &parent){
  // This method removes the single RulePortForwarding object specified by its keys.
  // Remember to call here the remove static method for all-sub-objects of RulePortForwarding.

  if (parent.portforwarding_->rules_.size() == 0) {
    // No rules to delete
    parent.logger()->info("No PortForwarding rules to remove");
    return;
  }

  for (int i = 0; i < parent.portforwarding_->rules_.size(); i++) {
    auto rule = parent.portforwarding_->rules_[i];
    rule->removeFromDatapath();
  }

  parent.portforwarding_->rules_.clear();

  parent.logger()->info("Removed all PortForwarding rules");
}

RulePortForwardingAppendOutputJsonObject RulePortForwarding::append(RulePortForwardingAppendInputJsonObject input) {
  RulePortForwardingEntryJsonObject conf;
  conf.setInternalIp(input.getInternalIp());
  conf.setInternalPort(input.getInternalPort());
  conf.setExternalIp(input.getExternalIp());
  conf.setExternalPort(input.getExternalPort());
  conf.setProto(input.getProto());
  uint32_t id = rules_.size();
  conf.setId(id);
  RulePortForwardingEntry::create(*this, id, conf);

  RulePortForwardingAppendOutputJsonObject output;
  output.setId(id);
  return output;
}

std::shared_ptr<spdlog::logger> RulePortForwarding::logger() {
  return parent_.logger();
}

