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
  snat_ = std::make_shared<RuleSnat>(*this);
  dnat_ = std::make_shared<RuleDnat>(*this);
  portforwarding_ = std::make_shared<RulePortForwarding>(*this);
  masquerade_ = std::make_shared<RuleMasquerade>(*this);
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

std::shared_ptr<spdlog::logger> Rule::logger() {
  return parent_.logger();
}

Nat &Rule::getParent() {
  return parent_;
}

std::shared_ptr<RuleSnat> Rule::getSnat() {
  return snat_;
}

void Rule::addSnat(const RuleSnatJsonObject &value) {
  snat_ = std::make_shared<RuleSnat>(*this, value);
}

void Rule::replaceSnat(const RuleSnatJsonObject &conf) {
  delSnat();
  addSnat(conf);
}

void Rule::delSnat() {
  snat_->delEntryList();
}

std::shared_ptr<RuleMasquerade> Rule::getMasquerade() {
  return masquerade_;
}

void Rule::addMasquerade(const RuleMasqueradeJsonObject &value) {
  masquerade_ = std::make_shared<RuleMasquerade>(*this, value);
}

void Rule::replaceMasquerade(const RuleMasqueradeJsonObject &conf) {
  delMasquerade();
  addMasquerade(conf);
}

void Rule::delMasquerade() {
  // Treat deletion as disabling
  masquerade_->disable();
}

std::shared_ptr<RuleDnat> Rule::getDnat() {
  return dnat_;
}

void Rule::addDnat(const RuleDnatJsonObject &value) {
  dnat_ = std::make_shared<RuleDnat>(*this, value);
}

void Rule::replaceDnat(const RuleDnatJsonObject &conf) {
  delDnat();
  addDnat(conf);
}

void Rule::delDnat() {
  dnat_->delEntryList();
}

std::shared_ptr<RulePortForwarding> Rule::getPortForwarding() {
  return portforwarding_;
}

void Rule::addPortForwarding(const RulePortForwardingJsonObject &value) {
  portforwarding_ = std::make_shared<RulePortForwarding>(*this, value);
}

void Rule::replacePortForwarding(const RulePortForwardingJsonObject &conf) {
  delPortForwarding();
  addPortForwarding(conf);
}

void Rule::delPortForwarding() {
  portforwarding_->delEntryList();
}