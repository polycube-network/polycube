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

#include "RuleDnat.h"
#include "Nat.h"

using namespace polycube::service;

RuleDnat::RuleDnat(Rule &parent) : parent_(parent) {}

RuleDnat::RuleDnat(Rule &parent, const RuleDnatJsonObject &conf)
    : parent_(parent) {
  update(conf);
}

RuleDnat::~RuleDnat() {
  delEntryList();
}

void RuleDnat::update(const RuleDnatJsonObject &conf) {
  // This method updates all the object/parameter in RuleDnat object specified
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

RuleDnatJsonObject RuleDnat::toJsonObject() {
  RuleDnatJsonObject conf;
  for (auto &i : getEntryList()) {
    conf.addRuleDnatEntry(i->toJsonObject());
  }
  return conf;
}

RuleDnatAppendOutputJsonObject RuleDnat::append(
    RuleDnatAppendInputJsonObject input) {
  RuleDnatEntryJsonObject conf;
  conf.setExternalIp(input.getExternalIp());
  conf.setInternalIp(input.getInternalIp());
  uint32_t id = rules_.size();
  conf.setId(id);
  addEntry(id, conf);

  RuleDnatAppendOutputJsonObject output;
  output.setId(id);
  return output;
}

std::shared_ptr<spdlog::logger> RuleDnat::logger() {
  return parent_.logger();
}

std::shared_ptr<RuleDnatEntry> RuleDnat::getEntry(const uint32_t &id) {
  for (auto &it : rules_) {
    if (it->getId() == id) {
      return it;
    }
  }
  throw std::runtime_error("There is no rule " + id);
}

std::vector<std::shared_ptr<RuleDnatEntry>> RuleDnat::getEntryList() {
  return rules_;
}

void RuleDnat::addEntry(const uint32_t &id,
                        const RuleDnatEntryJsonObject &conf) {
  auto newRule = std::make_shared<RuleDnatEntry>(*this, conf);
  if (newRule == nullptr) {
    // Totally useless, but it is needed to avoid the compiler making wrong
    // assumptions and reordering
    throw std::runtime_error("I won't be thrown");
  }

  // Check for duplicates
  // TODO: should id also be checked?
  for (auto &it : rules_) {
    if (it->getExternalIp() == newRule->getExternalIp()) {
      throw std::runtime_error("Cannot insert duplicate mapping");
    }
  }

  rules_.push_back(
      newRule);  // cannot use std::move() because rule is used below

  // Inject rule in the datapath table
  newRule->injectToDatapath();
}

void RuleDnat::addEntryList(const std::vector<RuleDnatEntryJsonObject> &conf) {
  for (auto &i : conf) {
    uint32_t id_ = i.getId();
    addEntry(id_, i);
  }
}

void RuleDnat::replaceEntry(const uint32_t &id,
                            const RuleDnatEntryJsonObject &conf) {
  // TODO: what about calling the update() method on that rule?
  delEntry(id);
  uint32_t id_ = conf.getId();
  addEntry(id_, conf);
}

void RuleDnat::delEntry(const uint32_t &id) {
  if (rules_.size() < id) {
    throw std::runtime_error("There is no rule " + id);
  }
  for (auto &it : rules_) {
    if (it->getId() == id) {
      it->removeFromDatapath();
      break;
    }
  }
  // reallocate vector
  for (uint32_t i = id; i < rules_.size() - 1; ++i) {
    rules_[i] = rules_[i + 1];
    // TODO: should the id be changed?
    rules_[i]->id = i;
  }
  rules_.resize(rules_.size() - 1);
  logger()->info("Removed DNAT entry {0}", id);
}

void RuleDnat::delEntryList() {
  for (int i = 0; i < rules_.size(); i++) {
    rules_[i]->removeFromDatapath();
    rules_[i] = nullptr;
  }

  rules_.clear();

  logger()->info("Removed all DNAT rules");
}