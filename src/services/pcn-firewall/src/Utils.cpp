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

#include "ChainRule.h"
#include "Firewall.h"

int Firewall::protocol_from_string_to_int(const std::string &proto) {
  if (proto == "TCP")
    return IPPROTO_TCP;
  if (proto == "UDP")
    return IPPROTO_UDP;
  if (proto == "ICMP")
    return IPPROTO_ICMP;
  if (proto == "GRE")
    return IPPROTO_GRE;
  else
    throw std::runtime_error("Protocol not supported.");
}

void Firewall::replaceAll(std::string &str, const std::string &from,
                          const std::string &to) {
  if (from.empty())
    return;
  size_t start_pos = 0;
  while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
    str.replace(start_pos, from.length(), to);
    start_pos += to.length();  // In case 'to' contains 'from', like replacing
                               // 'x' with 'yx'
  }
}

int ChainRule::protocol_from_string_to_int(const std::string &proto) {
  if (proto == "TCP")
    return IPPROTO_TCP;
  if (proto == "UDP")
    return IPPROTO_UDP;
  if (proto == "ICMP")
    return IPPROTO_ICMP;
  if (proto == "GRE")
    return IPPROTO_GRE;

  throw std::runtime_error("Protocol not supported.");
  return 0;
}

std::string ChainRule::protocol_from_int_to_string(const int &proto) {
  if (proto == IPPROTO_TCP)
    return "TCP";
  if (proto == IPPROTO_UDP)
    return "UDP";
  if (proto == IPPROTO_ICMP)
    return "ICMP";
  if (proto == IPPROTO_GRE)
    return "GRE";

  throw std::runtime_error("Protocol not supported.");
  return "";
}

void ChainRule::flags_from_string_to_masks(const std::string &flags,
                                           uint8_t &flagsSet,
                                           uint8_t &flagsNotSet) {
  std::string modFlags = flags;
  flagsNotSet = 0;
  if (modFlags.find("!FIN") != std::string::npos) {
    modFlags.erase(modFlags.find("!FIN"), 4);
    SET_BIT(flagsNotSet, 0);
  }
  if (modFlags.find("!SYN") != std::string::npos) {
    modFlags.erase(modFlags.find("!SYN"), 4);
    SET_BIT(flagsNotSet, 1);
  }
  if (modFlags.find("!RST") != std::string::npos) {
    modFlags.erase(modFlags.find("!RST"), 4);
    SET_BIT(flagsNotSet, 2);
  }
  if (modFlags.find("!PSH") != std::string::npos) {
    modFlags.erase(modFlags.find("!PSH"), 4);
    SET_BIT(flagsNotSet, 3);
  }
  if (modFlags.find("!ACK") != std::string::npos) {
    modFlags.erase(modFlags.find("!ACK"), 4);
    SET_BIT(flagsNotSet, 4);
  }
  if (modFlags.find("!URG") != std::string::npos) {
    modFlags.erase(modFlags.find("!URG"), 4);
    SET_BIT(flagsNotSet, 5);
  }
  if (modFlags.find("!ECE") != std::string::npos) {
    modFlags.erase(modFlags.find("!ECE"), 4);
    SET_BIT(flagsNotSet, 6);
  }
  if (modFlags.find("!CWR") != std::string::npos) {
    modFlags.erase(modFlags.find("!CWR"), 4);
    SET_BIT(flagsNotSet, 7);
  }

  flagsSet = 0;
  if (modFlags.find("FIN") != std::string::npos) {
    SET_BIT(flagsSet, 0);
  }
  if (modFlags.find("SYN") != std::string::npos) {
    SET_BIT(flagsSet, 1);
  }
  if (modFlags.find("RST") != std::string::npos) {
    SET_BIT(flagsSet, 2);
  }
  if (modFlags.find("PSH") != std::string::npos) {
    SET_BIT(flagsSet, 3);
  }
  if (modFlags.find("ACK") != std::string::npos) {
    SET_BIT(flagsSet, 4);
  }
  if (modFlags.find("URG") != std::string::npos) {
    SET_BIT(flagsSet, 5);
  }
  if (modFlags.find("ECE") != std::string::npos) {
    SET_BIT(flagsSet, 6);
  }
  if (modFlags.find("CWR") != std::string::npos) {
    SET_BIT(flagsSet, 7);
  }

  if ((flagsSet & flagsNotSet) != 0) {
    throw std::runtime_error("A flag can't be both set and not set!");
  }
}

