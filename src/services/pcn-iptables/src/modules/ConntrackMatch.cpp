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
#include "datapaths/Iptables_ConntrackMatch_dp.h"

Iptables::ConntrackMatch::ConntrackMatch(const int &index,
                                         const ChainNameEnum &chain,
                                         Iptables &outer)
    : Iptables::Program(iptables_code_conntrackmatch, index, chain, outer,
                        (chain == ChainNameEnum::OUTPUT)
                            ? ProgramType::EGRESS
                            : ProgramType::INGRESS) {
  load();
}

Iptables::ConntrackMatch::~ConntrackMatch() {}

std::string Iptables::ConntrackMatch::getCode() {
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

  /*Replacing the default action*/
  replaceAll(no_macro_code, "_DEFAULTACTION", defaultActionString());

  if (program_type_ == ProgramType::INGRESS) {
    replaceAll(no_macro_code, "call_bpf_program", "call_ingress_program");
  } else if (program_type_ == ProgramType::EGRESS) {
    replaceAll(no_macro_code, "call_bpf_program", "call_egress_program");
  }

  return no_macro_code;
}

bool Iptables::ConntrackMatch::updateTableValue(
    uint8_t status, const std::vector<uint64_t> &value) {
  std::string table_name;
  table_name = "Conntrack";

  if (chain_ == ChainNameEnum::INPUT) {
    table_name += "Input";
  } else if (chain_ == ChainNameEnum::FORWARD) {
    table_name += "Forward";
  } else if (chain_ == ChainNameEnum::OUTPUT) {
    table_name += "Output";
  } else
    return false;
  try {
    std::lock_guard<std::mutex> guard(program_mutex_);
    auto table = iptables_.get_raw_table(table_name, index_, program_type_);
    uint32_t s = status;
    table.set(&s, value.data());
  } catch (std::runtime_error e) {
    iptables_.logger()->error("{0}", e.what());
    return false;
  }
  return true;
}

void Iptables::ConntrackMatch::updateMap(
    const std::map<uint8_t, std::vector<uint64_t>> &status) {
  for (auto ele : status) {
    updateTableValue(ele.first, ele.second);
  }
}
