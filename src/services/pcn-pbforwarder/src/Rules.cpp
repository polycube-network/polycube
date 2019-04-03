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

#include "Rules.h"
#include "Pbforwarder.h"

#include "polycube/services/utils.h"

using namespace polycube::service;

Rules::Rules(Pbforwarder &parent, const RulesJsonObject &conf)
    : parent_(parent), id(conf.getId()) {
  logger()->info("Creating Rules instance");
}

Rules::~Rules() {}

void Rules::update(const RulesJsonObject &conf) {
  updating = true;
  /*TODO: Improve code.
   The updating flag indicates to the setters to not interact with the datapath.
   In this way only one update will be done, by the update method itself.*/
  try {
    if (conf.srcMacIsSet()) {
      setSrcMac(conf.getSrcMac());
    }
    if (conf.dstMacIsSet()) {
      setDstMac(conf.getDstMac());
    }
    if (conf.vlanIsSet()) {
      setVlan(conf.getVlan());
    }

    if (conf.srcIpIsSet()) {
      setSrcIp(conf.getSrcIp());
    }
    if (conf.dstIpIsSet()) {
      setDstIp(conf.getDstIp());
    }

    if (conf.l4ProtoIsSet()) {
      setL4Proto(conf.getL4Proto());
    }

    if (conf.srcPortIsSet()) {
      setSrcPort(htons(conf.getSrcPort()));
    }

    if (conf.dstPortIsSet()) {
      setDstPort(htons(conf.getDstPort()));
    }

    if (conf.inPortIsSet()) {
      setInPort(conf.getInPort());
    }

    if (conf.actionIsSet()) {
      setAction(conf.getAction());
    }

    if (conf.outPortIsSet()) {
      setOutPort(conf.getOutPort());
    }
    // Update the datapath
    updateRuleMapAndReload();

    updating = false;
  } catch (std::runtime_error re) {
    // It is important that in case of exception the updating flag goes to
    // false, otherwise setters won't work when called singularly.
    updating = false;
    throw re;
  }
}

RulesJsonObject Rules::toJsonObject() {
  RulesJsonObject conf;

  try {
    conf.setDstMac(getDstMac());
  } catch (...) {
  }

  try {
    conf.setSrcPort(getSrcPort());
  } catch (...) {
  }

  try {
    conf.setVlan(getVlan());
  } catch (...) {
  }

  try {
    conf.setOutPort(getOutPort());
  } catch (...) {
  }

  try {
    conf.setId(getId());
  } catch (...) {
  }

  try {
    conf.setSrcIp(getSrcIp());
  } catch (...) {
  }

  try {
    conf.setDstPort(getDstPort());
  } catch (...) {
  }

  try {
    conf.setAction(getAction());
  } catch (...) {
  }

  try {
    conf.setDstIp(getDstIp());
  } catch (...) {
  }

  try {
    conf.setL4Proto(getL4Proto());
  } catch (...) {
  }

  try {
    conf.setSrcMac(getSrcMac());
  } catch (...) {
  }

  try {
    conf.setInPort(getInPort());
  } catch (...) {
  }

  return conf;
}

std::string Rules::getDstMac() {
  // This method retrieves the dstMac value.
  if (!IS_SET(flags, 1))
    throw std::runtime_error("[Rules]: DstMac not set");
  return dstMac;
}

void Rules::setDstMac(const std::string &value) {
  SET_BIT(flags, 1);
  dstMac = value;
  if (!updating)
    updateRuleMapAndReload();
}

uint16_t Rules::getSrcPort() {
  if (!IS_SET(flags, 6))
    throw std::runtime_error("[Rules]: SrcPort not set");
  return srcPort;
}

void Rules::setSrcPort(const uint16_t &value) {
  try {
    getL4Proto();
  } catch (...) {
    throw std::runtime_error(
        "Level 4 protocol is needed when level 4 ports are specified.\n");
  }
  SET_BIT(flags, 6);
  srcPort = value;
  if (!updating)
    updateRuleMapAndReload();
}

uint32_t Rules::getVlan() {
  if (!IS_SET(flags, 2))
    throw std::runtime_error("[Rules]: Vlan not specified");
  else
    return vlan;
}

void Rules::setVlan(const uint32_t &value) {
  SET_BIT(flags, 2);
  vlan = value;
  if (!updating)
    updateRuleMapAndReload();
}

std::string Rules::getOutPort() {
  if (outPort == "-1")
    throw std::runtime_error("[Rules]: No outPort specified");
  return outPort;
}

void Rules::setOutPort(const std::string &value) {
  outPort = value;
  if (!updating)
    updateRuleMapAndReload();
}

uint32_t Rules::getId() {
  return id;
}

std::string Rules::getSrcIp() {
  if (!IS_SET(flags, 3))
    throw std::runtime_error("[Rules]: SrcIp not set");
  return srcIp;
}

void Rules::setSrcIp(const std::string &value) {
  SET_BIT(flags, 3);
  srcIp = value;
  if (!updating)
    updateRuleMapAndReload();
}

uint16_t Rules::getDstPort() {
  if (!IS_SET(flags, 7))
    throw std::runtime_error("[Rules]: DstPort not set");
  return dstPort;
}

void Rules::setDstPort(const uint16_t &value) {
  try {
    getL4Proto();
  } catch (...) {
    throw std::runtime_error(
        "Level 4 protocol is needed when level 4 ports are specified.\n");
  }
  SET_BIT(flags, 7);
  dstPort = value;
  if (!updating)
    updateRuleMapAndReload();
}

RulesActionEnum Rules::getAction() {
  return action;
}