void ChainRule::flags_from_masks_to_string(std::string &flags,
                                           const uint8_t &flagsSet,
                                           const uint8_t &flagsNotSet) {
  if (flagsSet != 0) {
    if (CHECK_BIT(flagsSet, 0)) {
      flags += "FIN ";
    }
    if (CHECK_BIT(flagsSet, 1)) {
      flags += "SYN ";
    }
    if (CHECK_BIT(flagsSet, 2)) {
      flags += "RST ";
    }
    if (CHECK_BIT(flagsSet, 3)) {
      flags += "PSH ";
    }
    if (CHECK_BIT(flagsSet, 4)) {
      flags += "ACK ";
    }
    if (CHECK_BIT(flagsSet, 5)) {
      flags += "URG ";
    }
    if (CHECK_BIT(flagsSet, 6)) {
      flags += "ECE ";
    }
    if (CHECK_BIT(flagsSet, 7)) {
      flags += "CWR ";
    }
  }

  if (flagsNotSet != 0) {
    if (CHECK_BIT(flagsNotSet, 0)) {
      flags += "!FIN ";
    }
    if (CHECK_BIT(flagsNotSet, 1)) {
      flags += "!SYN ";
    }
    if (CHECK_BIT(flagsNotSet, 2)) {
      flags += "!RST ";
    }
    if (CHECK_BIT(flagsNotSet, 3)) {
      flags += "!PSH ";
    }
    if (CHECK_BIT(flagsNotSet, 4)) {
      flags += "!ACK ";
    }
    if (CHECK_BIT(flagsNotSet, 5)) {
      flags += "!URG ";
    }
    if (CHECK_BIT(flagsNotSet, 6)) {
      flags += "!ECE ";
    }
    if (CHECK_BIT(flagsNotSet, 7)) {
      flags += "!CWR ";
    }
  }
}

int ChainRule::ActionEnum_to_int(const ActionEnum &action) {
  if (action == ActionEnum::DROP)
    return DROP_ACTION;
  else if (action == ActionEnum::FORWARD)
    return FORWARD_ACTION;
  else
    throw std::runtime_error("Action not supported.");
}

static int ChainRuleConntrackEnum_to_int(const ConntrackstatusEnum &status) {
  // 0 is reserved for "wildcard"
  if (status == ConntrackstatusEnum::NEW) {
    return 1;
  } else if (status == ConntrackstatusEnum::ESTABLISHED) {
    return 2;
  } else if (status == ConntrackstatusEnum::RELATED) {
    return 3;
  } else if (status == ConntrackstatusEnum::INVALID) {
    return 4;
  }
}

