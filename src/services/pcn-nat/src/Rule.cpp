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

#include "Rule.h"
#include "Nat.h"

Rule::Rule(Nat &parent, const RuleJsonObject &conf) : parent_(parent) {
  update(conf);
}

Rule::~Rule() {}

void Rule::update(const RuleJsonObject &conf) {
  // This method updates all the object/parameter in Rule object specified in
  // the conf JsonObject.
  // You can modify this implementation.
  if (conf.snatIsSet()) {
    auto m = getSnat();
    m->update(conf.getSnat());
  }
  if (conf.masqueradeIsSet()) {
    auto m = getMasquerade();
    m->update(conf.getMasquerade());
  }
  if (conf.dnatIsSet()) {
    auto m = getDnat();
    m->update(conf.getDnat());
  }
  if (conf.portForwardingIsSet()) {
    auto m = getPortForwarding();
    m->update(conf.getPortForwarding());
  }
}

RuleJsonObject Rule::toJsonObject() {
  RuleJsonObject conf;
  conf.setSnat(getSnat()->toJsonObject());
  conf.setMasquerade(getMasquerade()->toJsonObject());
  conf.setDnat(getDnat()->toJsonObject());
  conf.setPortForwarding(getPortForwarding()->toJsonObject());
  return conf;
}

void Rule::create(Nat &parent, const RuleJsonObject &conf) {
  // This method creates the actual Rule object given thee key param.
  // Please remember to call here the create static method for all sub-objects
  // of Rule.
  parent.rule_ = std::make_shared<Rule>(parent, conf);
  parent.rule_->snat_ = std::make_shared<RuleSnat>(*parent.rule_.get());
  parent.rule_->dnat_ = std::make_shared<RuleDnat>(*parent.rule_.get());
  parent.rule_->portforwarding_ =
      std::make_shared<RulePortForwarding>(*parent.rule_.get());
  parent.rule_->masquerade_ =
      std::make_shared<RuleMasquerade>(*parent.rule_.get());
}

std::shared_ptr<Rule> Rule::getEntry(Nat &parent) {
  // This method retrieves the pointer to Rule object specified by its keys.
  return parent.rule_;
}

void Rule::removeEntry(Nat &parent) {
  // This method removes the single Rule object specified by its keys.
  // Remember to call here the remove static method for all-sub-objects of Rule.
  parent.rule_->delSnat();
  parent.rule_->delDnat();
  parent.rule_->delPortForwarding();
  parent.rule_->delMasquerade();
}

std::shared_ptr<spdlog::logger> Rule::logger() {
  return parent_.logger();
}

Nat &Rule::getParent() {
  return parent_;
}
