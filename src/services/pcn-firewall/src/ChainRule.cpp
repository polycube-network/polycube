/*
 * Copyright 2017 The Polycube Authors
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

#include "Chain.h"
#include "ChainRule.h"
#include "Firewall.h"

ChainRule::ChainRule(Chain &parent, const ChainRuleJsonObject &conf)
    : parent_(parent) {
  update(conf);
}

ChainRule::~ChainRule() {}

void ChainRule::update(const ChainRuleJsonObject &conf) {
  if (conf.conntrackIsSet()) {
    if (!parent_.parent_.isContrackActive()) {
      throw new std::runtime_error(
          "Please enable the connection tracking module.");
    }
    this->conntrack = conf.getConntrack();
    conntrackIsSet = true;
  }
  if (conf.srcIsSet()) {
    this->ipSrc.fromString(conf.getSrc());
    ipSrcIsSet = true;
  }
  if (conf.dstIsSet()) {
    this->ipDst.fromString(conf.getDst());
    ipDstIsSet = true;
  }
  if (conf.sportIsSet()) {
    this->srcPort = conf.getSport();
    srcPortIsSet = true;
  }
  if (conf.dportIsSet()) {
    this->dstPort = conf.getDport();
    dstPortIsSet = true;
  }
  if (conf.tcpflagsIsSet()) {
    flags_from_string_to_masks(conf.getTcpflags(), flagsSet, flagsNotSet);
    tcpFlagsIsSet = true;
  }
  if (conf.l4protoIsSet()) {
    l4Proto = protocol_from_string_to_int(conf.getL4proto());
    l4ProtoIsSet = true;
  }

  if (conf.descriptionIsSet()) {
    description = conf.getDescription();
    descriptionIsSet = true;
  }

  if (conf.actionIsSet()) {
    this->action = conf.getAction();
    actionIsSet = true;
  } else {
    this->action = ActionEnum::DROP;
    actionIsSet = true;
  }
}

ChainRuleJsonObject ChainRule::toJsonObject() {
  ChainRuleJsonObject conf;
  try {
    conf.setSrc(getSrc());
  } catch (...) {
  }
  try {
    conf.setDst(getDst());
  } catch (...) {
  }
  try {
    conf.setDport(getDport());
  } catch (...) {
  }
  try {
    conf.setTcpflags(getTcpflags());
  } catch (...) {
  }
  try {
    conf.setDescription(getDescription());
  } catch (...) {
  }

  try {
    conf.setConntrack(getConntrack());
  } catch (...) {
  }
  try {
    conf.setL4proto(getL4proto());
  } catch (...) {
  }
  try {
    conf.setAction(getAction());
  } catch (...) {
  }
  try {
    conf.setSport(getSport());
  } catch (...) {
  }
  try {
    conf.setId(getId());
  } catch (...) {
  }
  return conf;
}

void ChainRule::create(Chain &parent, const uint32_t &id,
                       const ChainRuleJsonObject &conf) {
  // This method creates the actual ChainRule object given thee key param.
  auto newRule = new ChainRule(parent, conf);

  // Forcing counters update
  ChainStats::get(parent);

  if (newRule == nullptr) {
    // Totally useless, but it is needed to avoid the compiler making wrong
    // assumptions and reordering
    throw new std::runtime_error("I won't be thrown");

  } else if (parent.rules_.size() <= id && newRule != nullptr) {
    parent.rules_.resize(id + 1);
  }
  if (parent.rules_[id]) {
    parent.logger()->info("Rule {0} overwritten!", id);
  }

  parent.rules_[id].reset(newRule);
  parent.rules_[id]->id = id;

  if (parent.parent_.interactive_) {
    parent.updateChain();
  }
}

std::shared_ptr<ChainRule> ChainRule::getEntry(Chain &parent,
                                               const uint32_t &id) {
  // This method retrieves the pointer to ChainRule object specified by its
  // keys.
  if (parent.rules_.size() < id || !parent.rules_[id]) {
    throw std::runtime_error("There is no rule " + id);
  }
  return parent.rules_[id];
}

void ChainRule::removeEntry(Chain &parent, const uint32_t &id) {
  // This method removes the single ChainRule object specified by its keys.
  if (parent.rules_.size() < id || !parent.rules_[id]) {
    throw std::runtime_error("There is no rule " + id);
  }

  // Forcing counters update
  ChainStats::get(parent);

  for (uint32_t i = id; i < parent.rules_.size() - 1; ++i) {
    parent.rules_[i] = parent.rules_[i + 1];
    parent.rules_[i]->id = i;
  }

  parent.rules_.resize(parent.rules_.size() - 1);

  for (uint32_t i = id; i < parent.counters_.size() - 1; ++i) {
    parent.counters_[i] = parent.counters_[i + 1];
    parent.counters_[i]->counter.setId(i);
  }
  parent.rules_.resize(parent.counters_.size() - 1);

  if (parent.parent_.interactive_) {
    parent.applyRules();
  }
}

std::vector<std::shared_ptr<ChainRule>> ChainRule::get(Chain &parent) {
  // This methods get the pointers to all the ChainRule objects in Chain.
  std::vector<std::shared_ptr<ChainRule>> rules;
  for (auto it = parent.rules_.begin(); it != parent.rules_.end(); ++it) {
    if (*it) {
      rules.push_back(*it);
    }
  }
  return rules;
}

void ChainRule::remove(Chain &parent) {
  // This method removes all ChainRule objects in Chain.
  parent.rules_.clear();
  parent.counters_.clear();
  if (parent.parent_.interactive_) {
    parent.applyRules();
  }
}

std::string ChainRule::getDescription() {
  if (!descriptionIsSet) {
    throw std::runtime_error("Description not set.");
  }
  return this->description;
}

std::string ChainRule::getSrc() {
  // This method retrieves the src value.
  if (!ipSrcIsSet) {
    throw std::runtime_error("Src not set.");
  }
  return this->ipSrc.toString();
}

std::string ChainRule::getDst() {
  // This method retrieves the dst value.
  if (!ipDstIsSet) {
    throw std::runtime_error("Dst not set.");
  }
  return this->ipDst.toString();
}

uint16_t ChainRule::getDport() {
  if (!dstPortIsSet) {
    throw std::runtime_error("DPort not set.");
  }
  return this->dstPort;
}

std::string ChainRule::getTcpflags() {
  if (!tcpFlagsIsSet) {
    throw std::runtime_error("TcpFlags not set.");
  }
  std::string flags = "";
  flags_from_masks_to_string(flags, this->flagsSet, this->flagsNotSet);
  return flags;
}

ConntrackstatusEnum ChainRule::getConntrack() {
  if (!conntrackIsSet) {
    throw std::runtime_error("Conntrack not set.");
  }
  return this->conntrack;
}

std::string ChainRule::getL4proto() {
  // This method retrieves the l4proto value.
  if (!l4ProtoIsSet) {
    throw std::runtime_error("L4Proto not set.");
  }
  return protocol_from_int_to_string(this->l4Proto);
}

ActionEnum ChainRule::getAction() {
  // This method retrieves the action value.
  if (!actionIsSet) {
    throw std::runtime_error("Action not set.");
  }
  return this->action;
}

uint16_t ChainRule::getSport() {
  // This method retrieves the sport value.
  if (!srcPortIsSet) {
    throw std::runtime_error("SPort not set.");
  }
  return this->srcPort;
}

uint32_t ChainRule::getId() {
  // This method retrieves the id value.
  return id;
}

std::shared_ptr<spdlog::logger> ChainRule::logger() {
  return parent_.logger();
}
