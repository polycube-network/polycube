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

#include "ChainStats.h"
#include "Firewall.h"

ChainStats::ChainStats(Chain &parent, const ChainStatsJsonObject &conf)
    : parent_(parent) {
  this->counter = conf;
}

ChainStats::~ChainStats() {}

void ChainStats::update(const ChainStatsJsonObject &conf) {}

ChainStatsJsonObject ChainStats::toJsonObject() {
  ChainStatsJsonObject conf;

  try {
    conf.setSrc(getSrc());
  } catch (...) {
  }

  try {
    conf.setDescription(getDescription());
  } catch (...) {
  }

  try {
    conf.setDst(getDst());
  } catch (...) {
  }

  try {
    conf.setBytes(getBytes());
  } catch (...) {
  }

  try {
    conf.setPkts(getPkts());
  } catch (...) {
  }

  try {
    conf.setAction(getAction());
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
    conf.setConntrack(getConntrack());
  } catch (...) {
  }

  try {
    conf.setDport(getDport());
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

void ChainStats::fetchCounters(const Chain &parent, const uint32_t &id,
                               uint64_t &pkts, uint64_t &bytes) {
  std::map<std::pair<uint8_t, ChainNameEnum>, Firewall::Program *> &programs =
      parent.parent_.programs;

  if (programs.find(std::make_pair(ModulesConstants::ACTION, parent.name)) ==
      programs.end()) {
    pkts = 0;
    bytes = 0;
    return;
  }

  auto actionProgram = dynamic_cast<Firewall::ActionLookup *>(
      programs[std::make_pair(ModulesConstants::ACTION, parent.name)]);

  bytes = actionProgram->getBytesCount(id);
  pkts = actionProgram->getPktsCount(id);
  actionProgram->flushCounters(id);
}

std::shared_ptr<ChainStats> ChainStats::getDefaultActionCounters(
    Chain &parent) {
  /*Adding default rule counter*/
  ChainStatsJsonObject csj;

  csj.setAction(parent.defaultAction);

  /*Assigning an random ID just for showing*/
  csj.setId(parent.rules_.size());

  csj.setDescription("DEFAULT");

  std::map<std::pair<uint8_t, ChainNameEnum>, Firewall::Program *> &programs =
      parent.parent_.programs;

  auto actionProgram =
      dynamic_cast<Firewall::DefaultAction *>(programs[std::make_pair(
          ModulesConstants::DEFAULTACTION, ChainNameEnum::INVALID)]);

  csj.setPkts(actionProgram->getPktsCount(parent.name));
  csj.setBytes(actionProgram->getBytesCount(parent.name));

  return std::make_shared<ChainStats>(parent, csj);
}

std::string ChainStats::getSrc() {
  if (counter.getId() == parent_.rules_.size()) {
    // Default action
    throw new std::runtime_error("Default Action has no fields");
  }
  return parent_.rules_[counter.getId()]->getSrc();
}

std::string ChainStats::getDescription() {
  if (counter.getId() == parent_.rules_.size()) {
    return counter.getDescription();
  }
  return parent_.rules_[counter.getId()]->getDescription();
}

std::string ChainStats::getDst() {
  if (counter.getId() == parent_.rules_.size()) {
    // Default action
    throw new std::runtime_error("Default Action has no fields");
  }
  return parent_.rules_[counter.getId()]->getDst();
}

uint64_t ChainStats::getBytes() {
  return counter.getBytes();
}

uint64_t ChainStats::getPkts() {
  return counter.getPkts();
}

ActionEnum ChainStats::getAction() {
  if (counter.getId() == parent_.rules_.size()) {
    // Default action counter has the action set.
    return counter.getAction();
  }
  return parent_.rules_[counter.getId()]->getAction();
}

std::string ChainStats::getTcpflags() {
  if (counter.getId() == parent_.rules_.size()) {
    // Default action
    throw new std::runtime_error("Default Action has no fields");
  }
  return parent_.rules_[counter.getId()]->getTcpflags();
}

std::string ChainStats::getL4proto() {
  if (counter.getId() == parent_.rules_.size()) {
    // Default action
    throw new std::runtime_error("Default Action has no fields");
  }
  return parent_.rules_[counter.getId()]->getL4proto();
}

ConntrackstatusEnum ChainStats::getConntrack() {
  if (counter.getId() == parent_.rules_.size()) {
    // Default action
    throw new std::runtime_error("Default Action has no fields");
  }
  return parent_.rules_[counter.getId()]->getConntrack();
}

uint16_t ChainStats::getDport() {
  if (counter.getId() == parent_.rules_.size()) {
    // Default action
    throw new std::runtime_error("Default Action has no fields");
  }
  return parent_.rules_[counter.getId()]->getDport();
}

uint16_t ChainStats::getSport() {
  if (counter.getId() == parent_.rules_.size()) {
    // Default action
    throw new std::runtime_error("Default Action has no fields");
  }
  return parent_.rules_[counter.getId()]->getSport();
}

uint32_t ChainStats::getId() {
  return counter.getId();
}

std::shared_ptr<spdlog::logger> ChainStats::logger() {
  return parent_.logger();
}
