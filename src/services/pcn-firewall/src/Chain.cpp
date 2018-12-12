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
#include "Firewall.h"

Chain::Chain(Firewall &parent, const ChainJsonObject &conf) : parent_(parent) {
  update(conf);
}

Chain::~Chain() {}

void Chain::update(const ChainJsonObject &conf) {
  // This method updates all the object/parameter in Chain object specified in
  // the conf JsonObject.

  if (conf.nameIsSet()) {
    name = conf.getName();
  }

  if (conf.defaultIsSet()) {
    setDefault(conf.getDefault());
  }

  if (conf.statsIsSet()) {
    for (auto &i : conf.getStats()) {
      auto id = i.getId();
      auto m = getStats(id);
      m->update(i);
    }
  }

  if (conf.ruleIsSet()) {
    for (auto &i : conf.getRule()) {
      auto id = i.getId();
      auto m = getRule(id);
      m->update(i);
    }
  }
}

ChainJsonObject Chain::toJsonObject() {
  ChainJsonObject conf;

  conf.setDefault(getDefault());

  for (auto &i : getStatsList()) {
    conf.addChainStats(i->toJsonObject());
  }

  conf.setName(getName());
  for (auto &i : getRuleList()) {
    conf.addChainRule(i->toJsonObject());
  }

  return conf;
}

void Chain::create(Firewall &parent, const ChainNameEnum &name,
                   const ChainJsonObject &conf) {
  // This method creates the actual Chain object given thee key param.
  ChainJsonObject namedChain = conf;
  namedChain.setName(name);
  if (parent.chains_.count(name) != 0) {
    throw std::runtime_error("There is already a chain " +
                             ChainJsonObject::ChainNameEnum_to_string(name));
  }

  parent.chains_.emplace(std::piecewise_construct, std::forward_as_tuple(name),
                         std::forward_as_tuple(parent, namedChain));
}

std::shared_ptr<Chain> Chain::getEntry(Firewall &parent,
                                       const ChainNameEnum &name) {
  // This method retrieves the pointer to Chain object specified by its keys.
  if (parent.chains_.count(name) == 0) {
    throw std::runtime_error("There is no chain " +
                             ChainJsonObject::ChainNameEnum_to_string(name));
  }
  if (!parent.transparent && name == ChainNameEnum::EGRESS) {
    throw std::runtime_error(
        "Cannot configure rules on EGRESS chain when the firewall is "
        "configured with unidirectional ports");
  }
  return std::shared_ptr<Chain>(&parent.chains_.at(name), [](Chain *) {});
}

void Chain::removeEntry(Firewall &parent, const ChainNameEnum &name) {
  // This method removes the single Chain object specified by its keys.
  // parent.chains_.erase(name);
  throw std::runtime_error("Method not supported.");
}

std::vector<std::shared_ptr<Chain>> Chain::get(Firewall &parent) {
  // This methods get the pointers to all the Chain objects in Firewall.
  std::vector<std::shared_ptr<Chain>> chains;

  for (auto &it : parent.chains_) {
    chains.push_back(Chain::getEntry(parent, it.first));
  }

  return chains;
}

void Chain::remove(Firewall &parent) {
  // This method removes all Chain objects in Firewall.
  // parent.chains_.clear();
  throw std::runtime_error("Method not supported.");
}

ActionEnum Chain::getDefault() {
  // This method retrieves the default value.
  return this->defaultAction;
}

void Chain::setDefault(const ActionEnum &value) {
  // This method set the default value.
  if (this->defaultAction == value) {
    logger()->debug("[{0}] Default action already set. ", parent_.getName());
    return;
  }
  this->defaultAction = value;
  try {
    parent_
        .programs[std::make_pair(ModulesConstants::DEFAULTACTION,
                                 ChainNameEnum::INVALID)]
        ->reload();

  } catch (std::runtime_error re) {
    logger()->error(
        "[{0}] Can't reload the code for default action. Error: {1} ",
        parent_.getName(), re.what());
    return;
  }

  logger()->debug("[{0}] Default action set. ", parent_.getName());
}

ChainNameEnum Chain::getName() {
  // This method retrieves the name value.
  return this->name;
}

