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

#include "Iptables.h"

int Iptables::protocolFromStringToInt(const std::string &proto) {
  if (proto == "TCP" || proto == "tcp")
    return IPPROTO_TCP;
  if (proto == "UDP" || proto == "udp")
    return IPPROTO_UDP;
  if (proto == "ICMP" || proto == "icmp")
    return IPPROTO_ICMP;
  if (proto == "GRE" || proto == "gre")
    return IPPROTO_GRE;
  else
    throw std::runtime_error("Protocol not supported.");
}

// used for replace strings in datapath code
void Iptables::replaceAll(std::string &str, const std::string &from,
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

int ChainRule::protocolFromStringToInt(const std::string &proto) {
  if (proto == "TCP" || proto == "tcp")
    return IPPROTO_TCP;
  if (proto == "UDP" || proto == "udp")
    return IPPROTO_UDP;
  if (proto == "ICMP" || proto == "icmp")
    return IPPROTO_ICMP;
  if (proto == "GRE" || proto == "gre")
    return IPPROTO_GRE;

  throw std::runtime_error("Protocol not supported.");
  return 0;
}

std::string ChainRule::protocolFromIntToString(const int &proto) {
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

void ChainRule::flagsFromStringToMasks(const std::string &flags,
                                       uint8_t &flags_set,
                                       uint8_t &flags_not_set) {
  std::string mod_flags = flags;
  flags_not_set = 0;
  if (mod_flags.find("!FIN") != std::string::npos) {
    mod_flags.erase(mod_flags.find("!FIN"), 4);
    SET_BIT(flags_not_set, 0);
  }
  if (mod_flags.find("!SYN") != std::string::npos) {
    mod_flags.erase(mod_flags.find("!SYN"), 4);
    SET_BIT(flags_not_set, 1);
  }
  if (mod_flags.find("!RST") != std::string::npos) {
    mod_flags.erase(mod_flags.find("!RST"), 4);
    SET_BIT(flags_not_set, 2);
  }
  if (mod_flags.find("!PSH") != std::string::npos) {
    mod_flags.erase(mod_flags.find("!PSH"), 4);
    SET_BIT(flags_not_set, 3);
  }
  if (mod_flags.find("!ACK") != std::string::npos) {
    mod_flags.erase(mod_flags.find("!ACK"), 4);
    SET_BIT(flags_not_set, 4);
  }
  if (mod_flags.find("!URG") != std::string::npos) {
    mod_flags.erase(mod_flags.find("!URG"), 4);
    SET_BIT(flags_not_set, 5);
  }
  if (mod_flags.find("!ECE") != std::string::npos) {
    mod_flags.erase(mod_flags.find("!ECE"), 4);
    SET_BIT(flags_not_set, 6);
  }
  if (mod_flags.find("!CWR") != std::string::npos) {
    mod_flags.erase(mod_flags.find("!CWR"), 4);
    SET_BIT(flags_not_set, 7);
  }

  flags_set = 0;
  if (mod_flags.find("FIN") != std::string::npos) {
    SET_BIT(flags_set, 0);
  }
  if (mod_flags.find("SYN") != std::string::npos) {
    SET_BIT(flags_set, 1);
  }
  if (mod_flags.find("RST") != std::string::npos) {
    SET_BIT(flags_set, 2);
  }
  if (mod_flags.find("PSH") != std::string::npos) {
    SET_BIT(flags_set, 3);
  }
  if (mod_flags.find("ACK") != std::string::npos) {
    SET_BIT(flags_set, 4);
  }
  if (mod_flags.find("URG") != std::string::npos) {
    SET_BIT(flags_set, 5);
  }
  if (mod_flags.find("ECE") != std::string::npos) {
    SET_BIT(flags_set, 6);
  }
  if (mod_flags.find("CWR") != std::string::npos) {
    SET_BIT(flags_set, 7);
  }

  if ((flags_set & flags_not_set) != 0) {
    throw std::runtime_error("A flag can't be both set and not set!");
  }
}

void ChainRule::flagsFromMasksToString(std::string &flags,
                                       const uint8_t &flags_set,
                                       const uint8_t &flags_not_set) {
  if (flags_set != 0) {
    if (CHECK_BIT(flags_set, 0)) {
      flags += "FIN ";
    }
    if (CHECK_BIT(flags_set, 1)) {
      flags += "SYN ";
    }
    if (CHECK_BIT(flags_set, 2)) {
      flags += "RST ";
    }
    if (CHECK_BIT(flags_set, 3)) {
      flags += "PSH ";
    }
    if (CHECK_BIT(flags_set, 4)) {
      flags += "ACK ";
    }
    if (CHECK_BIT(flags_set, 5)) {
      flags += "URG ";
    }
    if (CHECK_BIT(flags_set, 6)) {
      flags += "ECE ";
    }
    if (CHECK_BIT(flags_set, 7)) {
      flags += "CWR ";
    }
  }

  if (flags_not_set != 0) {
    if (CHECK_BIT(flags_not_set, 0)) {
      flags += "!FIN ";
    }
    if (CHECK_BIT(flags_not_set, 1)) {
      flags += "!SYN ";
    }
    if (CHECK_BIT(flags_not_set, 2)) {
      flags += "!RST ";
    }
    if (CHECK_BIT(flags_not_set, 3)) {
      flags += "!PSH ";
    }
    if (CHECK_BIT(flags_not_set, 4)) {
      flags += "!ACK ";
    }
    if (CHECK_BIT(flags_not_set, 5)) {
      flags += "!URG ";
    }
    if (CHECK_BIT(flags_not_set, 6)) {
      flags += "!ECE ";
    }
    if (CHECK_BIT(flags_not_set, 7)) {
      flags += "!CWR ";
    }
  }
}

int ChainRule::ActionEnumToInt(const ActionEnum &action) {
  if (action == ActionEnum::DROP)
    return DROP_ACTION;
  else if (action == ActionEnum::ACCEPT)
    return ACCEPT_ACTION;
  else
    throw std::runtime_error("Action not supported.");
}

static int ChainRuleConntrackEnumToInt(const ConntrackstatusEnum &status) {
  if (status == ConntrackstatusEnum::NEW) {
    return 0;
  } else if (status == ConntrackstatusEnum::ESTABLISHED) {
    return 1;
  } else if (status == ConntrackstatusEnum::RELATED) {
    return 2;
  } else if (status == ConntrackstatusEnum::INVALID) {
    return 3;
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
          FROM_NRULES_TO_NELEMENTS(Iptables::max_rules_));
      current.rule_id = rule_id;
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
        FROM_NRULES_TO_NELEMENTS(Iptables::max_rules_));
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

    ips.insert(
        std::pair<struct IpAddr, std::vector<uint64_t>>(wildcard_ip, bitVector));
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
      proto = Iptables::protocolFromStringToInt(rule->getL4proto());
    } catch (std::runtime_error re) {
      dont_care_rules.push_back(rule_id);
      continue;
    }
    auto it = protocols.find(proto);
    if (it == protocols.end()) {
      // First entry
      std::vector<uint64_t> bitVector(
          FROM_NRULES_TO_NELEMENTS(Iptables::max_rules_));
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
        FROM_NRULES_TO_NELEMENTS(Iptables::max_rules_));
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
          FROM_NRULES_TO_NELEMENTS(Iptables::max_rules_));
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
        FROM_NRULES_TO_NELEMENTS(Iptables::max_rules_));
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

