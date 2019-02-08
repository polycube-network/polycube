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

#include "Iptables.h"
#include "serializer/ChainStatsJsonObject.h"

ChainStats::ChainStats(Chain &parent, const ChainStatsJsonObject &conf)
    : parent_(parent) {
  logger()->trace("Creating ChainStats instance");
  this->counter = conf;
  rule = conf.getId();
}

ChainStats::~ChainStats() {}

void ChainStats::update(const ChainStatsJsonObject &conf) {
  // This method updates all the object/parameter in ChainStats object specified
  // in the conf JsonObject.
  // You can modify this implementation.
}

ChainStatsJsonObject ChainStats::toJsonObject() {
  ChainStatsJsonObject conf;

  conf.setPkts(getPkts());
  conf.setBytes(getBytes());
  conf.setId(getId());
  conf.setDesc(counter.getDesc());

  return conf;
}

void ChainStats::create(Chain &parent, const uint32_t &id,
                        const ChainStatsJsonObject &conf) {
  // This method creates the actual ChainStats object given thee key param.
  // Please remember to call here the create static method for all sub-objects
  // of ChainStats.

  throw std::runtime_error("[ChainStats]: Method create not supported");
}

std::shared_ptr<ChainStats> ChainStats::getEntry(Chain &parent,
                                                 const uint32_t &id) {
  // This method retrieves the pointer to ChainStats object specified by its
  // keys.
  if (parent.rules_.size() < id || !parent.rules_[id]) {
    throw std::runtime_error("There is no rule " + id);
  }

  auto &counters = parent.counters_;

  if (counters.size() <= id || !counters[id]) {
    // Counter not initialized yet
    ChainStatsJsonObject conf;
    uint64_t pkts, bytes;
    conf.setId(id);
    fetchCounters(parent, id, pkts, bytes);
    conf.setPkts(pkts);
    conf.setBytes(bytes);
    if (counters.size() <= id) {
      counters.resize(id + 1);
    }
    counters[id].reset(new ChainStats(parent, conf));
  } else {
    // Counter already existed, update it
    uint64_t pkts, bytes;
    fetchCounters(parent, id, pkts, bytes);
    counters[id]->counter.setPkts(counters[id]->getPkts() + pkts);
    counters[id]->counter.setBytes(counters[id]->getBytes() + bytes);
  }

  return counters[id];
}

void ChainStats::removeEntry(Chain &parent, const uint32_t &id) {
  // This method removes the single ChainStats object specified by its keys.
  // Remember to call here the remove static method for all-sub-objects of
  // ChainStats.
  throw std::runtime_error("[ChainStats]: Method removeEntry not supported");
}

void ChainStats::fetchCounters(const Chain &parent, const uint32_t &id,
                               uint64_t &pkts, uint64_t &bytes) {
  std::map<std::pair<uint8_t, ChainNameEnum>, Iptables::Program *> &programs =
      parent.parent_.programs_;

  if (programs.find(std::make_pair(ModulesConstants::ACTION, parent.name)) ==
      programs.end()) {
    pkts = 0;
    bytes = 0;
    return;
  }

  auto actionProgram = dynamic_cast<Iptables::ActionLookup *>(
      programs[std::make_pair(ModulesConstants::ACTION, parent.name)]);

  bytes = actionProgram->getBytesCount(id);
  pkts = actionProgram->getPktsCount(id);
  actionProgram->flushCounters(id);

  if (id == 0) {
    // if first rule and acceptEstablished optimization enabled for this chain
    // sum accept established optimization counters to first rule

    std::map<std::pair<uint8_t, ChainNameEnum>, Iptables::Program *> &programs =
        parent.parent_.programs_;

    uint8_t module = 0;
    ChainNameEnum chainnameenum;

    if (parent.name == ChainNameEnum::INPUT) {
      chainnameenum = ChainNameEnum::INVALID_INGRESS;
      if (!parent.parent_.accept_established_enabled_input_)
        return;
      module = ModulesConstants::CONNTRACKLABEL_INGRESS;
    }
    if (parent.name == ChainNameEnum::FORWARD) {
      chainnameenum = ChainNameEnum::INVALID_INGRESS;
      if (!parent.parent_.accept_established_enabled_forward_)
        return;
      module = ModulesConstants::CONNTRACKLABEL_INGRESS;
    }
    if (parent.name == ChainNameEnum::OUTPUT) {
      chainnameenum = ChainNameEnum::INVALID_EGRESS;
      if (!parent.parent_.accept_established_enabled_output_)
        return;
      module = ModulesConstants::CONNTRACKLABEL_EGRESS;
    }

    auto conntrackLabelProgram = dynamic_cast<Iptables::ConntrackLabel *>(
        programs[std::make_pair(module, chainnameenum)]);

    pkts += conntrackLabelProgram->getAcceptEstablishedPktsCount(parent.name);
    bytes += conntrackLabelProgram->getAcceptEstablishedBytesCount(parent.name);

    conntrackLabelProgram->flushCounters(parent.name, id);
  }

  if (parent.parent_.horus_runtime_enabled_) {
    if (!parent.parent_.horus_swap_) {
      auto horusProgram = dynamic_cast<Iptables::Horus *>(
          programs[std::make_pair(ModulesConstants::HORUS_INGRESS,
                                  ChainNameEnum::INVALID_INGRESS)]);
      bytes += horusProgram->getBytesCount(id);
      pkts += horusProgram->getPktsCount(id);
      horusProgram->flushCounters(id);
    } else {
      auto horusProgram = dynamic_cast<Iptables::Horus *>(
          programs[std::make_pair(ModulesConstants::HORUS_INGRESS_SWAP,
                                  ChainNameEnum::INVALID_INGRESS)]);
      bytes += horusProgram->getBytesCount(id);
      pkts += horusProgram->getPktsCount(id);
      horusProgram->flushCounters(id);
    }
  }
}

