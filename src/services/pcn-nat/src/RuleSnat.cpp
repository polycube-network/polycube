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

RuleSnat::~RuleSnat() {
  delEntryList();
  logger()->info("Removed all SNAT rules");
}

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

RuleSnatAppendOutputJsonObject RuleSnat::append(
    RuleSnatAppendInputJsonObject input) {
  RuleSnatEntryJsonObject conf;
  conf.setExternalIp(input.getExternalIp());
  conf.setInternalNet(input.getInternalNet());
  uint32_t id = rules_.size();
  conf.setId(id);
  addEntry(id, conf);

  RuleSnatAppendOutputJsonObject output;
  output.setId(id);
  return output;
}

std::shared_ptr<spdlog::logger> RuleSnat::logger() {
  return parent_.logger();
}

std::shared_ptr<RuleSnatEntry> RuleSnat::getEntry(const uint32_t &id) {
  for (auto &it : rules_) {
    if (it->getId() == id) {
      return it;
    }
  }
  throw std::runtime_error("There is no rule " + id);
}

std::vector<std::shared_ptr<RuleSnatEntry>> RuleSnat::getEntryList() {
  return rules_;
}

void RuleSnat::addEntry(const uint32_t &id,
                        const RuleSnatEntryJsonObject &conf) {
  auto newRule = std::make_shared<RuleSnatEntry>(*this, conf);
  if (newRule == nullptr) {
    // Totally useless, but it is needed to avoid the compiler making wrong
    // assumptions and reordering
    throw std::runtime_error("I won't be thrown");
  }

  // Check for duplicates
  for (auto &it : rules_) {
    if (it->getInternalNet() == newRule->getInternalNet()) {
      throw std::runtime_error("Cannot insert duplicate mapping");
    }
  }

  rules_.push_back(newRule);

  // Inject rule in the datapath table
  newRule->injectToDatapath();
}

void RuleSnat::addEntryList(const std::vector<RuleSnatEntryJsonObject> &conf) {
  for (auto &i : conf) {
    uint32_t id_ = i.getId();
    addEntry(id_, i);
  }
}

void RuleSnat::replaceEntry(const uint32_t &id,
                            const RuleSnatEntryJsonObject &conf) {
  delEntry(id);
  uint32_t id_ = conf.getId();
  addEntry(id_, conf);
}

void RuleSnat::delEntry(const uint32_t &id) {
  if (rules_.size() < id) {
    throw std::runtime_error("There is no rule " + id);
  }
  for (auto &it : rules_) {
    if (it->getId() == id) {
      // Remove rule from data path
      it->removeFromDatapath();
      break;
    }
  }
  for (uint32_t i = id; i < rules_.size() - 1; ++i) {
    rules_[i] = rules_[i + 1];
    rules_[i]->id = i;
  }
  rules_.resize(rules_.size() - 1);
  logger()->info("Removed SNAT entry {0}", id);
}

void RuleSnat::delEntryList() {
  for (int i = 0; i < rules_.size(); i++) {
    rules_[i]->removeFromDatapath();
    rules_[i] = nullptr;
  }

  rules_.clear();
}