bool Chain::interfaceFromRulesToMap(
        const uint8_t &type, std::map<uint16_t, std::vector<uint64_t>> &interfaces,
        const std::vector<std::shared_ptr<ChainRule>> &rules,
        Iptables& iptables) {
  std::vector<uint32_t> dont_care_rules;

  uint32_t rule_id;
  uint16_t interface;

  bool brk = true;

  for (auto const &rule : rules) {
    try {
      rule_id = rule->getId();
      interface = 0;
      if (type == IN_TYPE) {
        std::string interface_string = rule->getInIface();
        interface = iptables.interfaceNameToIndex(interface_string);
      } else {
        std::string interface_string = rule->getOutIface();
        interface = iptables.interfaceNameToIndex(interface_string);
      }
    } catch (std::runtime_error re) {
      // Interface not set: don't care rule.
      dont_care_rules.push_back(rule_id);
      continue;
    }

    auto it = interfaces.find(interface);
    if (it == interfaces.end()) {
      // First entry
      std::vector<uint64_t> bitVector(
              FROM_NRULES_TO_NELEMENTS(Iptables::max_rules_));
      SET_BIT(bitVector[rule_id / 63], rule_id % 63);
      interfaces.insert(std::pair<uint16_t, std::vector<uint64_t>>(interface, bitVector));
    } else {
      SET_BIT((it->second)[rule_id / 63], rule_id % 63);
    }
  }
  // Don't care rules are in all entries. Anyway, this loop is useless if there
  // are no rules at all requiring matching on this field.
  if (interfaces.size() != 0 && dont_care_rules.size() != 0) {
    std::vector<uint64_t> bitVector(
            FROM_NRULES_TO_NELEMENTS(Iptables::max_rules_));
    interfaces.insert(std::pair<uint16_t, std::vector<uint64_t>>(0, bitVector));
    for (auto const &ruleNumber : dont_care_rules) {
      for (auto &interface : interfaces) {
        SET_BIT((interface.second)[ruleNumber / 63], ruleNumber % 63);
      }
    }
    brk = false;
  }
  return brk;
}

