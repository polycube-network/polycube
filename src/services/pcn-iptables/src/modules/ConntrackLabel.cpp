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
#include "datapaths/Iptables_ConntrackLabel_dp.h"

Iptables::ConntrackLabel::ConntrackLabel(const int &index, Iptables &outer,
                                         const ProgramType t)
    : Iptables::Program(iptables_code_conntracklabel, index,
                        (t == ProgramType::INGRESS)
                            ? ChainNameEnum::INVALID_INGRESS
                            : ChainNameEnum::INVALID_EGRESS,
                        outer, t) {
  load();
}

Iptables::ConntrackLabel::~ConntrackLabel() {}

std::vector<std::pair<ct_k, ct_v>> Iptables::ConntrackLabel::getMap() {
  auto table =
      iptables_.get_hash_table<ct_k, ct_v>("connections", index_, program_type_);
  return table.get_all();
}

uint64_t Iptables::ConntrackLabel::getAcceptEstablishedPktsCount(
    ChainNameEnum chain) {
  std::string table_name = "pkts_acceptestablished_";

  if (chain == ChainNameEnum::INPUT)
    table_name += "Input";
  else if (chain == ChainNameEnum::FORWARD)
    table_name += "Forward";
  else if (chain == ChainNameEnum::OUTPUT)
    table_name += "Output";

  try {
    std::lock_guard<std::mutex> guard(program_mutex_);
    uint64_t pkts = 0;

    auto pkts_table =
        iptables_.get_percpuarray_table<uint64_t>(table_name, index_, program_type_);
    auto values = pkts_table.get(0);

    return std::accumulate(values.begin(), values.end(), pkts);
  } catch (...) {
    throw std::runtime_error("Counter not available.");
  }
}

uint64_t Iptables::ConntrackLabel::getAcceptEstablishedBytesCount(
    ChainNameEnum chain) {
  std::string table_name = "bytes_acceptestablished_";

  if (chain == ChainNameEnum::INPUT)
    table_name += "Input";
  else if (chain == ChainNameEnum::FORWARD)
    table_name += "Forward";
  else if (chain == ChainNameEnum::OUTPUT)
    table_name += "Output";

  try {
    std::lock_guard<std::mutex> guard(program_mutex_);
    auto bytes_table =
        iptables_.get_percpuarray_table<uint64_t>(table_name, index_, program_type_);
    auto values = bytes_table.get(0);
    uint64_t bytes = 0;
    return std::accumulate(values.begin(), values.end(), bytes);
  } catch (...) {
    throw std::runtime_error("Counter not available.");
  }
}

void Iptables::ConntrackLabel::flushCounters(ChainNameEnum chain,
                                             int rule_number) {
  std::string pkts_table_name = "pkts_acceptestablished_";
  std::string bytes_table_name = "bytes_acceptestablished_";

  if (chain == ChainNameEnum::INPUT) {
    pkts_table_name += "Input";
    bytes_table_name += "Input";
  } else if (chain == ChainNameEnum::FORWARD) {
    pkts_table_name += "Forward";
    bytes_table_name += "Forward";
  } else if (chain == ChainNameEnum::OUTPUT) {
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

std::string Iptables::ConntrackLabel::getCode() {
  std::string no_macro_code = code_;

  /*Ingress or Egress logic*/
  if (program_type_ == ProgramType::INGRESS) {
    // if this is a parser for INPUT hook:
    replaceAll(no_macro_code, "_INGRESS_LOGIC", std::to_string(1));
    replaceAll(no_macro_code, "_EGRESS_LOGIC", std::to_string(0));

    /*Pointing to the module in charge of updating the conn table and
     * forwarding*/
    replaceAll(no_macro_code, "_CONNTRACKTABLEUPDATE",
               std::to_string(ModulesConstants::CONNTRACKTABLEUPDATE_INGRESS));
  } else if (program_type_ == ProgramType::EGRESS) {
    // if this is a parser for OUTPUT hook:
    replaceAll(no_macro_code, "_INGRESS_LOGIC", std::to_string(0));
    replaceAll(no_macro_code, "_EGRESS_LOGIC", std::to_string(1));

    /*Pointing to the module in charge of updating the conn table and
     * forwarding*/
    replaceAll(no_macro_code, "_CONNTRACKTABLEUPDATE",
               std::to_string(ModulesConstants::CONNTRACKTABLEUPDATE_EGRESS));
  }

  /*Replacing the maximum number of rules*/
  replaceAll(no_macro_code, "_MAXRULES",
             std::to_string(FROM_NRULES_TO_NELEMENTS(iptables_.max_rules_)));

  // /*Replacing hops*/
  // replaceAll(noMacroCode, "_NEXT_HOP_1", std::to_string(index + 1));

  /* Replacing next hops_*/
  if (program_type_ == ProgramType::INGRESS)
    replaceAll(no_macro_code, "_CHAINFORWARDER",
               std::to_string(ModulesConstants::CHAINFORWARDER_INGRESS));
  else if (program_type_ == ProgramType::EGRESS)
    replaceAll(no_macro_code, "_CHAINFORWARDER",
               std::to_string(ModulesConstants::CHAINFORWARDER_EGRESS));

  replaceAll(no_macro_code, "_CONNTRACK_MODE_INPUT",
             std::to_string(iptables_.conntrack_mode_input_));
  replaceAll(no_macro_code, "_CONNTRACK_MODE_FORWARD",
             std::to_string(iptables_.conntrack_mode_forward_));
  replaceAll(no_macro_code, "_CONNTRACK_MODE_OUTPUT",
             std::to_string(iptables_.conntrack_mode_output_));

  if (program_type_ == ProgramType::INGRESS) {
    replaceAll(no_macro_code, "call_bpf_program", "call_ingress_program");
  } else if (program_type_ == ProgramType::EGRESS) {
    replaceAll(no_macro_code, "call_bpf_program", "call_egress_program");
  }

  return no_macro_code;
}
