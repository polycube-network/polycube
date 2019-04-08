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
#include "datapaths/Iptables_TcpFlagsLookup_dp.h"

Iptables::TcpFlagsLookup::TcpFlagsLookup(const int &index,
                                         const ChainNameEnum &chain,
                                         Iptables &outer)
    : Iptables::Program(iptables_code_tcpflagslookup, index, chain, outer,
                        (chain == ChainNameEnum::OUTPUT)
                            ? ProgramType::EGRESS
                            : ProgramType::INGRESS) {
  load();
}

Iptables::TcpFlagsLookup::~TcpFlagsLookup() {}

std::string Iptables::TcpFlagsLookup::getCode() {
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

bool Iptables::TcpFlagsLookup::updateTableValue(
    uint32_t flag_mask, const std::vector<uint64_t> &value) {
  std::string table_name = "tcpFlags";

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
    table.set(&flag_mask, value.data());
  } catch (...) {
    return false;
  }
  return true;
}

void Iptables::TcpFlagsLookup::updateMap(
    const std::vector<std::vector<uint64_t>> &flags) {
  for (uint32_t i = 0; i < flags.size(); ++i) {
    updateTableValue(i, flags[i]);
  }
}

const uint8_t Iptables::TcpFlagsLookup::possible_flags_combinations_[] = {
    0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   10,  11,  12,  13,  14,
    15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,
    30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,
    45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,
    60,  61,  62,  63,  64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,
    75,  76,  77,  78,  79,  80,  81,  82,  83,  84,  85,  86,  87,  88,  89,
    90,  91,  92,  93,  94,  95,  96,  97,  98,  99,  100, 101, 102, 103, 104,
    105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119,
    120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134,
    135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149,
    150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164,
    165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179,
    180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194,
    195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209,
    210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224,
    225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
    240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254,
    255};