// convert ip address list from internal rules representation, to Api
// representation
bool Chain::ipFromRulesToMap(
        const uint8_t &type, std::map<struct IpAddr, std::vector<uint64_t>> &ips,
        const std::vector<std::shared_ptr<ChainRule>> &rules) {
  // track if, at least, one wildcard rule is present
  std::vector<uint32_t> dont_care_rules;

  // current rule ip and id
  struct IpAddr current;
  uint32_t rule_id;

  bool brk = true;

  // std::cout<< "+ITERATING ON RULES+ Adding ips in map (except 0.0.0.0 handled
  // later)";

  // iterate over all rules
  // and keep track of different ips (except 0.0.0.0 hadled later)
  // push distinct ips as keys in ips map
  for (auto const &rule : rules) {
    try {
      if (type == SOURCE_TYPE) {
        current.fromString(rule->getSrc());
        // std::cout << "SRC IP RULE | ";
      } else {
        current.fromString(rule->getDst());
        // std::cout << "DST IP RULE | ";
      }
    } catch (std::runtime_error re) {
      // IP not set: don't care rule.
      dont_care_rules.push_back(rule->getId());
      // std::cout << "IP RULE DONT CARE | ID: " << rule->getId();
      continue;
    }
    rule_id = rule->getId();
    // std::cout << "ID: " << ruleId << " Current IP: " << current.toString() <<
    // std::endl;
    auto it = ips.find(current);
    if (it == ips.end()) {
      // std::cout << "FIRST TIME I SEE THIS IP -> insert " << std::endl;
      std::vector<uint64_t> bitVector(
              FROM_NRULES_TO_NELEMENTS(Firewall::maxRules));
      current.ruleId = rule_id;
      ips.insert(
              std::pair<struct IpAddr, std::vector<uint64_t>>(current, bitVector));

      //   for (auto eval : ips) {
      //     std::cout << eval.first.toString() << ": ";
      //     std::cout << std::bitset<32>(eval.second[0]) << " ";
      //     std::cout << std::endl;
      //     std::cout << std::endl;
      //  }
    }
  }

  // For each ip in ips map
  for (auto &eval : ips) {
    auto &address = eval.first;
    auto &bitVector = eval.second;

    struct IpAddr current_rule_ip;
    uint32_t current_rule_id;

    // For each rule (if don't care rule, use 0.0.0.0), then apply netmask,
    // compare and SET_BIT if match
    for (auto const &rule : rules) {
      try {
        if (type == SOURCE_TYPE) {
          current_rule_ip.fromString(rule->getSrc());
          // std::cout << "SRC IP RULE | ";
        } else {
          current_rule_ip.fromString(rule->getDst());
          // std::cout << "DST IP RULE | ";
        }
      } catch (std::runtime_error re) {
        // IP not set: don't care rule.
        current_rule_ip.fromString("0.0.0.0/0");
        // std::cout << "IP RULE DONT CARE | ID: " << rule->getId();
      }

      current_rule_id = rule->getId();
      // std::cout << "ID: " << currentRuleId << " Current rule IP: " <<
      // currentRuleIp.toString() << std::endl;
      auto netmask = (current_rule_ip.netmask);
      auto mask = (netmask == 32 ? 0xffffffff : (((uint32_t)1 << netmask) - 1));

      // std::cout << "ADDRESS(fixed): " << address.toString() << "
      // RULE-IP(loop):" << currentRuleIp.toString() << std::endl;
      // std::cout << "netmask: " << netmask << "mask: " <<
      // std::bitset<32>(mask) << std::endl;

      if (((address.ip & mask) == (current_rule_ip.ip & mask)) &&
          (current_rule_ip.netmask <= address.netmask)) {
        // std::cout << "Set BIT " << std::endl;
        SET_BIT(bitVector[current_rule_id / 63], current_rule_id % 63);
      }
    }
  }

  // std::cout << "ips.size: " << ips.size() << " dontCareRules.size(): " <<
  // dontCareRules.size() << std::endl;

  // Don't care rules are in all entries. Anyway, this loops is useless if there
  // are no rules at all requiring matching on this field.
  if (ips.size() != 0 && dont_care_rules.size() != 0) {
    // std::cout << "++ ADDING 0.0.0.0/0 rule :)" << std::endl;

    std::vector<uint64_t> bitVector(
            FROM_NRULES_TO_NELEMENTS(Firewall::maxRules));
    struct IpAddr wildcard_ip = {0, 0};

    auto &address = wildcard_ip;

    struct IpAddr current_rule_ip;
    uint32_t current_rule_id;

    for (auto const &rule : rules) {
      try {
        if (type == SOURCE_TYPE) {
          current_rule_ip.fromString(rule->getSrc());
          // std::cout << "SRC IP RULE | ";
        } else {
          current_rule_ip.fromString(rule->getDst());
          // std::cout << "DST IP RULE | ";
        }
      } catch (std::runtime_error re) {
        // IP not set: don't care rule.
        current_rule_ip.fromString("0.0.0.0/0");
        // std::cout << "IP RULE DONT CARE | ID: " << rule->getId();
      }

      current_rule_id = rule->getId();
      // std::cout << "ID: " << currentRuleId << " Current rule IP: " <<
      // currentRuleIp.toString() << std::endl;
      auto netmask = (current_rule_ip.netmask);
      auto mask = (netmask == 32 ? 0xffffffff : (((uint32_t)1 << netmask) - 1));

      // std::cout << "ADDRESS(fixed): " << address.toString() << "
      // RULE-IP(loop):" << currentRuleIp.toString() << std::endl;
      // std::cout << "netmask: " << netmask << "mask: " <<
      // std::bitset<32>(mask) << std::endl;

      if (((address.ip & mask) == (current_rule_ip.ip & mask)) &&
          (current_rule_ip.netmask <= address.netmask)) {
        // std::cout << "Set BIT " << std::endl;
        SET_BIT(bitVector[current_rule_id / 63], current_rule_id % 63);
      }
    }

    ips.insert(std::pair<struct IpAddr, std::vector<uint64_t>>(wildcard_ip,
                                                               bitVector));
    brk = false;
  }

  return brk;

  // std::cout << "++ END ++ " << std::endl;

  // for (auto eval : ips) {
  //   std::cout << eval.first.toString() << ": ";
  //   std::cout << std::bitset<32>(eval.second[0]) << " ";
  //   std::cout << std::endl;
  // }
}

