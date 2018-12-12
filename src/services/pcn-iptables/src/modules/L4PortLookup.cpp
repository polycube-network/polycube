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

#include "../Iptables.h"
#include "datapaths/Iptables_L4PortLookup_dp.h"

Iptables::L4PortLookup::L4PortLookup(const int &index,
                                     const ChainNameEnum &chain,
                                     const int &type, Iptables &outer)
    : Iptables::Program(iptables_code_l4portlookup, index, chain, outer,
                        (chain == ChainNameEnum::OUTPUT)
                            ? ProgramType::EGRESS
                            : ProgramType::INGRESS) {
  this->type_ = type;

  wildcard_rule_ = false;
  wildcard_string_ = "";

  load();
}

Iptables::L4PortLookup::L4PortLookup(
    const int &index, const ChainNameEnum &chain, const int &type,
    Iptables &outer, const std::map<uint16_t, std::vector<uint64_t>> &ports)
    : Iptables::Program(iptables_code_l4portlookup, index, chain, outer,
                        (chain == ChainNameEnum::OUTPUT)
                            ? ProgramType::EGRESS
                            : ProgramType::INGRESS) {
  this->type_ = type;

  auto it = ports.find(0);
  if (it == ports.end()) {
    wildcard_rule_ = false;
    wildcard_string_ = "";
    // std::cout << "-- wildcard NO --+" << std::endl;
  } else {
    wildcard_rule_ = true;
    wildcard_string_ = fromContainerToMapString(it->second.begin(),
                                              it->second.end(), "{", "}", ",");
    // std::cout << "-- wildcard YES --+" << std::endl;
    // std::cout << wildcardString << std::endl;
  }

  load();
}

Iptables::L4PortLookup::~L4PortLookup() {}

std::string Iptables::L4PortLookup::getCode() {
  std::string no_macro_code = code_;

  /*Replacing the maximum number of rules*/
  replaceAll(no_macro_code, "_MAXRULES",
             std::to_string(FROM_NRULES_TO_NELEMENTS(iptables_.max_rules_)));

  /*Replacing hops*/
  replaceAll(no_macro_code, "_NEXT_HOP_1", std::to_string(index_ + 1));

  /*Replacing nrElements*/
  replaceAll(no_macro_code, "_NR_ELEMENTS",
             std::to_string(FROM_NRULES_TO_NELEMENTS(
                 iptables_.getChain(chain_)->getNrRules())));

  /*Replacing direction suffix*/
  if (chain_ == ChainNameEnum::INPUT)
    replaceAll(no_macro_code, "_DIRECTION", "Input");
  if (chain_ == ChainNameEnum::FORWARD)
    replaceAll(no_macro_code, "_DIRECTION", "Forward");
  if (chain_ == ChainNameEnum::OUTPUT)
    replaceAll(no_macro_code, "_DIRECTION", "Output");

  /*Replacing type*/
  if (type_ == SOURCE_TYPE)
    replaceAll(no_macro_code, "_TYPE", "src");
  else
    replaceAll(no_macro_code, "_TYPE", "dst");

  /*Replacing the default action*/
  replaceAll(no_macro_code, "_DEFAULTACTION", defaultActionString());

  if (wildcard_rule_) {
    replaceAll(no_macro_code, "_WILDCARD_RULE", std::to_string(1));
    replaceAll(no_macro_code, "_WILDCARD_BITVECTOR", wildcard_string_);
  } else {
    replaceAll(no_macro_code, "_WILDCARD_RULE", std::to_string(0));
  }

  if (program_type_ == ProgramType::INGRESS) {
    replaceAll(no_macro_code, "call_bpf_program", "call_ingress_program");
  } else if (program_type_ == ProgramType::EGRESS) {
    replaceAll(no_macro_code, "call_bpf_program", "call_egress_program");
  }

  return no_macro_code;
}

bool Iptables::L4PortLookup::updateTableValue(
    uint16_t port, const std::vector<uint64_t> &value) {
  std::string table_name;

  if (type_ == SOURCE_TYPE) {
    table_name += "src";
  } else if (type_ == DESTINATION_TYPE) {
    table_name += "dst";
  } else {
    return false;
  }
  table_name += "Ports";

  if (chain_ == ChainNameEnum::INPUT)
    table_name += "Input";
  else if (chain_ == ChainNameEnum::FORWARD)
    table_name += "Forward";
  else if (chain_ == ChainNameEnum::OUTPUT)
    table_name += "Output";
  else
    return false;
  try {
    std::lock_guard<std::mutex> guard(program_mutex_);
    auto table = iptables_.get_raw_table(table_name, index_, program_type_);
    table.set(&port, value.data());
  } catch (...) {
    return false;
  }
  return true;
}

void Iptables::L4PortLookup::updateMap(
    const std::map<uint16_t, std::vector<uint64_t>> &ports) {
  for (auto ele : ports) {
    // std::cout << "+ele.second (btv.size) " << ele.second.size() << std::endl;
    // std::cout << "+FROM_NRULES_TO_NELEMENTS(iptables.max_rules_) : " <<
    // FROM_NRULES_TO_NELEMENTS(iptables.max_rules_) << std::endl;

    updateTableValue(ntohs(ele.first), ele.second);
  }
}
