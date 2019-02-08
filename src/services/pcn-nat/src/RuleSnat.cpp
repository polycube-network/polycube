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

#include "RuleSnat.h"
#include "Nat.h"

RuleSnat::RuleSnat(Rule &parent) : parent_(parent) {}

RuleSnat::RuleSnat(Rule &parent, const RuleSnatJsonObject &conf)
    : parent_(parent) {
  update(conf);
}

RuleSnat::~RuleSnat() {}

void RuleSnat::update(const RuleSnatJsonObject &conf) {
  // This method updates all the object/parameter in RuleSnat object specified
  // in the conf JsonObject.
  // You can modify this implementation.
  if (conf.entryIsSet()) {
    for (auto &i : conf.getEntry()) {
      auto id = i.getId();
      auto m = getEntry(id);
      m->update(i);
    }
  }
}

RuleSnatJsonObject RuleSnat::toJsonObject() {
  RuleSnatJsonObject conf;
  for (auto &i : getEntryList()) {
    conf.addRuleSnatEntry(i->toJsonObject());
  }
  return conf;
}

void RuleSnat::create(Rule &parent, const RuleSnatJsonObject &conf) {
  // This method creates the actual RuleSnat object given thee key param.
  // Please remember to call here the create static method for all sub-objects
  // of RuleSnat.
  parent.snat_ = std::make_shared<RuleSnat>(parent, conf);
}

std::shared_ptr<RuleSnat> RuleSnat::getEntry(Rule &parent) {
  // This method retrieves the pointer to RuleSnat object specified by its keys.
  return parent.snat_;
}

void RuleSnat::removeEntry(Rule &parent) {
  // This method removes the single RuleSnat object specified by its keys.
  // Remember to call here the remove static method for all-sub-objects of
  // RuleSnat.

  if (parent.snat_->rules_.size() == 0) {
    // No rules to delete
    parent.logger()->info("No SNAT rules to remove");
    return;
  }

  for (int i = 0; i < parent.snat_->rules_.size(); i++) {
    auto rule = parent.snat_->rules_[i];
    rule->removeFromDatapath();
  }

  parent.snat_->rules_.clear();

  parent.logger()->info("Removed all SNAT rules");
}

RuleSnatAppendOutputJsonObject RuleSnat::append(
    RuleSnatAppendInputJsonObject input) {
  RuleSnatEntryJsonObject conf;
  conf.setExternalIp(input.getExternalIp());
  conf.setInternalNet(input.getInternalNet());
  uint32_t id = rules_.size();
  conf.setId(id);
  RuleSnatEntry::create(*this, id, conf);

  RuleSnatAppendOutputJsonObject output;
  output.setId(id);
  return output;
}

std::shared_ptr<spdlog::logger> RuleSnat::logger() {
  return parent_.logger();
}
