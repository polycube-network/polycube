/*
 * Copyright 2019 The Polycube Authors
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
#include "datapaths/Iptables_ActionCache_dp.h"
//#include "polycube/common.h"

Iptables::ActionCache::ActionCache(const int &index, Iptables &outer,
                                   const ProgramType t)
    : Iptables::Program(iptables_code_actioncache, index,
                        (t == ProgramType::INGRESS)
                            ? ChainNameEnum::INVALID_INGRESS
                            : ChainNameEnum::INVALID_EGRESS,
                        outer, t) {
  load();
}

std::vector<std::pair<tts_k, tts_v>>
Iptables::ActionCache::getTupleToSessionMap() {
  auto table = iptables_.get_hash_table<tts_k, tts_v>("tupletosession", index_,
                                                      program_type_);
  return table.get_all();
}

std::vector<session_v> Iptables::ActionCache::getSessionMap() {
  auto table =
      iptables_.get_array_table<session_v>("session", index_, program_type_);
  std::vector<session_v> ret;
  ret.reserve(SESSION_DIM + 1);

  auto vec = table.get_all();

  for (std::vector<std::pair<uint32_t, session_v>>::iterator it = vec.begin();
       it != vec.end(); ++it) {
    ret[it->first] = it->second;
  }

  return ret;
}

Iptables::ActionCache::~ActionCache() {}

std::string Iptables::ActionCache::getCode() {
  std::string no_macro_code = code_;

  /*Ingress or Egress logic*/
  if (program_type_ == ProgramType::INGRESS) {
    // if this is a parser for INPUT hook:
    replaceAll(no_macro_code, "_INGRESS_LOGIC", std::to_string(1));
    replaceAll(no_macro_code, "_EGRESS_LOGIC", std::to_string(0));
    replaceAll(no_macro_code, "_DIRECTION", "INGRESS");
  } else if (program_type_ == ProgramType::EGRESS) {
    // if this is a parser for OUTPUT hook:
    replaceAll(no_macro_code, "_INGRESS_LOGIC", std::to_string(0));
    replaceAll(no_macro_code, "_EGRESS_LOGIC", std::to_string(1));
    replaceAll(no_macro_code, "_DIRECTION", "EGRESS");
  }

  if (program_type_ == ProgramType::INGRESS) {
    replaceAll(no_macro_code, "call_bpf_program", "call_ingress_program");
  } else if (program_type_ == ProgramType::EGRESS) {
    replaceAll(no_macro_code, "call_bpf_program", "call_egress_program");
  }

  /* Replacing next hops_*/
  replaceAll(no_macro_code, "_CONNTRACKLABEL_INGRESS",
             std::to_string(ModulesConstants::CONNTRACKLABEL_INGRESS));
  replaceAll(no_macro_code, "_CONNTRACKLABEL_EGRESS",
             std::to_string(ModulesConstants::CONNTRACKLABEL_EGRESS));

  /*Ingress or Egress logic*/
  if (program_type_ == ProgramType::INGRESS) {
    replaceAll(no_macro_code, "_HOOK", "INGRESS");
  } else if (program_type_ == ProgramType::EGRESS) {
    replaceAll(no_macro_code, "_HOOK", "EGRESS");
  }

  return no_macro_code;
}
