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

#include "RulePortForwarding.h"
#include "Nat.h"

using namespace polycube::service;

RulePortForwarding::RulePortForwarding(Rule &parent) : parent_(parent) {}

RulePortForwarding::RulePortForwarding(Rule &parent,
                                       const RulePortForwardingJsonObject &conf)
    : parent_(parent) {
  update(conf);
}

RulePortForwarding::~RulePortForwarding() {
  delEntryList();
  logger()->info("Removed all PortForwarding rules");
}

void RulePortForwarding::update(const RulePortForwardingJsonObject &conf) {
  // This method updates all the object/parameter in RulePortForwarding object
  // specified in the conf JsonObject.
  // You can modify this implementation.
  if (conf.entryIsSet()) {
    for (auto &i : conf.getEntry()) {
      auto id = i.getId();
      auto m = getEntry(id);
      m->update(i);
    }
  }
}

RulePortForwardingJsonObject RulePortForwarding::toJsonObject() {
  RulePortForwardingJsonObject conf;
  for (auto &i : getEntryList()) {
    conf.addRulePortForwardingEntry(i->toJsonObject());
  }
  return conf;
}

RulePortForwardingAppendOutputJsonObject RulePortForwarding::append(
    RulePortForwardingAppendInputJsonObject input) {
  RulePortForwardingEntryJsonObject conf;
  conf.setInternalIp(input.getInternalIp());
  conf.setInternalPort(input.getInternalPort());
  conf.setExternalIp(input.getExternalIp());
  conf.setExternalPort(input.getExternalPort());
  conf.setProto(input.getProto());
  uint32_t id = rules_.size();
  conf.setId(id);
  addEntry(id, conf);

  RulePortForwardingAppendOutputJsonObject output;
  output.setId(id);
  return output;
}

std::shared_ptr<spdlog::logger> RulePortForwarding::logger() {
  return parent_.logger();
}

std::shared_ptr<RulePortForwardingEntry> RulePortForwarding::getEntry(
    const uint32_t &id) {
  for (auto &it : rules_) {
    if (it->getId() == id) {
      return it;
    }
  }
  throw std::runtime_error("There is no rule " + id);
}

std::vector<std::shared_ptr<RulePortForwardingEntry>>
RulePortForwarding::getEntryList() {
  return rules_;
}

void RulePortForwarding::addEntry(
    const uint32_t &id, const RulePortForwardingEntryJsonObject &conf) {
  auto newRule = std::make_shared<RulePortForwardingEntry>(*this, conf);
  if (newRule == nullptr) {
    // Totally useless, but it is needed to avoid the compiler making wrong
    // assumptions and reordering
    throw std::runtime_error("I won't be thrown");
  }

  // Check for duplicates
  // TODO: check for id?
  for (auto &it : rules_) {
    if (it->getExternalIp() == newRule->getExternalIp() &&
        it->getExternalPort() == newRule->getExternalPort() &&
        it->getProto() == newRule->getProto()) {
      throw std::runtime_error("Cannot insert duplicate mapping");
    }
  }

  rules_.push_back(newRule);

  // Inject rule in the datapath table
  newRule->injectToDatapath();
}

void RulePortForwarding::addEntryList(
    const std::vector<RulePortForwardingEntryJsonObject> &conf) {
  for (auto &i : conf) {
    uint32_t id_ = i.getId();
    addEntry(id_, i);
  }
}

void RulePortForwarding::replaceEntry(
    const uint32_t &id, const RulePortForwardingEntryJsonObject &conf) {
  delEntry(id);
  uint32_t id_ = conf.getId();
  addEntry(id_, conf);
}

void RulePortForwarding::delEntry(const uint32_t &id) {
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
  logger()->info("Removed PortForwarding entry {0}", id);
}

void RulePortForwarding::delEntryList() {
  for (int i = 0; i < rules_.size(); i++) {
    rules_[i]->removeFromDatapath();
    rules_[i] = nullptr;
  }

  rules_.clear();
}