std::shared_ptr<ChainStats> ChainStats::getDefaultActionCounters(
    Chain &parent) {
  /*Adding default rule counter*/
  ChainStatsJsonObject csj;

  /*Assigning an random ID just for showing*/
  csj.setId(parent.rules_.size());

  csj.setDesc("DEFAULT");

  std::map<std::pair<uint8_t, ChainNameEnum>, Iptables::Program *> &programs =
      parent.parent_.programs_;

  uint8_t module = 0;
  ChainNameEnum chainnameenum;

  if (parent.getName() == ChainNameEnum::INPUT) {
    chainnameenum = ChainNameEnum::INVALID_INGRESS;
    module = ModulesConstants::CHAINSELECTOR_INGRESS;
  }
  if (parent.getName() == ChainNameEnum::FORWARD) {
    chainnameenum = ChainNameEnum::INVALID_INGRESS;
    module = ModulesConstants::CHAINSELECTOR_INGRESS;
  }
  if (parent.getName() == ChainNameEnum::OUTPUT) {
    chainnameenum = ChainNameEnum::INVALID_EGRESS;
    module = ModulesConstants::CHAINSELECTOR_EGRESS;
  }

  auto chainSelectorProgram = dynamic_cast<Iptables::ChainSelector *>(
      programs[std::make_pair(module, chainnameenum)]);

  csj.setPkts(chainSelectorProgram->getDefaultPktsCount(parent.name));
  csj.setBytes(chainSelectorProgram->getDefaultBytesCount(parent.name));

  return std::make_shared<ChainStats>(parent, csj);
}

std::vector<std::shared_ptr<ChainStats>> ChainStats::get(Chain &parent) {
  // This methods get the pointers to all the ChainStats objects in Chain.
  std::vector<std::shared_ptr<ChainStats>> vect;
  ChainStatsJsonObject csj;

  for (std::shared_ptr<ChainRule> cr : parent.rules_) {
    if (cr) {
      csj.setId(cr->id);
      // vect.push_back(std::make_shared<ChainStats>(parent, csj));
      vect.push_back(getEntry(parent, cr->id));
    }
  }

  vect.push_back(getDefaultActionCounters(parent));

  return vect;
}

void ChainStats::remove(Chain &parent) {
  // This method removes all ChainStats objects in Chain.
  // Remember to call here the remove static method for all-sub-objects of
  // ChainStats.
  throw std::runtime_error("[ChainStats]: Method remove not implemented");
}

uint64_t ChainStats::getPkts() {
  return counter.getPkts();
}

uint64_t ChainStats::getBytes() {
  return counter.getBytes();
}

uint32_t ChainStats::getId() {
  // This method retrieves the id value.
  return counter.getId();
}

std::shared_ptr<spdlog::logger> ChainStats::logger() {
  return parent_.logger();
}
