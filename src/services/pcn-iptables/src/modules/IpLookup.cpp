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
#include "datapaths/Iptables_IpLookup_dp.h"

using namespace polycube::service;

/* definitions copied from datapath */
struct lpm_k {
  uint32_t netmask_len;
  uint32_t ip;
} __attribute__((packed));

Iptables::IpLookup::IpLookup(const int &index, const ChainNameEnum &chain,
                             const int &type, Iptables &outer)
    : Iptables::Program(iptables_code_iplookup, index, chain, outer,
                        (chain == ChainNameEnum::OUTPUT)
                            ? ProgramType::EGRESS
                            : ProgramType::INGRESS) {
  this->type_ = type;
  load();
}

Iptables::IpLookup::~IpLookup() {}

std::string Iptables::IpLookup::getCode() {
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

  if (program_type_ == ProgramType::INGRESS) {
    replaceAll(no_macro_code, "call_bpf_program", "call_ingress_program");
  } else if (program_type_ == ProgramType::EGRESS) {
    replaceAll(no_macro_code, "call_bpf_program", "call_egress_program");
  }

  return no_macro_code;
}

void Iptables::IpLookup::updateTableValue(uint8_t netmask, std::string ip,
                                          const std::vector<uint64_t> &value) {
  std::string table_name = "ip";

  if (type_ == SOURCE_TYPE) {
    table_name += "src";
  } else if (type_ == DESTINATION_TYPE) {
    table_name += "dst";
  }
  table_name += "Trie";

  if (chain_ == ChainNameEnum::INPUT)
    table_name += "Input";
  if (chain_ == ChainNameEnum::FORWARD)
    table_name += "Forward";
  if (chain_ == ChainNameEnum::OUTPUT)
    table_name += "Output";

  lpm_k key{
      .netmask_len = netmask, .ip = utils::ip_string_to_be_uint(ip),
  };

  std::lock_guard<std::mutex> guard(program_mutex_);
  auto table = iptables_.get_raw_table(table_name, index_, program_type_);
  table.set(&key, value.data());
}

void Iptables::IpLookup::updateTableValue(IpAddr ip,
                                          const std::vector<uint64_t> &value) {
  std::string table_name = "ip";

  if (type_ == SOURCE_TYPE) {
    table_name += "src";
  } else if (type_ == DESTINATION_TYPE) {
    table_name += "dst";
  }
  table_name += "Trie";

  if (chain_ == ChainNameEnum::INPUT)
    table_name += "Input";
  if (chain_ == ChainNameEnum::FORWARD)
    table_name += "Forward";
  if (chain_ == ChainNameEnum::OUTPUT)
    table_name += "Output";

  lpm_k key{
      .netmask_len = ip.netmask, .ip = ip.ip,
  };

  std::lock_guard<std::mutex> guard(program_mutex_);
  auto table = iptables_.get_raw_table(table_name, index_, program_type_);
  table.set(&key, value.data());
}

void Iptables::IpLookup::updateMap(
    const std::map<struct IpAddr, std::vector<uint64_t>> &ips) {
  for (auto ele : ips) {
    updateTableValue(ele.first, ele.second);
  }
}
