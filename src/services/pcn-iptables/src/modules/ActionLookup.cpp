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
#include "datapaths/Iptables_ActionLookup_dp.h"
#include "polycube/common.h"

Iptables::ActionLookup::ActionLookup(const int &index,
                                     const ChainNameEnum &chain,
                                     Iptables &outer)
    : Iptables::Program(iptables_code_actionlookup, index, chain, outer,
                        (chain == ChainNameEnum::OUTPUT)
                            ? ProgramType::EGRESS
                            : ProgramType::INGRESS) {
  load();
}

Iptables::ActionLookup::~ActionLookup() {}

std::string Iptables::ActionLookup::getCode() {
  std::string no_macro_code = code_;

  /*Replacing the maximum number of rules*/
  replaceAll(no_macro_code, "_MAXRULES",
             std::to_string(FROM_NRULES_TO_NELEMENTS(iptables_.max_rules_)));

  /*Replacing nrElements*/
  replaceAll(no_macro_code, "_NR_ELEMENTS",
             std::to_string(FROM_NRULES_TO_NELEMENTS(
                 iptables_.getChain(chain_)->getNrRules())));

  bool ingress = true;

  /*Replacing direction suffix*/
  if (chain_ == ChainNameEnum::INPUT)
    replaceAll(no_macro_code, "_DIRECTION", "Input");
  if (chain_ == ChainNameEnum::FORWARD)
    replaceAll(no_macro_code, "_DIRECTION", "Forward");
  if (chain_ == ChainNameEnum::OUTPUT) {
    replaceAll(no_macro_code, "_DIRECTION", "Output");
    ingress = false;
  }

  if (ingress) {
    /*Pointing to the module in charge of updating the conn table and
     * forwarding*/
    replaceAll(no_macro_code, "_CONNTRACKTABLEUPDATE",
               std::to_string(ModulesConstants::CONNTRACKTABLEUPDATE_INGRESS));
  } else {
    /*Pointing to the module in charge of updating the conn table and
     * forwarding*/
    replaceAll(no_macro_code, "_CONNTRACKTABLEUPDATE",
               std::to_string(ModulesConstants::CONNTRACKTABLEUPDATE_EGRESS));
  }

  if (program_type_ == ProgramType::INGRESS) {
    replaceAll(no_macro_code, "call_bpf_program", "call_ingress_program");
  } else if (program_type_ == ProgramType::EGRESS) {
    replaceAll(no_macro_code, "call_bpf_program", "call_egress_program");
  }

  /*Ingress or Egress logic*/
  if (program_type_ == ProgramType::INGRESS) {
    replaceAll(no_macro_code, "_HOOK", "INGRESS");
  } else if (program_type_ == ProgramType::EGRESS) {
    replaceAll(no_macro_code, "_HOOK", "EGRESS");
  }

  return no_macro_code;
}

uint64_t Iptables::ActionLookup::getPktsCount(int rule_number) {
  std::string table_name = "pkts";

  if (chain_ == ChainNameEnum::INPUT)
    table_name += "Input";
  else if (chain_ == ChainNameEnum::FORWARD)
    table_name += "Forward";
  else if (chain_ == ChainNameEnum::OUTPUT)
    table_name += "Output";

  try {
    std::lock_guard<std::mutex> guard(program_mutex_);
    uint64_t pkts = 0;

    auto pkts_table = iptables_.get_percpuarray_table<uint64_t>(
        table_name, index_, program_type_);
    auto values = pkts_table.get(rule_number);

    return std::accumulate(values.begin(), values.end(), pkts);
  } catch (...) {
    throw std::runtime_error("Counter not available.");
  }
}

uint64_t Iptables::ActionLookup::getBytesCount(int ruleNumber) {
  std::string table_name = "bytes";

  if (chain_ == ChainNameEnum::INPUT)
    table_name += "Input";
  else if (chain_ == ChainNameEnum::FORWARD)
    table_name += "Forward";
  else if (chain_ == ChainNameEnum::OUTPUT)
    table_name += "Output";

  try {
    std::lock_guard<std::mutex> guard(program_mutex_);
    auto bytes_table = iptables_.get_percpuarray_table<uint64_t>(
        table_name, index_, program_type_);
    auto values = bytes_table.get(ruleNumber);
    uint64_t bytes = 0;
    return std::accumulate(values.begin(), values.end(), bytes);
  } catch (...) {
    throw std::runtime_error("Counter not available.");
  }
}

void Iptables::ActionLookup::flushCounters(int rule_number) {
  std::string pkts_table_name = "pkts";
  std::string bytes_table_name = "bytes";

  if (chain_ == ChainNameEnum::INPUT) {
    pkts_table_name += "Input";
    bytes_table_name += "Input";
  } else if (chain_ == ChainNameEnum::FORWARD) {
    pkts_table_name += "Forward";
    bytes_table_name += "Forward";
  } else if (chain_ == ChainNameEnum::OUTPUT) {
    pkts_table_name += "Output";
    bytes_table_name += "Output";
  }

  try {
    auto pkts_table = iptables_.get_percpuarray_table<uint64_t>(
        pkts_table_name, index_, program_type_);
    auto bytes_table = iptables_.get_percpuarray_table<uint64_t>(
        bytes_table_name, index_, program_type_);

    pkts_table.set(rule_number, 0);
    bytes_table.set(rule_number, 0);
  } catch (std::exception &e) {
    throw std::runtime_error("Counters not available: " +
                             std::string(e.what()));
  }
}

bool Iptables::ActionLookup::updateTableValue(int rule_number, int action) {
  std::string table_name = "actions";

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
    auto actions_table =
        iptables_.get_array_table<int>(table_name, index_, program_type_);
    actions_table.set(rule_number, action);
  } catch (...) {
    return false;
  }
  return true;
}
