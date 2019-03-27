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
  conf.setDescription(counter.getDescription());

  return conf;
}

void ChainStats::fetchCounters(const Chain &parent, const uint32_t &id,
                               uint64_t &pkts, uint64_t &bytes) {
  std::map<std::pair<uint8_t, ChainNameEnum>,
           std::shared_ptr<Iptables::Program>> &programs =
      parent.parent_.programs_;

  if (programs.find(std::make_pair(ModulesConstants::ACTION, parent.name)) ==
      programs.end()) {
    pkts = 0;
    bytes = 0;
    return;
  }

  auto actionProgram = std::dynamic_pointer_cast<Iptables::ActionLookup>(
      programs[std::make_pair(ModulesConstants::ACTION, parent.name)]);

  bytes = actionProgram->getBytesCount(id);
  pkts = actionProgram->getPktsCount(id);
  actionProgram->flushCounters(id);

  if (id == 0) {
    // if first rule and acceptEstablished optimization enabled for this chain
    // sum accept established optimization counters to first rule

    std::map<std::pair<uint8_t, ChainNameEnum>,
             std::shared_ptr<Iptables::Program>> &programs =
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

    auto conntrackLabelProgram =
        std::dynamic_pointer_cast<Iptables::ConntrackLabel>(
            programs[std::make_pair(module, chainnameenum)]);

    pkts += conntrackLabelProgram->getAcceptEstablishedPktsCount(parent.name);
    bytes += conntrackLabelProgram->getAcceptEstablishedBytesCount(parent.name);

    conntrackLabelProgram->flushCounters(parent.name, id);
  }

  if (parent.parent_.horus_runtime_enabled_) {
    if (!parent.parent_.horus_swap_) {
      auto horusProgram = std::dynamic_pointer_cast<Iptables::Horus>(
          programs[std::make_pair(ModulesConstants::HORUS_INGRESS,
                                  ChainNameEnum::INVALID_INGRESS)]);
      bytes += horusProgram->getBytesCount(id);
      pkts += horusProgram->getPktsCount(id);
      horusProgram->flushCounters(id);
    } else {
      auto horusProgram = std::dynamic_pointer_cast<Iptables::Horus>(
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

  csj.setDescription("DEFAULT");

  std::map<std::pair<uint8_t, ChainNameEnum>,
           std::shared_ptr<Iptables::Program>> &programs =
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

  auto chainSelectorProgram =
      std::dynamic_pointer_cast<Iptables::ChainSelector>(
          programs[std::make_pair(module, chainnameenum)]);

  csj.setPkts(chainSelectorProgram->getDefaultPktsCount(parent.name));
  csj.setBytes(chainSelectorProgram->getDefaultBytesCount(parent.name));

  return std::make_shared<ChainStats>(parent, csj);
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

std::string ChainStats::getDescription() {
  // This method retrieves the description value.
  return "";
}

std::shared_ptr<spdlog::logger> ChainStats::logger() {
  return parent_.logger();
}
