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


#include "RuleDnat.h"
#include "Nat.h"

using namespace polycube::service;

RuleDnat::RuleDnat(Rule &parent) : parent_(parent) {}

RuleDnat::RuleDnat(Rule &parent, const RuleDnatJsonObject &conf): parent_(parent) {
  update(conf);
}

RuleDnat::~RuleDnat() { }

void RuleDnat::update(const RuleDnatJsonObject &conf) {
  // This method updates all the object/parameter in RuleDnat object specified in the conf JsonObject.
  // You can modify this implementation.
  if (conf.entryIsSet()) {
    for(auto &i : conf.getEntry()){
      auto id = i.getId();
      auto m = getEntry(id);
      m->update(i);
    }
  }
}

RuleDnatJsonObject RuleDnat::toJsonObject(){
  RuleDnatJsonObject conf;
  for(auto &i : getEntryList()){
    conf.addRuleDnatEntry(i->toJsonObject());
  }
  return conf;
}


void RuleDnat::create(Rule &parent, const RuleDnatJsonObject &conf){
  // This method creates the actual RuleDnat object given thee key param.
  // Please remember to call here the create static method for all sub-objects of RuleDnat.
  parent.dnat_ = std::make_shared<RuleDnat>(parent, conf);
}

std::shared_ptr<RuleDnat> RuleDnat::getEntry(Rule &parent){
  // This method retrieves the pointer to RuleDnat object specified by its keys.
  return parent.dnat_;
}

void RuleDnat::removeEntry(Rule &parent){
  // This method removes the single RuleDnat object specified by its keys.
  // Remember to call here the remove static method for all-sub-objects of RuleDnat.

  if (parent.dnat_->rules_.size() == 0) {
    // No rules to delete
    parent.logger()->info("No DNAT rules to remove");
    return;
  }

  for (int i = 0; i < parent.dnat_->rules_.size(); i++) {
    auto rule = parent.dnat_->rules_[i];
    rule->removeFromDatapath();
  }

  parent.dnat_->rules_.clear();

  parent.logger()->info("Removed all DNAT rules");
}

RuleDnatAppendOutputJsonObject RuleDnat::append(RuleDnatAppendInputJsonObject input) {
  RuleDnatEntryJsonObject conf;
  conf.setExternalIp(input.getExternalIp());
  conf.setInternalIp(input.getInternalIp());
  uint32_t id = rules_.size();
  conf.setId(id);
  RuleDnatEntry::create(*this, id, conf);

  RuleDnatAppendOutputJsonObject output;
  output.setId(id);
  return output;
}

std::shared_ptr<spdlog::logger> RuleDnat::logger() {
  return parent_.logger();
}

