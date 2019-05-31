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

Chain::Chain(Firewall &parent, const ChainJsonObject &conf) : ChainBase(parent) {
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
    if (name == ChainNameEnum::INGRESS) {
      parent_.ingress_programs[ModulesConstants::DEFAULTACTION]->reload();
    } else if (name == ChainNameEnum::EGRESS) {
      parent_.egress_programs[ModulesConstants::DEFAULTACTION]->reload();
    }

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
  conf.setId(id);
  addRule(id, conf);

  ChainAppendOutputJsonObject result;
  result.setId(id);

//  TODO improve
//  if (parent_.interactive_) {
//    ChainRule::applyAcceptEstablishedOptimization(*this);
//    parent_.attachInterfaces();
//  }

  return result;
}

ChainResetCountersOutputJsonObject Chain::resetCounters() {
  ChainResetCountersOutputJsonObject result;
  try {
    std::vector<Firewall::Program *> *programs;

    bool * horus_runtime_enabled_;
    bool * horus_swap_;

    if (name == ChainNameEnum::INGRESS) {
      programs = &parent_.ingress_programs;
      horus_runtime_enabled_ = &parent_.horus_runtime_enabled_ingress_;
      horus_swap_ = &parent_.horus_swap_ingress_;
    } else if (name == ChainNameEnum::EGRESS) {
      programs = &parent_.egress_programs;
      horus_runtime_enabled_ = &parent_.horus_runtime_enabled_egress_;
      horus_swap_ = &parent_.horus_swap_egress_;
    } else {
      return result;
    }

    if (programs->at(ModulesConstants::ACTION) == nullptr) {
      throw std::runtime_error("No action loaded yet.");
    }

    auto actionProgram = dynamic_cast<Firewall::ActionLookup *>(
        programs->at(ModulesConstants::ACTION));

    Firewall::Horus * horusProgram;

    if (*horus_runtime_enabled_) {
      if (!(*horus_swap_)) {
        horusProgram = dynamic_cast<Firewall::Horus *>(programs->at(ModulesConstants::HORUS_INGRESS));
      } else {
        horusProgram = dynamic_cast<Firewall::Horus *>(programs->at(ModulesConstants::HORUS_INGRESS_SWAP));
      }
    }

    for (auto cr : rules_) {
      actionProgram->flushCounters(cr->getId());
      if (*horus_runtime_enabled_){
        horusProgram->flushCounters(cr->getId());
      }
    }

    dynamic_cast<Firewall::DefaultAction *>(
            programs->at(ModulesConstants::DEFAULTACTION))->flushCounters(name);

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

uint32_t Chain::getNrRules() {
  return rules_.size();
}

void Chain::updateChain() {
  std::vector<Firewall::Program *> *programs;
  bool * horus_runtime_enabled_;
  bool * horus_swap_;

  if (name == ChainNameEnum::INGRESS) {
    programs = &parent_.ingress_programs;
    horus_runtime_enabled_ = &parent_.horus_runtime_enabled_ingress_;
    horus_swap_ = &parent_.horus_swap_ingress_;
  } else if (name == ChainNameEnum::EGRESS) {
     programs = &parent_.egress_programs;
    horus_runtime_enabled_ = &parent_.horus_runtime_enabled_egress_;
    horus_swap_ = &parent_.horus_swap_egress_;
  } else {
    return;
  }

  logger()->info("[{0}] Starting to update the {1} chain for {2} rules...",
                 parent_.get_name(),
                 ChainJsonObject::ChainNameEnum_to_string(name), rules_.size());
  // std::lock_guard<std::mutex> lkBpf(parent_.bpfInjectMutex);
  auto start = std::chrono::high_resolution_clock::now();

  int index = ModulesConstants::NR_INITIAL_MODULES + (chainNumber * ModulesConstants::NR_MODULES);

  int startingIndex = index;
  Firewall::Program *firstProgramLoaded;
  std::vector<Firewall::Program *> newProgramsChain(ModulesConstants::NR_INITIAL_MODULES + ModulesConstants::NR_MODULES + 1);
  std::map<uint8_t, std::vector<uint64_t>> conntrack_map;
  std::map<struct IpAddr, std::vector<uint64_t>> ipsrc_map;
  std::map<struct IpAddr, std::vector<uint64_t>> ipdst_map;
  std::map<uint16_t, std::vector<uint64_t>> portsrc_map;
  std::map<uint16_t, std::vector<uint64_t>> portdst_map;
  std::map<int, std::vector<uint64_t>> protocol_map;
  std::vector<std::vector<uint64_t>> flags_map;

  std::map<struct HorusRule, struct HorusValue> horus;


  /*
   * HORUS - Homogeneous RUleset analySis
   *
   * Horus optimization allows to
   * a) offload a group of contiguous rules matching on same field
   * b) match the group of offloaded rules with complexity O(1) - single hashmap lookup
   * c) dynamically adapting to different groups of rules, matching each
   *    combination of ipsrc/dst, portsrc/dst, tcpflags
   * d) dynamically check when the optimization is possible according to current
   *    ruleset. It means check orthogonality
   *    of rules before the offloaded group, respect to the group itself.
   *
   * each pkt received by the program, is looked-up vs the HORUS HASHMAP.
   * hit ->
   *  -DROP action: drop the packet;
   *  -ACCEPT action: goto CTLABELING and CTTABLEUPDATE without going through pipeline
   * miss ->
   *  -GOTO all pipeline steps
   */

  *horus_runtime_enabled_ = false;

  // Apply Horus optimization only if it is enabled
  if (parent_.horus_enabled) {
    // if len Chain >= MIN_RULES_HORUS_OPTIMIZATION
    if (getRuleList().size() >= HorusConst::MIN_RULE_SIZE_FOR_HORUS) {
      // calculate horus ruleset
      horusFromRulesToMap(horus, getRuleList());

      // if horus.size() >= MIN_RULES_HORUS_OPTIMIZATION
      if (horus.size() >= HorusConst::MIN_RULE_SIZE_FOR_HORUS) {
        logger()->info("Horus Optimization ENABLED for this rule-set");

        *horus_runtime_enabled_ = true;

        // SWAP indexes
        *horus_swap_ = !(*horus_swap_);

        uint8_t horus_index_new = -1;
        uint8_t horus_index_old = -1;

        // Apply Horus optimization

        // Calculate current new/old indexes
        if (*horus_swap_) {
          horus_index_new = ModulesConstants::HORUS_INGRESS_SWAP;
          horus_index_old = ModulesConstants::HORUS_INGRESS;
        } else {
          horus_index_old = ModulesConstants::HORUS_INGRESS_SWAP;
          horus_index_new = ModulesConstants::HORUS_INGRESS;
        }

        // Compile and inject program

        std::vector<Firewall::Program *> *prog;

        if (name == ChainNameEnum::INGRESS) {
          prog = &parent_.ingress_programs;
        } else if (name == ChainNameEnum::EGRESS) {
          prog = &parent_.egress_programs;
        } else {
          throw std::runtime_error("No ingress/egress chain");
        }

        Firewall::Horus * horusptr =
                new Firewall::Horus(horus_index_new, parent_, name, horus);
        prog->at(horus_index_new) = horusptr;

        auto horusProgram = dynamic_cast<Firewall::Horus *>(
                programs->at(horus_index_new));

        horusProgram->updateMap(horus);

        auto parserIngress = dynamic_cast<Firewall::Parser *>(programs->at(ModulesConstants::PARSER));
        parserIngress->reload();

        // Delete old Horus, if present

        if (programs->at(horus_index_old) != nullptr) {
          delete programs->at(horus_index_old);
        }
        programs->at(horus_index_old) = nullptr;
      }
    }
  }
  if (*horus_runtime_enabled_ == false) {
    auto parserIngress = dynamic_cast<Firewall::Parser *>(programs->at(ModulesConstants::PARSER));
    parserIngress->reload();

    // Delete old Horus, if present
    delete programs->at(ModulesConstants::HORUS_INGRESS);
    delete programs->at(ModulesConstants::HORUS_INGRESS_SWAP);
    programs->at(ModulesConstants::HORUS_INGRESS) = nullptr;
    programs->at(ModulesConstants::HORUS_INGRESS_SWAP) = nullptr;
  }


  // calculate bitvectors, and check if no wildcard is present.
  // if no wildcard is present, we can early break the pipeline.
  // so we put modules with _break flags_map, before the others in order
  // to maximize probability to early break the pipeline.
  bool conntrack_break = conntrackFromRulesToMap(conntrack_map, rules_);
  bool ipsrc_break = ipFromRulesToMap(SOURCE_TYPE, ipsrc_map, rules_);
  bool ipdst_break = ipFromRulesToMap(DESTINATION_TYPE, ipdst_map, rules_);
  bool protocol_break = transportProtoFromRulesToMap(protocol_map, rules_);
  bool portsrc_break = portFromRulesToMap(SOURCE_TYPE, portsrc_map, rules_);
  bool portdst_break = portFromRulesToMap(DESTINATION_TYPE, portdst_map, rules_);
  bool flags_break = flagsFromRulesToMap(flags_map, rules_);

  logger()->debug(
          "Early break of pipeline conntrack:{0} ipsrc:{1} ipdst:{2} protocol:{3} "
          "portstc:{4} portdst:{5} flags_map:{6} ",
          conntrack_break, ipsrc_break, ipdst_break, protocol_break, portsrc_break,
          portdst_break, flags_break);

  // first loop iteration pushes program that could early break the pipeline
  // second iteration, push others programs

  bool second = false;

  for (int j = 0; j < 2; j++) {
    if (j == 1)
      second = true;

    // Looping through conntrack
    if (!conntrack_map.empty() && conntrack_break ^ second) {
      // At least one rule requires a matching on conntrack, so it can be
      // injected.
      if (!parent_.isContrackActive()) {
        logger()->error(
                "[{0}] Conntrack is not active, please remember to activate it.",
                parent_.getName());
      }
      Firewall::ConntrackMatch *conntrack =
              new Firewall::ConntrackMatch(index, name, this->parent_);
      newProgramsChain[ModulesConstants::CONNTRACKMATCH] = conntrack;
      // Now the program is loaded, populate it.
      conntrack->updateMap(conntrack_map);

      // This check is not really needed here, it will always be the first module
      // to be injected
      if (index == startingIndex) {
        firstProgramLoaded = conntrack;
      }
      ++index;
    }
    // Done looping through conntrack

    // Looping through IP source
    if (!ipsrc_map.empty() && (ipsrc_break ^ second)) {
      // At least one rule requires a matching on ipsource, so inject
      // the module on the first available position
      Firewall::IpLookup *iplookup =
              new Firewall::IpLookup(index, name, SOURCE_TYPE, this->parent_);
      newProgramsChain[ModulesConstants::IPSOURCE] = iplookup;
      // If this is the first module, adjust parsing to forward to it.
      if (index == startingIndex) {
        firstProgramLoaded = iplookup;
      }
      ++index;

      // Now the program is loaded, populate it.
      iplookup->updateMap(ipsrc_map);
    }
    // Done looping through IP source

    // Looping through IP destination
    if (!ipdst_map.empty() && ipdst_break ^ second) {
      // At least one rule requires a matching on ipdestination, so inject
      // the module on the first available position
      Firewall::IpLookup *iplookup =
              new Firewall::IpLookup(index, name, DESTINATION_TYPE, this->parent_);
      newProgramsChain[ModulesConstants::IPDESTINATION] = iplookup;
      // If this is the first module, adjust parsing to forward to it.
      if (index == startingIndex) {
        firstProgramLoaded = iplookup;
      }
      ++index;

      // Now the program is loaded, populate it.
      iplookup->updateMap(ipdst_map);
    }
    // Done looping through IP destination

    // Looping through l4 protocol
    if (!protocol_map.empty() && protocol_break ^ second) {
      // At least one rule requires a matching on
      // source port__map, so inject the module
      // on the first available position
      Firewall::L4ProtocolLookup *protocollookup =
              new Firewall::L4ProtocolLookup(index, name, this->parent_);
      newProgramsChain[ModulesConstants::L4PROTO] = protocollookup;
      // If this is the first module, adjust parsing to forward to it.
      if (index == startingIndex) {
        firstProgramLoaded = protocollookup;
      }
      ++index;

      // Now the program is loaded, populate it.
      protocollookup->updateMap(protocol_map);
    }
    // Done looping through l4 protocol

    // Looping through source port
    if (!portsrc_map.empty() && portsrc_break ^ second) {
      // At least one rule requires a matching on  source port__map,
      // so inject the  module  on the first available position
      Firewall::L4PortLookup *portlookup =
              new Firewall::L4PortLookup(index, name, SOURCE_TYPE, this->parent_, portsrc_map);
      newProgramsChain[ModulesConstants::PORTSOURCE] = portlookup;
      // If this is the first module, adjust parsing to forward to it.
      if (index == startingIndex) {
        firstProgramLoaded = portlookup;
      }
      ++index;

      // Now the program is loaded, populate it.
      portlookup->updateMap(portsrc_map);
    }
    // Done looping through source port

    // Looping through destination port
    if (!portdst_map.empty() && portdst_break ^ second) {
      // At least one rule requires a matching on source port__map,
      // so inject the module  on the first available position
      Firewall::L4PortLookup *portlookup =
              new Firewall::L4PortLookup(index, name, DESTINATION_TYPE, this->parent_, portdst_map);
      newProgramsChain[ModulesConstants::PORTDESTINATION] = portlookup;
      // If this is the first module, adjust parsing to forward to it.
      if (index == startingIndex) {
        firstProgramLoaded = portlookup;
      }
      ++index;

      // Now the program is loaded, populate it.
      portlookup->updateMap(portdst_map);
    }
    // Done looping through destination port

    // Looping through tcp flags_map
    if (!flags_map.empty() && flags_break ^ second) {
      // At least one rule requires a matching on flags_map,
      // so inject the  module in the first available position
      Firewall::TcpFlagsLookup *tcpflagslookup =
              new Firewall::TcpFlagsLookup(index, name, this->parent_);
      newProgramsChain[ModulesConstants::TCPFLAGS] = tcpflagslookup;
      // If this is the first module, adjust parsing to forward to it.
      if (index == startingIndex) {
        firstProgramLoaded = tcpflagslookup;
      }
      ++index;

      // Now the program is loaded, populate it.
      tcpflagslookup->updateMap(flags_map);
    }
    // Done looping through tcp flags_map
  }

  // Adding bitscan
  Firewall::BitScan *bitscan =
      new Firewall::BitScan(index, name, this->parent_);
  newProgramsChain[ModulesConstants::BITSCAN] = bitscan;
  // If this is the first module, adjust parsing to forward to it.
  if (index == startingIndex) {
    firstProgramLoaded = bitscan;
  }
  ++index;

  // Adding action taker
  Firewall::ActionLookup *actionlookup =
      new Firewall::ActionLookup(index, name, this->parent_);
  newProgramsChain[ModulesConstants::ACTION] = actionlookup;
  // If this is the first module, adjust parsing to forward to it.
  if (index == startingIndex) {
    firstProgramLoaded = actionlookup;
  }
  ++index;

  for (auto &rule : rules_) {
    actionlookup->updateTableValue(rule->getId(),
        ChainRule::ActionEnum_to_int(rule->getAction()));
  }

  // The new chain is ready. Instruct chainForwarder to switch to the new chain.
  auto &chainforwarder = programs->at(ModulesConstants::CHAINFORWARDER);
  chainforwarder->updateHop(1, firstProgramLoaded, name);
  chainforwarder->reload();

  // The parser has to be reloaded to account the new nmbr of elements
  programs->at(ModulesConstants::PARSER)->reload();

  // Unload the programs belonging to the old chain.
  for (int i = ModulesConstants::CONNTRACKMATCH;
       i <= ModulesConstants::ACTION; i++) {
    if (programs->at(i)) {
      delete programs->at(i);
      programs->at(i) = nullptr;
    }
  }

  for (int i = ModulesConstants::CONNTRACKMATCH;
       i <= ModulesConstants::ACTION; i++) {
    programs->at(i) = newProgramsChain[i];
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

std::shared_ptr<ChainStats> Chain::getStats(const uint32_t &id) {
  if (rules_.size() < id || !rules_[id]) {
    throw std::runtime_error("There is no rule " + id);
  }

  auto &counters = counters_;

  if (counters.size() <= id || !counters[id]) {
    // Counter not initialized yet
    ChainStatsJsonObject conf;
    uint64_t pkts, bytes;
    conf.setId(id);
    ChainStats::fetchCounters(*this, id, pkts, bytes);
    conf.setPkts(pkts);
    conf.setBytes(bytes);
    if (counters.size() <= id) {
      counters.resize(id + 1);
    }
    counters[id].reset(new ChainStats(*this, conf));
  } else {
    // Counter already existed, update it
    uint64_t pkts, bytes;
    ChainStats::fetchCounters(*this, id, pkts, bytes);
    counters[id]->counter.setPkts(counters[id]->getPkts() + pkts);
    counters[id]->counter.setBytes(counters[id]->getBytes() + bytes);
  }

  return counters[id];
}

std::vector<std::shared_ptr<ChainStats>> Chain::getStatsList() {
  std::vector<std::shared_ptr<ChainStats>> vect;

  for (std::shared_ptr<ChainRule> cr : rules_) {
    if (cr) {
      vect.push_back(getStats(cr->getId()));
    }
  }

  vect.push_back(ChainStats::getDefaultActionCounters(*this));

  return vect;
}

void Chain::addStats(const uint32_t &id, const ChainStatsJsonObject &conf) {
  throw std::runtime_error("[ChainStats]: Method create not allowed.");
}

void Chain::addStatsList(const std::vector<ChainStatsJsonObject> &conf) {
  throw std::runtime_error("[ChainStats]: Method create not allowed.");
}

void Chain::replaceStats(const uint32_t &id, const ChainStatsJsonObject &conf) {
  throw std::runtime_error("[ChainStats]: Method replace not allowed.");
}

void Chain::delStats(const uint32_t &id) {
  throw std::runtime_error("[ChainStats]: Method removeEntry not allowed");
}

void Chain::delStatsList() {
  throw std::runtime_error("[ChainStats]: Method removeEntry not allowed");
}

std::shared_ptr<ChainRule> Chain::getRule(const uint32_t &id) {
  if (rules_.size() <= id || !rules_[id]) {
    throw std::runtime_error("There is no rule " + id);
  }
  return rules_[id];
}

std::vector<std::shared_ptr<ChainRule>> Chain::getRuleList() {
  auto rules(rules_);

  // Adding a "stub" default rule
  ChainRuleJsonObject defaultRule;
  defaultRule.setAction(getDefault());
  defaultRule.setDescription("Default Policy");
  defaultRule.setId(rules_.size());

  rules.push_back(
      std::shared_ptr<ChainRule>(new ChainRule(*this, defaultRule)));

  return rules;
}

void Chain::addRule(const uint32_t &id, const ChainRuleJsonObject &conf) {

  if (id > rules_.size()) {
    throw std::runtime_error("rule id not allowed");
  }

  auto newRule = std::make_shared<ChainRule>(*this, conf);

  // Forcing counters update
  getStatsList();

  if (newRule == nullptr) {
    // Totally useless, but it is needed to avoid the compiler making wrong
    // assumptions and reordering
    throw new std::runtime_error("I won't be thrown");

  } else if (rules_.size() <= id && newRule != nullptr) {
    rules_.resize(rules_.size() + 1);
  }
  if (rules_[id]) {
    logger()->info("Rule {0} overwritten!", id);
  }

  rules_[id] = newRule;

  if (parent_.interactive_) {
    updateChain();
  }
}

void Chain::addRuleList(const std::vector<ChainRuleJsonObject> &conf) {
  for (auto &i : conf) {
    uint32_t id_ = i.getId();
    addRule(id_, i);
  }
}

// TODO check
void Chain::replaceRule(const uint32_t &id, const ChainRuleJsonObject &conf) {
  uint32_t id_ = conf.getId();
  addRule(id_, conf);
}

void Chain::delRule(const uint32_t &id) {
  if ((id >= rules_.size()) || (!rules_[id])) {
    throw std::runtime_error("There is no rule " + id);
  }

  // Forcing counters update
  getStatsList();

  for (uint32_t i = id; i < rules_.size() - 1; ++i) {
    rules_[i] = rules_[i + 1];
    rules_[i]->id = i;
  }

  rules_.resize(rules_.size() - 1);

  for (uint32_t i = id; i < counters_.size() - 1; ++i) {
    counters_[i] = counters_[i + 1];
    counters_[i]->counter.setId(i);
  }
  counters_.resize(counters_.size() - 1);

  if (parent_.interactive_) {
    applyRules();
  }
}

void Chain::delRuleList() {
  rules_.clear();
  counters_.clear();
  if (parent_.interactive_) {
    applyRules();
  }
}

ChainInsertOutputJsonObject Chain::insert(ChainInsertInputJsonObject input) {
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
  if (input.actionIsSet()) {
    conf.setAction(input.getAction());
  } else {
    conf.setAction(ActionEnum::DROP);
  }

  uint32_t id = 0;
  if (input.idIsSet()) {
    id = input.getId();
  }

  if (id > rules_.size()) {
    throw std::runtime_error("id not allowed");
  }

  auto newRule = std::make_shared<ChainRule>(*this, conf);

  ChainStatsJsonObject confStats;
  auto newStats = std::make_shared<ChainStats>(*this, confStats);

  getStatsList();

  if (newRule == nullptr) {
    // Totally useless, but it is needed to avoid the compiler making wrong
    // assumptions and reordering
    throw new std::runtime_error("I won't be thrown");

  } else if (rules_.size() >= id && newRule != nullptr) {
    rules_.resize(rules_.size() + 1);
    counters_.resize(counters_.size() + 1);
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

  for (i = rules_.size() - 2; i >= id_int; i--) {
    rules_[i + 1] = rules_[i];
    counters_[i + 1] = counters_[i];
    if (rules_[i + 1] != nullptr) {
      rules_[i + 1]->id = i + 1;
    }
    if (counters_[i + 1] != nullptr) {
      counters_[i + 1]->counter.setId(i + 1);
    }
  }

  rules_[id] = newRule;
  rules_[id]->id = id;

  counters_[id] = newStats;
  counters_[id]->counter.setPkts(0);
  counters_[id]->counter.setBytes(0);
  counters_[id]->counter.setId(id);

  if (parent_.interactive_) {
    updateChain();
  }

  // set fields for return object
  ChainInsertOutputJsonObject result;
  result.setId(id);

  return result;
}

void Chain::deletes(ChainDeleteInputJsonObject input) {
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
  if (input.actionIsSet()) {
    conf.setAction(input.getAction());
  } else {
    conf.setAction(ActionEnum::DROP);
  }

  for (int i = 0; i < rules_.size(); i++) {
    if (rules_[i] != nullptr) {
      ChainRule c(*this, conf);
      if (rules_[i]->equal(c)) {
        delRule(i);
        return;
      }
    }
  }
  throw std::runtime_error("no matching rule to delete");
}