void Rules::setAction(const RulesActionEnum &value) {
  action = value;
  if (!updating)
    updateRuleMapAndReload();
}

std::string Rules::getDstIp() {
  if (!IS_SET(flags, 4))
    throw std::runtime_error("[Rules]: DstIp not specified");
  return dstIp;
}

void Rules::setDstIp(const std::string &value) {
  SET_BIT(flags, 4);
  dstIp = value;
  if (!updating)
    updateRuleMapAndReload();
}

RulesL4ProtoEnum Rules::getL4Proto() {
  if (!IS_SET(flags, 5))
    throw std::runtime_error("[Rules]: L4Proto not set.");
  return L4Proto;
}

void Rules::setL4Proto(const RulesL4ProtoEnum &value) {
  SET_BIT(flags, 5);
  L4Proto = value;
  if (!updating)
    updateRuleMapAndReload();
}

std::string Rules::getSrcMac() {
  if (!IS_SET(flags, 0))
    throw std::runtime_error("[Rules]: SrcMac not set.");
  return srcMac;
}

void Rules::setSrcMac(const std::string &value) {
  SET_BIT(flags, 0);
  srcMac = value;
  if (!updating)
    updateRuleMapAndReload();
}

std::string Rules::getInPort() {
  if (!IS_SET(flags, 8))
    throw std::runtime_error("[Rules]: InPort not set.");
  return inPort;
}

void Rules::setInPort(const std::string &value) {
  SET_BIT(flags, 8);
  inPort = value;
  if (!updating)
    updateRuleMapAndReload();
}

std::shared_ptr<spdlog::logger> Rules::logger() {
  return parent_.logger();
}

rule Rules::to_rule() {
  uint16_t inPortIndex = 0;
  uint16_t outPortIndex = 0;
  try {
    inPortIndex = parent_.get_port(inPort)->index();
  } catch (...) {
  }

  try {
    outPortIndex = parent_.get_port(outPort)->index();
  } catch (...) {
  }

  rule r{
      .srcMac = utils::mac_string_to_be_uint(srcMac),
      .dstMac = utils::mac_string_to_be_uint(dstMac),
      .vlanid = vlan,
      .srcIp = utils::ip_string_to_be_uint(srcIp),
      .dstIp = utils::ip_string_to_be_uint(dstIp),
      .lvl4_proto = rulesL4ProtoEnum_to_int(L4Proto),
      .src_port = srcPort,
      .dst_port = dstPort,
      .inport = inPortIndex,
      .action = rulesActionEnum_to_int(action),
      .outport = outPortIndex,
      .flags = flags,
  };

  return r;
}

uint16_t Rules::rulesL4ProtoEnum_to_int(RulesL4ProtoEnum proto) {
  if (proto == RulesL4ProtoEnum::UDP) {
    return IPPROTO_UDP;
  } else if (proto == RulesL4ProtoEnum::TCP) {
    return IPPROTO_TCP;
  } else {
    throw std::runtime_error("No valid level 4 protocol");
  }
}

uint16_t Rules::rulesActionEnum_to_int(RulesActionEnum action) {
  if (action == RulesActionEnum::DROP) {
    return 0;
  } else if (action == RulesActionEnum::SLOWPATH) {
    return 1;
  } else if (action == RulesActionEnum::FORWARD) {
    return 2;
  } else {
    throw std::runtime_error("No valid action");
  }
}

void Rules::updateRuleMapAndReload() {
  int oldMatchLevel = parent_.match_level;
  int oldNrRules = parent_.nr_rules;

  if (flags >= 8 && flags < 32 && parent_.match_level < 3) {
    /*
     * In the bitmap only one or more of the first 5 fields is set,
     * this means that the rule is only about lvl 2 and lvl 3.
     * In general, no level 4 rule has been set up to now.
     *
     * This is the first rule that requires IP matching.
     * The code to match the two levels is loaded.
     */

    parent_.logger()->info("[{0}] Switching to level 3.",
                           parent_.Cube::get_name());
    parent_.match_level = 3;

  } else if (flags >= 32 && parent_.match_level < 4) {
    /*
     * In the bitmap only one or more of the first 9 fields is set,
     * this means that the rule is about lvl 2, 3, 4.
     *
     * This is the first rule that requires full matching.
     * The code to match the two levels is loaded.
     */

    parent_.logger()->debug("[{0}] Switching to level 4.",
                            parent_.Cube::get_name());
    parent_.match_level = 4;
  }

  auto rules_table = parent_.get_hash_table<uint32_t, rule>("rules", 1);
  rules_table.set(id, to_rule());

  if (id >= parent_.nr_rules) {
    parent_.nr_rules = id + 1;
  }

  try {
    parent_.reload(parent_.generate_code_matching(), 1);
    parent_.reload(parent_.generate_code_parsing(), 0);
    parent_.logger()->debug("[{0}] Success inserting the rule.",
                            parent_.Cube::get_name());
    auto it = parent_.rules_.find(id);
    if (it == parent_.rules_.end()) {
      parent_.rules_.insert(std::pair<uint32_t, Rules>(id, *this));
    }

  } catch (std::runtime_error re) {
    rules_table.remove(id);
    if (parent_.match_level != oldMatchLevel ||
        parent_.nr_rules != oldNrRules) {
      parent_.match_level = oldMatchLevel;
      parent_.nr_rules = oldNrRules;
      parent_.reload(parent_.generate_code_parsing(), 0);
      parent_.reload(parent_.generate_code_matching(), 1);
    }

    parent_.logger()->error(
        "[{0}] Can't reload the code. Maybe too many rules? \n{1}",
        parent_.Cube::get_name(), re.what());
  }
}