ChainAppendOutputJsonObject Chain::append(ChainAppendInputJsonObject input) {
  ChainRuleJsonObject conf;
  if (input.conntrackIsSet()) {
    conf.setConntrack(input.getConntrack());
  }
  if (input.srcIsSet()) {
    conf.setSrc(input.getSrc());
  }
  if (input.dstIsSet()) {
    conf.setDst(input.getDst());
  }
  if (input.sportIsSet()) {
    conf.setSport(input.getSport());
  }
  if (input.dportIsSet()) {
    conf.setDport(input.getDport());
  }
  if (input.tcpflagsIsSet()) {
    conf.setTcpflags(input.getTcpflags());
  }
  if (input.l4protoIsSet()) {
    conf.setL4proto(input.getL4proto());
  }
  if (input.descriptionIsSet()) {
    conf.setDescription(input.getDescription());
  }
  if (input.actionIsSet()) {
    conf.setAction(input.getAction());
  } else {
    conf.setAction(ActionEnum::DROP);
  }

  uint32_t id = rules_.size();
  ChainAppendOutputJsonObject result;
  ChainRule::create(*this, id, conf);
  result.setId(id);
  return result;
}

ChainResetCountersOutputJsonObject Chain::resetCounters() {
  ChainResetCountersOutputJsonObject result;
  try {
    std::map<std::pair<uint8_t, ChainNameEnum>, Firewall::Program *> &programs =
        parent_.programs;

    if (programs.find(std::make_pair(ModulesConstants::ACTION, name)) ==
        programs.end()) {
      throw std::runtime_error("No action loaded yet.");
    }

    auto actionProgram = dynamic_cast<Firewall::ActionLookup *>(
        programs[std::make_pair(ModulesConstants::ACTION, name)]);

    for (auto cr : rules_) {
      actionProgram->flushCounters(cr->getId());
    }

    dynamic_cast<Firewall::DefaultAction *>(
        programs[std::make_pair(ModulesConstants::DEFAULTACTION,
                                ChainNameEnum::INVALID)])
        ->flushCounters(name);

    counters_.clear();

    result.setResult(true);
  } catch (std::exception &e) {
    logger()->error("[{0}] Flushing counters error: {1} ", parent_.getName(),
                    e.what());
    result.setResult(false);
  }

  return result;
}

ChainApplyRulesOutputJsonObject Chain::applyRules() {
  ChainApplyRulesOutputJsonObject result;
  try {
    updateChain();
    result.setResult(true);
  } catch (...) {
    result.setResult(false);
  }
  return result;
}

std::shared_ptr<spdlog::logger> Chain::logger() {
  return parent_.logger();
}

uint32_t Chain::getNrRules() {
  /*
   * ChainRule::get returns only the valid rules to avoid segmentation faults
   * all around the code.
   * This methods returns the true number of rules, that can't be get from
   * ChainRule::get without
   * looking for the max id.
   */
  return rules_.size();
}

std::vector<std::shared_ptr<ChainRule>> Chain::getRealRuleList() {
  return ChainRule::get(*this);
}