bool Chain::transportProtoFromRulesToMap(
        std::map<int, std::vector<uint64_t>> &protocols,
        const std::vector<std::shared_ptr<ChainRule>> &rules) {
  std::vector<uint32_t> dont_care_rules;

  int proto;
  uint32_t rule_id;

  bool brk = true;

  for (auto const &rule : rules) {
    try {
      rule_id = rule->getId();
      proto = Firewall::protocol_from_string_to_int(rule->getL4proto());
    } catch (std::runtime_error re) {
      dont_care_rules.push_back(rule_id);
      continue;
    }
    auto it = protocols.find(proto);
    if (it == protocols.end()) {
      // First entry
      std::vector<uint64_t> bitVector(
              FROM_NRULES_TO_NELEMENTS(Firewall::maxRules));
      SET_BIT(bitVector[rule_id / 63], rule_id % 63);
      protocols.insert(std::pair<int, std::vector<uint64_t>>(proto, bitVector));
    } else {
      SET_BIT((it->second)[rule_id / 63], rule_id % 63);
    }
  }
  // Don't care rules are in all entries. Anyway, this loops is useless if there
  // are no rules at all requiring matching on this field.
  if (protocols.size() != 0 && dont_care_rules.size() != 0) {
    std::vector<uint64_t> bitVector(
            FROM_NRULES_TO_NELEMENTS(Firewall::maxRules));
    protocols.insert(std::pair<int, std::vector<uint64_t>>(0, bitVector));
    for (auto const &ruleNumber : dont_care_rules) {
      for (auto &proto : protocols) {
        SET_BIT((proto.second)[ruleNumber / 63], ruleNumber % 63);
      }
    }
    brk = false;
  }
  return brk;
}


bool Chain::portFromRulesToMap(
        const uint8_t &type, std::map<uint16_t, std::vector<uint64_t>> &ports,
        const std::vector<std::shared_ptr<ChainRule>> &rules) {
  std::vector<uint32_t> dont_care_rules;

  uint32_t rule_id;
  uint16_t port;

  bool brk = true;

  for (auto const &rule : rules) {
    try {
      rule_id = rule->getId();
      if (type == SOURCE_TYPE)
        port = rule->getSport();
      else
        port = rule->getDport();
    } catch (std::runtime_error re) {
      // IP not set: don't care rule.
      dont_care_rules.push_back(rule_id);
      continue;
    }

    auto it = ports.find(port);
    if (it == ports.end()) {
      // First entry
      std::vector<uint64_t> bitVector(
              FROM_NRULES_TO_NELEMENTS(Firewall::maxRules));
      SET_BIT(bitVector[rule_id / 63], rule_id % 63);
      ports.insert(std::pair<uint16_t, std::vector<uint64_t>>(port, bitVector));
    } else {
      SET_BIT((it->second)[rule_id / 63], rule_id % 63);
    }
  }
  // Don't care rules are in all entries. Anyway, this loop is useless if there
  // are no rules at all requiring matching on this field.
  if (ports.size() != 0 && dont_care_rules.size() != 0) {
    std::vector<uint64_t> bitVector(
            FROM_NRULES_TO_NELEMENTS(Firewall::maxRules));
    ports.insert(std::pair<uint16_t, std::vector<uint64_t>>(0, bitVector));
    for (auto const &ruleNumber : dont_care_rules) {
      for (auto &port : ports) {
        SET_BIT((port.second)[ruleNumber / 63], ruleNumber % 63);
      }
    }
    brk = false;
  }
  return brk;
}

