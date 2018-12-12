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

#include "Chain.h"
#include "ChainRule.h"
#include "Iptables.h"

ChainRule::ChainRule(Chain &parent, const ChainRuleJsonObject &conf)
    : parent_(parent) {
  logger()->trace("Creating ChainRule instance");
  update(conf);
}

ChainRule::~ChainRule() {}

void ChainRule::update(const ChainRuleJsonObject &conf) {
  // This method updates all the object/parameter in ChainRule object specified
  // in the conf JsonObject.
  // You can modify this implementation.

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
    flagsFromStringToMasks(conf.getTcpflags(), flagsSet, flagsNotSet);
    tcpFlagsIsSet = true;
  }
  if (conf.l4protoIsSet()) {
    l4Proto = protocolFromStringToInt(conf.getL4proto());
    l4ProtoIsSet = true;
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
  try {
    conf.setOutIface(getOutIface());
  } catch (...) {
  }
  try {
    conf.setInIface(getInIface());
  } catch (...) {
  }
  try {
    conf.setConntrack(getConntrack());
  } catch (...) {
  }

  return conf;
}

bool ChainRule::equal(ChainRule &cmp) {
  if (ipSrcIsSet != cmp.ipSrcIsSet)
    return false;
  if (ipSrcIsSet) {
    if (ipSrc.toString() != cmp.ipSrc.toString())
      return false;
  }

  if (ipDstIsSet != cmp.ipDstIsSet)
    return false;
  if (ipDstIsSet) {
    if (ipDst.toString() != cmp.ipDst.toString())
      return false;
  }

  if (srcPortIsSet != cmp.srcPortIsSet)
    return false;
  if (srcPortIsSet) {
    if (srcPort != cmp.srcPort)
      return false;
  }

  if (dstPortIsSet != cmp.dstPortIsSet)
    return false;
  if (dstPortIsSet) {
    if (dstPort != cmp.dstPort)
      return false;
  }

  if (l4ProtoIsSet != cmp.l4ProtoIsSet)
    return false;
  if (l4ProtoIsSet) {
    if (l4Proto != cmp.l4Proto)
      return false;
  }

  if (tcpFlagsIsSet != cmp.tcpFlagsIsSet)
    return false;
  if (tcpFlagsIsSet) {
    if ((flagsSet != cmp.flagsSet) || ((flagsNotSet != cmp.flagsNotSet)))
      return false;
  }

  if (actionIsSet != cmp.actionIsSet)
    return false;

  if (actionIsSet) {
    if (action != cmp.action)
      return false;
  }

  if (conntrackIsSet != cmp.conntrackIsSet)
    return false;
  if (conntrackIsSet) {
    if (conntrack != cmp.conntrack)
      return false;
  }

  return true;
}

