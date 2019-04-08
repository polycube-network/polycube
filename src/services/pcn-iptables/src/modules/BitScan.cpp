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
#include "datapaths/Iptables_BitScan_dp.h"

Iptables::BitScan::BitScan(const int &index, const ChainNameEnum &chain,
                           Iptables &outer)
    : Iptables::Program(iptables_code_bitscan, index, chain, outer,
                        (chain == ChainNameEnum::OUTPUT)
                            ? ProgramType::EGRESS
                            : ProgramType::INGRESS) {
  load();
  loadIndex64();
}

Iptables::BitScan::~BitScan() {}

void Iptables::BitScan::loadIndex64() {
  std::lock_guard<std::mutex> guard(program_mutex_);
  const uint16_t index64[64] = {
      0,  47, 1,  56, 48, 27, 2,  60, 57, 49, 41, 37, 28, 16, 3,  61,
      54, 58, 35, 52, 50, 42, 21, 44, 38, 32, 29, 23, 17, 11, 4,  62,
      46, 55, 26, 59, 40, 36, 15, 53, 34, 51, 20, 43, 31, 22, 10, 45,
      25, 39, 14, 33, 19, 30, 9,  24, 13, 18, 8,  12, 7,  6,  5,  63};

  auto table =
      iptables_.get_array_table<uint16_t>("index64", index_, program_type_);

  for (int i = 0; i < 64; ++i) {
    table.set(i, index64[i]);
  }
}

std::string Iptables::BitScan::getCode() {
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

  /*Ingress or Egress logic*/
  if (program_type_ == ProgramType::INGRESS) {
    replaceAll(no_macro_code, "_HOOK", "INGRESS");
  } else if (program_type_ == ProgramType::EGRESS) {
    replaceAll(no_macro_code, "_HOOK", "EGRESS");
  }

  return no_macro_code;
}