void Chain::updateChain() {
  logger()->info("[{0}] Starting to update the {1} chain for {2} rules...",
                 parent_.get_name(),
                 ChainJsonObject::ChainNameEnum_to_string(name), rules_.size());
  // std::lock_guard<std::mutex> lkBpf(parent_.bpfInjectMutex);
  auto start = std::chrono::high_resolution_clock::now();

  int index;
  if (name == ChainNameEnum::INGRESS) {
    index = 3 + (chainNumber * ModulesConstants::NR_MODULES);
  } else {
    index = 3 + chainNumber * ModulesConstants::NR_MODULES +
            ModulesConstants::NR_MODULES * 2;
  }

  int startingIndex = index;
  Firewall::Program *firstProgramLoaded;
  std::map<std::pair<uint8_t, ChainNameEnum>, Firewall::Program *>
      newProgramsChain;
  std::map<uint8_t, std::vector<uint64_t>> states;
  std::map<struct IpAddr, std::vector<uint64_t>> ips;
  std::map<uint16_t, std::vector<uint64_t>> ports;
  std::map<int, std::vector<uint64_t>> protocols;
  std::vector<std::vector<uint64_t>> flags;

  auto rules = getRealRuleList();

  // Looping through conntrack
  conntrack_from_rules_to_map(states, rules);
  if (!states.empty()) {
    // At least one rule requires a matching on conntrack, so it can be
    // injected.
    if (!parent_.isContrackActive()) {
      logger()->error(
          "[{0}] Conntrack is not active, please remember to activate it.",
          parent_.getName());
    }
    newProgramsChain.insert(
        std::pair<std::pair<uint8_t, ChainNameEnum>, Firewall::Program *>(
            std::make_pair(ModulesConstants::CONNTRACKMATCH, name),
            new Firewall::ConntrackMatch(index, name, this->parent_)));
    // Now the program is loaded, populate it.
    dynamic_cast<Firewall::ConntrackMatch *>(
        newProgramsChain[std::make_pair(ModulesConstants::CONNTRACKMATCH,
                                        name)])
        ->updateMap(states);

    // This check is not really needed here, it will always be the first module
    // to be injected
    if (index == startingIndex) {
      firstProgramLoaded = newProgramsChain[std::make_pair(
          ModulesConstants::CONNTRACKMATCH, name)];
    }
    ++index;
  }
  states.clear();
  // Done looping through conntrack

  // Looping through IP source
  ip_from_rules_to_map(SOURCE_TYPE, ips, rules);
  if (!ips.empty()) {
    // At least one rule requires a matching on ipsource, so inject
    // the module on the first available position
    newProgramsChain.insert(
        std::pair<std::pair<uint8_t, ChainNameEnum>, Firewall::Program *>(
            std::make_pair(ModulesConstants::IPSOURCE, name),
            new Firewall::IpLookup(index, name, SOURCE_TYPE, this->parent_)));
    // If this is the first module, adjust parsing to forward to it.
    if (index == startingIndex) {
      firstProgramLoaded =
          newProgramsChain[std::make_pair(ModulesConstants::IPSOURCE, name)];
    }
    ++index;

    // Now the program is loaded, populate it.
    dynamic_cast<Firewall::IpLookup *>(
        newProgramsChain[std::make_pair(ModulesConstants::IPSOURCE, name)])
        ->updateMap(ips);
  }
  ips.clear();
  // Done looping through IP source

  // Looping through IP destination
  ip_from_rules_to_map(DESTINATION_TYPE, ips, rules);

  if (!ips.empty()) {
    // At least one rule requires a matching on source ip, so inject the
    // module on the first available position
    newProgramsChain.insert(
        std::pair<std::pair<uint8_t, ChainNameEnum>, Firewall::Program *>(
            std::make_pair(ModulesConstants::IPDESTINATION, name),
            new Firewall::IpLookup(index, name, DESTINATION_TYPE,
                                   this->parent_)));
    // If this is the first module, adjust parsing to forward to it.
    if (index == startingIndex) {
      firstProgramLoaded = newProgramsChain[std::make_pair(
          ModulesConstants::IPDESTINATION, name)];
    }
    ++index;

    // Now the program is loaded, populate it.
    dynamic_cast<Firewall::IpLookup *>(
        newProgramsChain[std::make_pair(ModulesConstants::IPDESTINATION, name)])
        ->updateMap(ips);
  }
  ips.clear();
  // Done looping through IP destination

  // Looping through l4 protocol
  transportproto_from_rules_to_map(protocols, rules);

  if (!protocols.empty()) {
    // At least one rule requires a matching on
    // source ports, so inject the module
    // on the first available position
    newProgramsChain.insert(
        std::pair<std::pair<uint8_t, ChainNameEnum>, Firewall::Program *>(
            std::make_pair(ModulesConstants::L4PROTO, name),
            new Firewall::L4ProtocolLookup(index, name, this->parent_)));

    // If this is the first module, adjust parsing to forward to it.
    if (index == startingIndex) {
      firstProgramLoaded =
          newProgramsChain[std::make_pair(ModulesConstants::L4PROTO, name)];
    }
    ++index;

    // Now the program is loaded, populate it.
    dynamic_cast<Firewall::L4ProtocolLookup *>(
        newProgramsChain[std::make_pair(ModulesConstants::L4PROTO, name)])
        ->updateMap(protocols);
  }
  protocols.clear();
  // Done looping through l4 protocol

  // Looping through source port
  port_from_rules_to_map(SOURCE_TYPE, ports, rules);

  if (!ports.empty()) {
    // At least one rule requires a matching on  source ports,
    // so inject the  module  on the first available position
    newProgramsChain.insert(
        std::pair<std::pair<uint8_t, ChainNameEnum>, Firewall::Program *>(
            std::make_pair(ModulesConstants::PORTSOURCE, name),
            new Firewall::L4PortLookup(index, name, SOURCE_TYPE,
                                       this->parent_)));

    // If this is the first module, adjust parsing to forward to it.
    if (index == startingIndex) {
      firstProgramLoaded =
          newProgramsChain[std::make_pair(ModulesConstants::PORTSOURCE, name)];
    }
    ++index;

    // Now the program is loaded, populate it.
    dynamic_cast<Firewall::L4PortLookup *>(
        newProgramsChain[std::make_pair(ModulesConstants::PORTSOURCE, name)])
        ->updateMap(ports);
  }
  ports.clear();
  // Done looping through source port

  // Looping through destination port
  port_from_rules_to_map(DESTINATION_TYPE, ports, rules);

  if (!ports.empty()) {
    // At least one rule requires a matching on source ports,
    // so inject the module  on the first available position
    newProgramsChain.insert(
        std::pair<std::pair<uint8_t, ChainNameEnum>, Firewall::Program *>(
            std::make_pair(ModulesConstants::PORTDESTINATION, name),
            new Firewall::L4PortLookup(index, name, DESTINATION_TYPE,
                                       this->parent_)));
    // If this is the first module, adjust
    // parsing to forward to it.
    if (index == startingIndex) {
      firstProgramLoaded = newProgramsChain[std::make_pair(
          ModulesConstants::PORTDESTINATION, name)];
    }
    ++index;

    // Now the program is loaded, populate it.
    dynamic_cast<Firewall::L4PortLookup *>(
        newProgramsChain[std::make_pair(ModulesConstants::PORTDESTINATION,
                                        name)])
        ->updateMap(ports);
  }
  ports.clear();
  // Done looping through destination port

  // Looping through tcp flags
  flags_from_rules_to_map(flags, rules);

  if (!flags.empty()) {
    // At least one rule requires a matching on flags,
    // so inject the  module in the first available position
    newProgramsChain.insert(
        std::pair<std::pair<uint8_t, ChainNameEnum>, Firewall::Program *>(
            std::make_pair(ModulesConstants::TCPFLAGS, name),
            new Firewall::TcpFlagsLookup(index, name, this->parent_)));

    // If this is the first module, adjust parsing to forward to it.
    if (index == startingIndex) {
      firstProgramLoaded =
          newProgramsChain[std::make_pair(ModulesConstants::TCPFLAGS, name)];
    }
    ++index;

    // Now the program is loaded, populate it.
    dynamic_cast<Firewall::TcpFlagsLookup *>(
        newProgramsChain[std::make_pair(ModulesConstants::TCPFLAGS, name)])
        ->updateMap(flags);
  }
  flags.clear();

  // Done looping through tcp flags

  // Adding bitscan
  newProgramsChain.insert(
      std::pair<std::pair<uint8_t, ChainNameEnum>, Firewall::Program *>(
          std::make_pair(ModulesConstants::BITSCAN, name),
          new Firewall::BitScan(index, name, this->parent_)));
  // If this is the first module, adjust parsing to forward to it.
  if (index == startingIndex) {
    firstProgramLoaded =
        newProgramsChain[std::make_pair(ModulesConstants::BITSCAN, name)];
  }
  ++index;

  // Adding action taker
  newProgramsChain.insert(
      std::pair<std::pair<uint8_t, ChainNameEnum>, Firewall::Program *>(
          std::make_pair(ModulesConstants::ACTION, name),
          new Firewall::ActionLookup(index, name, this->parent_)));

  for (auto rule : rules) {
    dynamic_cast<Firewall::ActionLookup *>(
        newProgramsChain[std::make_pair(ModulesConstants::ACTION, name)])
        ->updateTableValue(rule->getId(),
                           ChainRule::ActionEnum_to_int(rule->getAction()));
  }
  // The new chain is ready. Instruct chainForwarder to switch to the new chain.
  parent_
      .programs[std::make_pair(ModulesConstants::CHAINFORWARDER,
                               ChainNameEnum::INVALID)]
      ->updateHop(1, firstProgramLoaded, name);

  parent_
      .programs[std::make_pair(ModulesConstants::CHAINFORWARDER,
                               ChainNameEnum::INVALID)]
      ->reload();

  // The parser has to be reloaded to account the new nmbr of elements
  parent_
      .programs[std::make_pair(ModulesConstants::PARSER,
                               ChainNameEnum::INVALID)]
      ->reload();

  // Unload the programs belonging to the old chain.
  for (auto it = parent_.programs.begin(); it != parent_.programs.end();) {
    if (it->first.second == name) {
      delete it->second;
      it = parent_.programs.erase(it);
    } else {
      ++it;
    }
  }

  // Copy the new program references to the main map.
  for (auto &program : newProgramsChain) {
    parent_
        .programs[std::make_pair(program.first.first, program.first.second)] =
        program.second;
  }

  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed_seconds = end - start;

  // toggle chainNumberIngress
  chainNumber = (chainNumber == 0) ? 1 : 0;
  logger()->info("[{0}] Rules for the {1} chain have been updated in {2}s!",
                 parent_.get_name(),
                 ChainJsonObject::ChainNameEnum_to_string(name),
                 elapsed_seconds.count());
}