void ChainRule::create(Chain &parent, const uint32_t &id,
                       const ChainRuleJsonObject &conf) {
  // This method creates the actual ChainRule object given thee key param.

  // Create new ChainRule Object
  auto newRule = new ChainRule(parent, conf);

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

// Now, rule is inserted, new rules are moved.
// when rule are moved, update counters
void ChainRule::insert(Chain &parent, const uint32_t &id,
                       const ChainRuleJsonObject &conf) {
  auto newRule = new ChainRule(parent, conf);

  ChainStatsJsonObject confStats;
  auto newStats = new ChainStats(parent, confStats);

  ChainStats::get(parent);

  if (newRule == nullptr) {
    // Totally useless, but it is needed to avoid the compiler making wrong
    // assumptions and reordering
    throw new std::runtime_error("I won't be thrown");

  } else if (parent.rules_.size() >= id && newRule != nullptr) {
    parent.rules_.resize(parent.rules_.size() + 1);
    parent.counters_.resize(parent.counters_.size() + 1);
  }

  // 0, 1, 2, 3
  // insert @2
  // 0, 1, 2*, 2->3, 3->4

  // for rules before id
  // nothing

  // for rules starting from id to rules size-1
  // move ahead i -> i+i
  // btw, better to start from the end of the array
  // for rules starting from size-1 to id
  // move ahead i -> i+i

  int i = 0;
  int id_int = (int)id;

  // ids are 0,1,2
  // size=3
  // id = 1 (insert)

  // new size = 4

  // from 1 to 2
  // move 2->3
  // move 1->2,
  // replace 1

  for (i = parent.rules_.size() - 2; i >= id_int; i--) {
    parent.rules_[i + 1] = parent.rules_[i];
    parent.counters_[i + 1] = parent.counters_[i];
    if (parent.rules_[i + 1] != nullptr) {
      parent.rules_[i + 1]->id = i + 1;
    }
    if (parent.counters_[i + 1] != nullptr) {
      parent.counters_[i + 1]->counter.setId(i + 1);
    }
  }

  parent.rules_[id].reset(newRule);
  parent.rules_[id]->id = id;

  parent.counters_[id].reset(newStats);
  parent.counters_[id]->counter.setPkts(0);
  parent.counters_[id]->counter.setBytes(0);
  parent.counters_[id]->counter.setId(id);

  if (parent.parent_.interactive_) {
    parent.updateChain();
  }
}

void ChainRule::applyAcceptEstablishedOptimization(Chain &chain) {
  if (acceptEstablishedOptimizationFound(chain))
    chain.parent_.enableAcceptEstablished(chain);
  else
    chain.parent_.disableAcceptEstablished(chain);
}

bool ChainRule::acceptEstablishedOptimizationFound(Chain &chain) {
  if (chain.rules_.size() > 0) {
    ChainRuleJsonObject conf;

    conf.setConntrack(ConntrackstatusEnum::ESTABLISHED);
    conf.setAction(ActionEnum::ACCEPT);

    ChainRule c(chain, conf);
    if (chain.rules_[0] != nullptr) {
      if (chain.rules_[0]->equal(c)) {
        chain.logger()->trace(
            "Accept established optimization founded in rule-set");
        return true;
      }
    }
  }

  chain.logger()->trace(
      "Accept established optimization NOT founded in rule-set");
  return false;
}

// Using removeEntry method is not resizing the rule set, it is just replacing
// the current rule with no-rule
// the result is

void ChainRule::deletes(Chain &parent, const ChainRuleJsonObject &conf) {
  // Find Matching rule number
  for (int i = 0; i < parent.rules_.size(); i++) {
    if (parent.rules_[i] != nullptr) {
      ChainRule c(parent, conf);
      // TODO improve
      if (parent.rules_[i]->equal(c)) {
        removeEntry(parent, i);
        return;
      }
    }
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
  // Remember to call here the remove static method for all-sub-objects of
  // ChainRule.
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
  parent.counters_.resize(parent.counters_.size() - 1);

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

// called when FLUSH chain
void ChainRule::remove(Chain &parent) {
  // This method removes all ChainRule objects in Chain.
  // Remember to call here the remove static method for all-sub-objects of
  // ChainRule.
  parent.rules_.clear();
  parent.counters_.clear();

  if (parent.parent_.interactive_) {
    parent.applyRules();
  }
}

std::string ChainRule::getSrc() {
  // This method retrieves the src value.
  if (!ipSrcIsSet) {
    throw std::runtime_error("Src not set.");
  }
  return this->ipSrc.toString();
}

void ChainRule::setSrc(const std::string &value) {
  // This method set the src value.
  throw std::runtime_error(
      "It is not possible to modify rule fields once created. Replace it with "
      "new one.");
}

std::string ChainRule::getDst() {
  // This method retrieves the dst value.
  if (!ipDstIsSet) {
    throw std::runtime_error("Dst not set.");
  }
  return this->ipDst.toString();
}

void ChainRule::setDst(const std::string &value) {
  // This method set the dst value.
  throw std::runtime_error(
      "It is not possible to modify rule fields once created. Replace it with "
      "new one.");
}

// TODO TO IMPLEMENT
std::string ChainRule::getOutIface() {
  // This method retrieves the outIface value.
  throw std::runtime_error("[ChainRule]: Method getOutIface not implemented");
}

void ChainRule::setOutIface(const std::string &value) {
  // This method set the outIface value.
  throw std::runtime_error(
      "It is not possible to modify rule fields once created. Replace it with "
      "new one.");
}

uint16_t ChainRule::getDport() {
  // This method retrieves the dport value.
  if (!dstPortIsSet) {
    throw std::runtime_error("DPort not set.");
  }
  return this->dstPort;
}

void ChainRule::setDport(const uint16_t &value) {
  // This method set the dport value.
  throw std::runtime_error(
      "It is not possible to modify rule fields once created. Replace it with "
      "new one.");
}
std::string ChainRule::getTcpflags() {
  // This method retrieves the tcpflags value.
  if (!tcpFlagsIsSet) {
    throw std::runtime_error("TcpFlags not set.");
  }
  std::string flags = "";
  flagsFromMasksToString(flags, this->flagsSet, this->flagsNotSet);
  return flags;
}

void ChainRule::setTcpflags(const std::string &value) {
  // This method set the tcpflags value.
  throw std::runtime_error(
      "It is not possible to modify rule fields once created. Replace it with "
      "new one.");
}

// TODO TO IMPLEMENT
std::string ChainRule::getInIface() {
  // This method retrieves the inIface value.
  throw std::runtime_error("[ChainRule]: Method getInIface not implemented");
}

void ChainRule::setInIface(const std::string &value) {
  // This method set the inIface value.
  throw std::runtime_error(
      "It is not possible to modify rule fields once created. Replace it with "
      "new one.");
}
std::string ChainRule::getL4proto() {
  // This method retrieves the l4proto value.
  if (!l4ProtoIsSet) {
    throw std::runtime_error("L4Proto not set.");
  }
  return protocolFromIntToString(this->l4Proto);
}

void ChainRule::setL4proto(const std::string &value) {
  // This method set the l4proto value.
  throw std::runtime_error("[ChainRule]: Method setL4proto not implemented");
}

ConntrackstatusEnum ChainRule::getConntrack() {
  if (!conntrackIsSet) {
    throw std::runtime_error("conntrack not set.");
  }
  return this->conntrack;
}

void ChainRule::setConntrack(const ConntrackstatusEnum &value) {
  // This method set the conntrack value.
  throw std::runtime_error(
      "It is not possible to modify rule fields once created. Replace it with "
      "new one.");
}

ActionEnum ChainRule::getAction() {
  // This method retrieves the action value.
  if (!actionIsSet) {
    throw std::runtime_error("Action not set.");
  }
  return this->action;
}

void ChainRule::setAction(const ActionEnum &value) {
  // This method set the action value.
  throw std::runtime_error(
      "It is not possible to modify rule fields once created. Replace it with "
      "new one.");
}

uint16_t ChainRule::getSport() {
  // This method retrieves the sport value.
  if (!srcPortIsSet) {
    throw std::runtime_error("SPort not set.");
  }
  return this->srcPort;
}

void ChainRule::setSport(const uint16_t &value) {
  // This method set the sport value.
  throw std::runtime_error(
      "It is not possible to modify rule fields once created. Replace it with "
      "new one.");
}

// ConntrackstatusEnum ChainRule::getConntrack() {
//   if (!conntrackIsSet) {
//     throw std::runtime_error("Conntrack not set.");
//   }
//   return this->conntrack;
// }

uint32_t ChainRule::getId() {
  // This method retrieves the id value.
  return id;
}

std::shared_ptr<spdlog::logger> ChainRule::logger() {
  return parent_.logger();
}
