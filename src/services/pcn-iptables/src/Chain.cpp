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
#include "Iptables.h"

Chain::Chain(Iptables &parent, const ChainJsonObject &conf) : parent_(parent) {
  logger()->trace("Creating Chain instance");
  update(conf);
}

Chain::~Chain() {}

void Chain::update(const ChainJsonObject &conf) {
  // This method updates all the object/parameter in Chain object specified in
  // the conf JsonObject.
  // You can modify this implementation.

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

void Chain::create(Iptables &parent, const ChainNameEnum &name,
                   const ChainJsonObject &conf) {
  // This method creates the actual Chain object given thee key param.
  // Please remember to call here the create static method for all sub-objects
  // of Chain.

  ChainJsonObject namedChain = conf;
  namedChain.setName(name);
  if (parent.chains_.count(name) != 0) {
    throw std::runtime_error("There is already a chain " +
                             ChainJsonObject::ChainNameEnum_to_string(name));
  }

  parent.chains_.emplace(std::piecewise_construct, std::forward_as_tuple(name),
                         std::forward_as_tuple(parent, namedChain));
}

std::shared_ptr<Chain> Chain::getEntry(Iptables &parent,
                                       const ChainNameEnum &name) {
  // This method retrieves the pointer to Chain object specified by its keys.
  if (parent.chains_.count(name) == 0) {
    throw std::runtime_error("There is no chain " +
                             ChainJsonObject::ChainNameEnum_to_string(name));
  }

  return std::shared_ptr<Chain>(&parent.chains_.at(name), [](Chain *) {});
}

void Chain::removeEntry(Iptables &parent, const ChainNameEnum &name) {
  // This method removes the single Chain object specified by its keys.
  // Remember to call here the remove static method for all-sub-objects of
  // Chain.
  throw std::runtime_error("[Chain]: Method removeEntry not supported");
}

std::vector<std::shared_ptr<Chain>> Chain::get(Iptables &parent) {
  // This methods get the pointers to all the Chain objects in Iptables.
  std::vector<std::shared_ptr<Chain>> chains;
  for (auto &it : parent.chains_) {
    chains.push_back(Chain::getEntry(parent, it.first));
  }
  return chains;
}

void Chain::remove(Iptables &parent) {
  // This method removes all Chain objects in Iptables.
  // Remember to call here the remove static method for all-sub-objects of
  // Chain.
  throw std::runtime_error("[Chain]: Method remove not supported");
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
    parent_.reloadChain(name);
    if (getName() == ChainNameEnum::OUTPUT) {
      if (parent_.programs_.find(std::make_pair(
              ModulesConstants::CHAINFORWARDER_EGRESS,
              ChainNameEnum::INVALID_EGRESS)) != parent_.programs_.end()) {
        parent_
            .programs_[std::make_pair(ModulesConstants::CHAINFORWARDER_EGRESS,
                                     ChainNameEnum::INVALID_EGRESS)]
            ->reload();
      }
      if (parent_.programs_.find(std::make_pair(
              ModulesConstants::CHAINSELECTOR_EGRESS,
              ChainNameEnum::INVALID_EGRESS)) != parent_.programs_.end()) {
        parent_
            .programs_[std::make_pair(ModulesConstants::CHAINSELECTOR_EGRESS,
                                     ChainNameEnum::INVALID_EGRESS)]
            ->reload();
      }
    } else if (getName() == ChainNameEnum::INPUT ||
               getName() == ChainNameEnum::FORWARD) {
      if (parent_.programs_.find(std::make_pair(
              ModulesConstants::CHAINFORWARDER_INGRESS,
              ChainNameEnum::INVALID_INGRESS)) != parent_.programs_.end()) {
        parent_
            .programs_[std::make_pair(ModulesConstants::CHAINFORWARDER_INGRESS,
                                     ChainNameEnum::INVALID_INGRESS)]
            ->reload();
      }
      if (parent_.programs_.find(std::make_pair(
              ModulesConstants::CHAINSELECTOR_INGRESS,
              ChainNameEnum::INVALID_INGRESS)) != parent_.programs_.end()) {
        parent_
            .programs_[std::make_pair(ModulesConstants::CHAINSELECTOR_INGRESS,
                                     ChainNameEnum::INVALID_INGRESS)]
            ->reload();
      }
    }
  } catch (std::runtime_error re) {
    logger()->error(
        "[{0}] Can't reload the code for default action. Error: {1} ",
        parent_.getName(), re.what());
    return;
  }
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
  if (input.inIfaceIsSet()) {
    conf.setInIface(input.getInIface());
  }
  if (input.outIfaceIsSet()) {
    conf.setOutIface(input.getOutIface());
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

  // get rule id to append rule
  uint32_t id = rules_.size();

  // invoke create method with automatically generated first free rule id
  ChainRule::create(*this, id, conf);

  // set fields for return object
  ChainAppendOutputJsonObject result;
  // result.setId(id);

  if (parent_.interactive_) {
    ChainRule::applyAcceptEstablishedOptimization(*this);
    parent_.attachInterfaces();
  }

  return result;
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
  if (input.inIfaceIsSet()) {
    conf.setInIface(input.getInIface());
  }
  if (input.outIfaceIsSet()) {
    conf.setOutIface(input.getOutIface());
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

  // invoke create method with automatically generated first free rule id
  ChainRule::insert(*this, id, conf);
  if (parent_.interactive_) {
    ChainRule::applyAcceptEstablishedOptimization(*this);
  }

  // set fields for return object
  ChainInsertOutputJsonObject result;
  // result.setId(id);

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
  if (input.inIfaceIsSet()) {
    conf.setInIface(input.getInIface());
  }
  if (input.outIfaceIsSet()) {
    conf.setOutIface(input.getOutIface());
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

  try {
    // invoke create method with automatically generated first free rule id
    ChainRule::deletes(*this, conf);
  } catch (...) {
    throw std::runtime_error("No matching rule to delete");
  }
  ChainRule::applyAcceptEstablishedOptimization(*this);
}

ChainResetCountersOutputJsonObject Chain::resetCounters() {
  ChainResetCountersOutputJsonObject result;
  try {
    std::map<std::pair<uint8_t, ChainNameEnum>, Iptables::Program *> &programs =
        parent_.programs_;

    if (programs.find(std::make_pair(ModulesConstants::ACTION, name)) ==
        programs.end()) {
      throw std::runtime_error("No action loaded yet.");
    }

    auto actionProgram = dynamic_cast<Iptables::ActionLookup *>(
        programs[std::make_pair(ModulesConstants::ACTION, name)]);

    for (auto cr : rules_) {
      actionProgram->flushCounters(cr->getId());
    }
    counters_.clear();
    result.setResult(true);
  } catch (std::exception &e) {
    logger()->error("[{0}] Flushing counters error: {1} ", parent_.getName(),
                    e.what());
    result.setResult(false);
  }
  return result;
}

// apply rules action is per-chain action
ChainApplyRulesOutputJsonObject Chain::applyRules() {
  ChainApplyRulesOutputJsonObject result;
  try {
    updateChain();
    result.setResult(true);
  } catch (...) {
    result.setResult(false);
  }
  ChainRule::applyAcceptEstablishedOptimization(*this);
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

/* UPDATE CHAIN */

// updates dataplane and control plane strcuts and datapaths
// when rules are changed
void Chain::updateChain() {
  logger()->info("[{0}] Starting to update the {1} chain for {2} rules...",
                 parent_.get_name(),
                 ChainJsonObject::ChainNameEnum_to_string(name), rules_.size());

  auto start = std::chrono::high_resolution_clock::now();

  // Programs indexes
  // Look at defines.h

  int index;
  if (name == ChainNameEnum::INPUT) {
    // chainNumber (0/1) switching each time
    index = ModulesConstants::NR_INITIAL_MODULES +
            (chainNumber * ModulesConstants::NR_MODULES);
  }
  if (name == ChainNameEnum::FORWARD) {
    // chainNumber (0/1) switching each time
    index = ModulesConstants::NR_INITIAL_MODULES +
            chainNumber * ModulesConstants::NR_MODULES +
            ModulesConstants::NR_MODULES * 2;
  }
  if (name == ChainNameEnum::OUTPUT) {
    // chainNumber (0/1) switching each time
    index = ModulesConstants::NR_INITIAL_MODULES +
            chainNumber * ModulesConstants::NR_MODULES;
  }

  // srarting index found
  int startingIndex = index;
  // save first program loaded, since parser needs to know how to point to it
  Iptables::Program *firstProgramLoaded;

  std::map<std::pair<uint8_t, ChainNameEnum>, Iptables::Program *>
      newProgramsChain;

  // containing various bitvector(s)
  std::map<uint8_t, std::vector<uint64_t>> conntrack_map;
  std::map<struct IpAddr, std::vector<uint64_t>> ipsrc_map;
  std::map<struct IpAddr, std::vector<uint64_t>> ipdst_map;
  std::map<uint16_t, std::vector<uint64_t>> portsrc_map;
  std::map<uint16_t, std::vector<uint64_t>> portdst_map;
  std::map<uint16_t, std::vector<uint64_t>> interface_map;
  std::map<int, std::vector<uint64_t>> protocol_map;
  std::vector<std::vector<uint64_t>> flags_map;

  std::map<struct HorusRule, struct HorusValue> horus;

  /*
   * HORUS - Homogeneous RUleset analySis
   *
   * Horus optimization allows to
   * a) offload a group of contiguous rules matching on same field
   * b) match the group of offloaded rules with complexity O(1) - single map lookup
   * c) dynamically adapting to different groups of rules, matching each combination of ipsrc/dst, portsrc/dst, tcpflags
   * d) dynamically check when the optimization is possible according to current ruleset. It means check orthogonality
   * of rules before the offloaded group, respect to the group itself.
   *
   * -Working on INPUT chain, if no FORWARD rules are present.
   * TODO: put Horus after chainselector, so it can work only on INPUT pkts without possible semantic issues.
   *
   * each pkt received by the program, is looked-up vs the HORUS HASHMAP.
   * hit ->
   * DROP action: drop the packet;
   * ACCEPT action: goto CTLABELING and CTTABLEUPDATE without going through pipeline
   * miss ->
   * GOTO all pipeline steps
   */

  parent_.horus_runtime_enabled_ = false;

  // Apply Horus optimization only if we are updating INPUT chain
  if ((name == ChainNameEnum::INPUT) && (parent_.horus_enabled)) {
    // if len INPUT >= MIN_RULES_HORUS_OPTIMIZATION
    // if len FORWARD == 0
    if ((getRuleList().size() >= HorusConst::MIN_RULE_SIZE_FOR_HORUS) &&
        (parent_.getChain(ChainNameEnum::FORWARD)->getRuleList().size() == 0)) {
      // calculate horus ruleset
      horusFromRulesToMap(horus, getRuleList());

      // if horus.size() >= MIN_RULES_HORUS_OPTIMIZATION
      if (horus.size() >= HorusConst::MIN_RULE_SIZE_FOR_HORUS) {
        logger()->info("Horus Optimization ENABLED for this rule-set");

        // horus_runtime_enabled_ = true -> (Parser should access to
        // this var to compile itself)
        parent_.horus_runtime_enabled_ = true;

        // SWAP indexes
        parent_.horus_swap_ = !parent_.horus_swap_;

        uint8_t horus_index_new = -1;
        uint8_t horus_index_old = -1;

        // Apply Horus mitigator

        // Calculate current new/old indexes
        if (parent_.horus_swap_) {
          horus_index_new = ModulesConstants::HORUS_INGRESS_SWAP;
          horus_index_old = ModulesConstants::HORUS_INGRESS;
        } else {
          horus_index_old = ModulesConstants::HORUS_INGRESS_SWAP;
          horus_index_new = ModulesConstants::HORUS_INGRESS;
        }

        // Compile and inject program (Horus should have 1 static var
        // to switch index)

        // Horus Constructor is in charge to compile datapath with correct const
        parent_.programs_.insert(
            std::pair<std::pair<uint8_t, ChainNameEnum>, Iptables::Program *>(
                std::make_pair(horus_index_new, ChainNameEnum::INVALID_INGRESS),
                new Iptables::Horus(horus_index_new, parent_, horus)));

        // Horus UpdateMap is in charge to update maps
        dynamic_cast<Iptables::Horus *>(
            parent_.programs_[std::make_pair(horus_index_new,
                                            ChainNameEnum::INVALID_INGRESS)])
            ->updateMap(horus);

        // Recompile parser
        // parser should ask to Horus its index getIndex from Horus
        parent_
            .programs_[std::make_pair(ModulesConstants::PARSER_INGRESS,
                                     ChainNameEnum::INVALID_INGRESS)]
            ->reload();

        // Delete old Horus, if present
        auto it = parent_.programs_.find(
            std::make_pair(horus_index_old, ChainNameEnum::INVALID_INGRESS));
        if (it != parent_.programs_.end()) {
          delete it->second;
          parent_.programs_.erase(it);
        }
      }
    }
  }
  if (parent_.horus_runtime_enabled_ == false) {
    // Recompile parser
    // parser should ask to Horus its index getIndex from Horus
    parent_.programs_[std::make_pair(ModulesConstants::PARSER_INGRESS,ChainNameEnum::INVALID_INGRESS)]->reload();

    // Delete old Horus, if present
    auto it = parent_.programs_.find(
            std::make_pair(ModulesConstants::HORUS_INGRESS, ChainNameEnum::INVALID_INGRESS));
    if (it != parent_.programs_.end()) {
      delete it->second;
      parent_.programs_.erase(it);
    }

    // Delete old Horus, if present
    it = parent_.programs_.find(
            std::make_pair(ModulesConstants::HORUS_INGRESS_SWAP, ChainNameEnum::INVALID_INGRESS));
    if (it != parent_.programs_.end()) {
      delete it->second;
      parent_.programs_.erase(it);
    }
  }

  // calculate bitvectors, and check if no wildcard is present.
  // if no wildcard is present, we can early break the pipeline.
  // so we put modules with _break flags, before the others in order
  // to maximize probability to early break the pipeline.
  bool conntrack_break =
          conntrackFromRulesToMap(conntrack_map, getRuleList());
  bool ipsrc_break =
          ipFromRulesToMap(SOURCE_TYPE, ipsrc_map, getRuleList());
  bool ipdst_break =
          ipFromRulesToMap(DESTINATION_TYPE, ipdst_map, getRuleList());
  bool protocol_break =
          transportProtoFromRulesToMap(protocol_map, getRuleList());
  bool portsrc_break =
          portFromRulesToMap(SOURCE_TYPE, portsrc_map, getRuleList());
  bool portdst_break =
          portFromRulesToMap(DESTINATION_TYPE, portdst_map, getRuleList());
  bool interface_break =
          interfaceFromRulesToMap( (name == ChainNameEnum::OUTPUT) ? OUT_TYPE : IN_TYPE, interface_map, getRuleList(), parent_);
  bool flags_break = flagsFromRulesToMap(flags_map, getRuleList());

  logger()->debug(
      "Early break of pipeline conntrack:{0} ipsrc:{1} ipdst:{2} protocol:{3} "
      "portstc:{4} portdst:{5} interface:{6} flags:{7} ",
      conntrack_break, ipsrc_break, ipdst_break, protocol_break, portsrc_break,
      portdst_break, interface_break, flags_break);

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
      newProgramsChain.insert(
          std::pair<std::pair<uint8_t, ChainNameEnum>, Iptables::Program *>(
              std::make_pair(ModulesConstants::CONNTRACKMATCH, name),
              new Iptables::ConntrackMatch(index, name, this->parent_)));
      // Now the program is loaded, populate it.
      dynamic_cast<Iptables::ConntrackMatch *>(
          newProgramsChain[std::make_pair(ModulesConstants::CONNTRACKMATCH,
                                          name)])
          ->updateMap(conntrack_map);

      // This check is not really needed here, it will always be the first
      // module
      // to be injected
      if (index == startingIndex) {
        firstProgramLoaded = newProgramsChain[std::make_pair(
            ModulesConstants::CONNTRACKMATCH, name)];
      }
      logger()->trace("Conntrack index:{0}", index);
      ++index;
    }
    // conntrack_map.clear();
    // Done looping through conntrack

    // Looping through IP source
    // utils.h

    // Maps to populate
    // SRC/DST
    // map to populate
    // rules, as received by API object
    if (!ipsrc_map.empty() && (ipsrc_break ^ second)) {
      // At least one rule requires a matching on ipsource, so inject
      // the module on the first available position
      newProgramsChain.insert(
          std::pair<std::pair<uint8_t, ChainNameEnum>, Iptables::Program *>(
              std::make_pair(ModulesConstants::IPSOURCE, name),
              // also injecting porgram
              new Iptables::IpLookup(index, name, SOURCE_TYPE, this->parent_)));
      // If this is the first module, adjust parsing to forward to it.
      if (index == startingIndex) {
        firstProgramLoaded =
            newProgramsChain[std::make_pair(ModulesConstants::IPSOURCE, name)];
      }
      logger()->trace("IP Src index:{0}", index);
      ++index;

      // Now the program is loaded, populate it.
      dynamic_cast<Iptables::IpLookup *>(
          newProgramsChain[std::make_pair(ModulesConstants::IPSOURCE, name)])
          ->updateMap(ipsrc_map);
    }
    // ipsrc_map.clear();
    // Done looping through IP source

    // Looping through IP destination
    if (!ipdst_map.empty() && ipdst_break ^ second) {
      // At least one rule requires a matching on source ip, so inject the
      // module on the first available position
      newProgramsChain.insert(
          std::pair<std::pair<uint8_t, ChainNameEnum>, Iptables::Program *>(
              std::make_pair(ModulesConstants::IPDESTINATION, name),
              new Iptables::IpLookup(index, name, DESTINATION_TYPE,
                                     this->parent_)));
      // If this is the first module, adjust parsing to forward to it.
      if (index == startingIndex) {
        firstProgramLoaded = newProgramsChain[std::make_pair(
            ModulesConstants::IPDESTINATION, name)];
      }
      logger()->trace("IP Dst index:{0}", index);
      ++index;

      // Now the program is loaded, populate it.
      dynamic_cast<Iptables::IpLookup *>(
          newProgramsChain[std::make_pair(ModulesConstants::IPDESTINATION,
                                          name)])
          ->updateMap(ipdst_map);
    }
    // ipdst_map.clear();
    // Done looping through IP destination

    // Looping through l4 protocol
    if (!protocol_map.empty() && protocol_break ^ second) {
      // At least one rule requires a matching on
      // source ports, so inject the module
      // on the first available position
      newProgramsChain.insert(
          std::pair<std::pair<uint8_t, ChainNameEnum>, Iptables::Program *>(
              std::make_pair(ModulesConstants::L4PROTO, name),
              new Iptables::L4ProtocolLookup(index, name, this->parent_)));

      // If this is the first module, adjust parsing to forward to it.
      if (index == startingIndex) {
        firstProgramLoaded =
            newProgramsChain[std::make_pair(ModulesConstants::L4PROTO, name)];
      }
      logger()->trace("Protocol index:{0}", index);
      ++index;

      // Now the program is loaded, populate it.
      dynamic_cast<Iptables::L4ProtocolLookup *>(
          newProgramsChain[std::make_pair(ModulesConstants::L4PROTO, name)])
          ->updateMap(protocol_map);
    }
    // protocol_map.clear();
    // Done looping through l4 protocol

    // Looping through source port
    if (!portsrc_map.empty() && portsrc_break ^ second) {
      // At least one rule requires a matching on  source ports,
      // so inject the  module  on the first available position
      newProgramsChain.insert(
          std::pair<std::pair<uint8_t, ChainNameEnum>, Iptables::Program *>(
              std::make_pair(ModulesConstants::PORTSOURCE, name),
              new Iptables::L4PortLookup(index, name, SOURCE_TYPE,
                                         this->parent_, portsrc_map)));

      // If this is the first module, adjust parsing to forward to it.
      if (index == startingIndex) {
        firstProgramLoaded = newProgramsChain[std::make_pair(
            ModulesConstants::PORTSOURCE, name)];
      }
      logger()->trace("Port Src index:{0}", index);
      ++index;

      // Now the program is loaded, populate it.
      dynamic_cast<Iptables::L4PortLookup *>(
          newProgramsChain[std::make_pair(ModulesConstants::PORTSOURCE, name)])
          ->updateMap(portsrc_map);
    }
    // portsrc_map.clear();
    // Done looping through source port

    // Looping through destination port
    if (!portdst_map.empty() && portdst_break ^ second) {
      // At least one rule requires a matching on source ports,
      // so inject the module  on the first available position
      newProgramsChain.insert(
          std::pair<std::pair<uint8_t, ChainNameEnum>, Iptables::Program *>(
              std::make_pair(ModulesConstants::PORTDESTINATION, name),
              new Iptables::L4PortLookup(index, name, DESTINATION_TYPE,
                                         this->parent_, portdst_map)));
      // If this is the first module, adjust
      // parsing to forward to it.
      if (index == startingIndex) {
        firstProgramLoaded = newProgramsChain[std::make_pair(
            ModulesConstants::PORTDESTINATION, name)];
      }
      logger()->trace("Port Dst index:{0}", index);
      ++index;

      // Now the program is loaded, populate it.
      dynamic_cast<Iptables::L4PortLookup *>(
          newProgramsChain[std::make_pair(ModulesConstants::PORTDESTINATION,
                                          name)])
          ->updateMap(portdst_map);
    }
    // portdst_map.clear();
    // Done looping through destination port

    // Looping through interface
    if (!interface_map.empty() && interface_break ^ second) {
      // At least one rule requires a matching on interface,
      // so inject the  module  on the first available position
      newProgramsChain.insert(
              std::pair<std::pair<uint8_t, ChainNameEnum>, Iptables::Program *>(
                      std::make_pair(ModulesConstants::INTERFACE, name),
                      new Iptables::InterfaceLookup(index, name, (name == ChainNameEnum::OUTPUT) ? OUT_TYPE : IN_TYPE,
                                                 this->parent_, interface_map)));

      // If this is the first module, adjust parsing to forward to it.
      if (index == startingIndex) {
        firstProgramLoaded = newProgramsChain[std::make_pair(
                ModulesConstants::INTERFACE, name)];
      }
      logger()->trace("Interface index:{0}", index);
      ++index;

      // Now the program is loaded, populate it.
      dynamic_cast<Iptables::InterfaceLookup *>(
              newProgramsChain[std::make_pair(ModulesConstants::INTERFACE, name)])
              ->updateMap(interface_map);
    }
    // Done looping through interface

    // Looping through tcp flags
    if (!flags_map.empty() && flags_break ^ second) {
      // At least one rule requires a matching on flags,
      // so inject the  module in the first available position
      newProgramsChain.insert(
          std::pair<std::pair<uint8_t, ChainNameEnum>, Iptables::Program *>(
              std::make_pair(ModulesConstants::TCPFLAGS, name),
              new Iptables::TcpFlagsLookup(index, name, this->parent_)));

      // If this is the first module, adjust parsing to forward to it.
      if (index == startingIndex) {
        firstProgramLoaded =
            newProgramsChain[std::make_pair(ModulesConstants::TCPFLAGS, name)];
      }
      logger()->trace("Flags index:{0}", index);
      ++index;

      // Now the program is loaded, populate it.
      dynamic_cast<Iptables::TcpFlagsLookup *>(
          newProgramsChain[std::make_pair(ModulesConstants::TCPFLAGS, name)])
          ->updateMap(flags_map);
    }
    // flags_map.clear();
    // Done looping through tcp flags
  }

  // Adding bitscan
  newProgramsChain.insert(
      std::pair<std::pair<uint8_t, ChainNameEnum>, Iptables::Program *>(
          std::make_pair(ModulesConstants::BITSCAN, name),
          new Iptables::BitScan(index, name, this->parent_)));
  // If this is the first module, adjust parsing to forward to it.
  if (index == startingIndex) {
    firstProgramLoaded =
        newProgramsChain[std::make_pair(ModulesConstants::BITSCAN, name)];
  }
  ++index;

  // Adding action taker
  newProgramsChain.insert(
      std::pair<std::pair<uint8_t, ChainNameEnum>, Iptables::Program *>(
          std::make_pair(ModulesConstants::ACTION, name),
          new Iptables::ActionLookup(index, name, this->parent_)));

  for (auto rule : getRuleList()) {
    dynamic_cast<Iptables::ActionLookup *>(
        newProgramsChain[std::make_pair(ModulesConstants::ACTION, name)])
        ->updateTableValue(rule->getId(),
                           ChainRule::ActionEnumToInt(rule->getAction()));
  }

  uint8_t chainfwd;
  uint8_t chainparser;
  uint8_t chainselector;
  ChainNameEnum chainnameenum;

  if (getName() == ChainNameEnum::OUTPUT) {
    chainfwd = ModulesConstants::CHAINFORWARDER_EGRESS;
    chainparser = ModulesConstants::PARSER_EGRESS;
    chainselector = ModulesConstants::CHAINSELECTOR_EGRESS;
    chainnameenum = ChainNameEnum::INVALID_EGRESS;
  } else {
    chainfwd = ModulesConstants::CHAINFORWARDER_INGRESS;
    chainparser = ModulesConstants::PARSER_INGRESS;
    chainselector = ModulesConstants::CHAINSELECTOR_INGRESS;
    chainnameenum = ChainNameEnum::INVALID_INGRESS;
  }

  parent_.programs_[std::make_pair(chainfwd, chainnameenum)]->updateHop(
      1, firstProgramLoaded, name);

  parent_.programs_[std::make_pair(chainfwd, chainnameenum)]->reload();

  parent_.programs_[std::make_pair(chainselector, chainnameenum)]->reload();

  parent_.programs_[std::make_pair(chainparser, chainnameenum)]->reload();

  // Unload the programs belonging to the old chain.
  // Implicit in destructors
  // Except parser for OUTPUT chain
  for (auto it = parent_.programs_.begin(); it != parent_.programs_.end();) {
    if (it->first.second == name) {
      delete it->second;
      it = parent_.programs_.erase(it);
    } else {
      ++it;
    }
  }

  // Copy the new program references to the main map.
  for (auto &program : newProgramsChain) {
    parent_
        .programs_[std::make_pair(program.first.first, program.first.second)] =
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