bool Chain::conntrackFromRulesToMap(
    std::map<uint8_t, std::vector<uint64_t>> &statusMap,
    const std::vector<std::shared_ptr<ChainRule>> &rules) {
  std::vector<uint16_t> dontCareRules;

  uint32_t ruleId;
  uint8_t status;
  for (auto const &rule : rules) {
    try {
      ruleId = rule->getId();
      status = ChainRuleConntrackEnum_to_int(rule->getConntrack());

    } catch (std::runtime_error re) {
      // Not set: don't care rule.
      dontCareRules.push_back(ruleId);
      continue;
    }

    auto it = statusMap.find(status);
    if (it == statusMap.end()) {
      // First entry
      std::vector<uint64_t> bitVector(
          FROM_NRULES_TO_NELEMENTS(Firewall::maxRules));
      SET_BIT(bitVector[ruleId / 63], ruleId % 63);
      statusMap.insert(
          std::pair<uint8_t, std::vector<uint64_t>>(status, bitVector));
    } else {
      SET_BIT((it->second)[ruleId / 63], ruleId % 63);
    }
  }
  // Don't care rules are in all entries. Anyway, this loop is useless if there
  // are no rules at all requiring matching on this field.
  if (statusMap.size() != 0 && dontCareRules.size() != 0) {
    std::vector<uint64_t> bitVector(
        FROM_NRULES_TO_NELEMENTS(Firewall::maxRules));
    statusMap.insert(std::pair<uint8_t, std::vector<uint64_t>>(0, bitVector));
    for (auto const &ruleNumber : dontCareRules) {
      for (auto &statusMapEntry : statusMap) {
        SET_BIT((statusMapEntry.second)[ruleNumber / 63], ruleNumber % 63);
      }
    }
  }

  return false;
}

bool Chain::flagsFromRulesToMap(
    std::vector<std::vector<uint64_t>> &flags,
    const std::vector<std::shared_ptr<ChainRule>> &rules) {
  flags.clear();
  // Preliminary check if there are rules requiring match on flags.
  bool areFlagsPresent = false;
  for (auto const &rule : rules) {
    try {
      rule->getTcpflags();
      areFlagsPresent = true;
      break;
    } catch (std::runtime_error re) {
      continue;
    }
  }
  if (!areFlagsPresent) {
    return false;
  }

  std::vector<uint64_t> bitVector(FROM_NRULES_TO_NELEMENTS(Firewall::maxRules));
  for (int j = 0; j < 256; ++j) {
    // Flags map is an ARRAY. All entries will be allocated, this requires to
    // populate the entire table.
    flags.push_back(bitVector);
  }
  uint8_t flagsSet;
  uint8_t flagsNotSet;
  uint32_t ruleId;

  for (auto const &rule : rules) {
    ruleId = rule->getId();
    try {
      ChainRule::flags_from_string_to_masks(rule->getTcpflags(), flagsSet,
                                            flagsNotSet);
    } catch (std::runtime_error re) {
      // Don't care rule: it has to have a 1 in every entry.
      for (auto &flagEntry : flags) {
        SET_BIT(flagEntry[ruleId / 63], ruleId % 63);
      }
      continue;
    }
    if (flagsSet == 0) {
      // Default value needed for the next computation.
      flagsSet = 255;
    }

    for (int j = 0; j < 256; ++j) {
      uint8_t candidateFlag =
          Firewall::TcpFlagsLookup::possibleFlagsCombinations[j];
      if (((candidateFlag & flagsSet) == flagsSet) &&
          ((candidateFlag & flagsNotSet) == 0)) {
        SET_BIT((flags[candidateFlag])[ruleId / 63], ruleId % 63);
      }
    }
  }

  // tcp flags current implementation is based on array.
  // no break optimization could be performed right now.
  return false;
}