void Chain::fromRuleToHorusKeyValue(std::shared_ptr<ChainRule> rule,
                                    struct HorusRule &key,
                                    struct HorusValue &value) {
  key.setFields = 0;

  try {
    IpAddr ips;
    ips.fromString(rule->getSrc());
    if (ips.netmask == 32) {
      SET_BIT(key.setFields, HorusConst::SRCIP);
      key.src_ip = ips.ip;
    }
  } catch (std::runtime_error) {
  }

  try {
    IpAddr ipd;
    ipd.fromString(rule->getDst());
    if (ipd.netmask == 32) {
      SET_BIT(key.setFields, HorusConst::DSTIP);
      key.dst_ip = ipd.ip;
    }
  } catch (std::runtime_error) {
  }

  try {
    uint8_t proto = Iptables::protocolFromStringToInt(rule->getL4proto());
    SET_BIT(key.setFields, HorusConst::L4PROTO);
    key.l4proto = proto;
  } catch (std::runtime_error) {
  }

  try {
    uint16_t srcport = rule->getSport();
    SET_BIT(key.setFields, HorusConst::SRCPORT);
    key.src_port = srcport;
  } catch (std::runtime_error) {
  }

  try {
    uint16_t dstport = rule->getDport();
    SET_BIT(key.setFields, HorusConst::DSTPORT);
    key.dst_port = dstport;
  } catch (std::runtime_error) {
  }

  // TODO Check if ACCEPT/DROP semantic is valid

  ActionEnum action = rule->getAction();

  if (action == ActionEnum::DROP)
    value.action = 0;
  else
    value.action = 1;

  value.ruleID = rule->getId();
  return;
}

void Chain::horusFromRulesToMap(
        std::map<struct HorusRule, struct HorusValue> &horus,
        const std::vector<std::shared_ptr<ChainRule>> &rules) {
  struct HorusRule key;
  struct HorusValue value;

  bool first_rule = true;
  uint64_t set_fields = 0;

  // find match pattern for first rule (e.g. 01101 means ips proto ports set)

  int i = 0;

  for (auto const &rule : rules) {
    i++;
    fromRuleToHorusKeyValue(rule, key, value);

    if (i == 1) {
      if (key.setFields == 0) {
        break;
      }

      set_fields = key.setFields;
    }

    if (key.setFields != set_fields) {
      break;
    }

    horus.insert(std::pair<struct HorusRule, struct HorusValue>(key, value));
  }
}

bool Chain::conntrackFromRulesToMap(
        std::map<uint8_t, std::vector<uint64_t>> &statusMap,
        const std::vector<std::shared_ptr<ChainRule>> &rules) {
  std::vector<uint8_t> statesVector({NEW, ESTABLISHED, RELATED, INVALID});
  uint32_t rule_id;
  uint8_t rule_state;
  bool conntrackRulePresent = false;

  for (auto const &rule : rules) {
    try {
      rule_state = ChainRuleConntrackEnumToInt(rule->getConntrack());
      conntrackRulePresent = true;
    } catch (...) {
    }
  }

  if (!conntrackRulePresent)
    return false;

  for (uint8_t state : statesVector) {
    std::vector<uint64_t> bitVector(
        FROM_NRULES_TO_NELEMENTS(Iptables::max_rules_));
    for (auto const &rule : rules) {
      try {
        rule_id = rule->getId();
        rule_state = ChainRuleConntrackEnumToInt(rule->getConntrack());
        if (rule_state == state)
          SET_BIT(bitVector[rule_id / 63], rule_id % 63);
      } catch (std::runtime_error re) {
        // wildcard rule, set bit to 1
        SET_BIT(bitVector[rule_id / 63], rule_id % 63);
      }
    }
    statusMap.insert(
        std::pair<uint8_t, std::vector<uint64_t>>(state, bitVector));
  }

  return false;
}

bool Chain::flagsFromRulesToMap(
        std::vector<std::vector<uint64_t>> &flags,
        const std::vector<std::shared_ptr<ChainRule>> &rules) {
  flags.clear();
  // Preliminary check if there are rules requiring match on flags.
  bool are_flags_present = false;
  for (auto const &rule : rules) {
    try {
      rule->getTcpflags();
      are_flags_present = true;
      break;
    } catch (std::runtime_error re) {
      continue;
    }
  }
  if (!are_flags_present) {
    return false;
  }

  std::vector<uint64_t> bitVector(FROM_NRULES_TO_NELEMENTS(Iptables::max_rules_));
  for (int j = 0; j < 256; ++j) {
    // Flags map is an ARRAY. All entries will be allocated, this requires to
    // populate the entire table.
    flags.push_back(bitVector);
  }
  uint8_t flags_set;
  uint8_t flags_not_set;
  uint32_t rule_id;

  for (auto const &rule : rules) {
    rule_id = rule->getId();
    try {
      ChainRule::flagsFromStringToMasks(rule->getTcpflags(), flags_set,
                                        flags_not_set);
    } catch (std::runtime_error re) {
      // Don't care rule: it has to have a 1 in every entry.
      for (auto &flag_entry : flags) {
        SET_BIT(flag_entry[rule_id / 63], rule_id % 63);
      }
      continue;
    }
    if (flags_set == 0) {
      // Default value needed for the next computation.
      flags_set = 255;
    }

    for (int j = 0; j < 256; ++j) {
      uint8_t candidate_flag =
          Iptables::TcpFlagsLookup::possible_flags_combinations_[j];
      if (((candidate_flag & flags_set) == flags_set) &&
          ((candidate_flag & flags_not_set) == 0)) {
        SET_BIT((flags[candidate_flag])[rule_id / 63], rule_id % 63);
      }
    }
  }

  // tcp flags current implementation is based on array.
  // no break optimization could be performed right now.
  return false;
}
