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

#include "../../../../polycubed/src/extiface_info.h"
#include "../Iptables.h"
#include "datapaths/Iptables_ChainForwarder_dp.h"

Iptables::ChainForwarder::ChainForwarder(const int &index, Iptables &outer,
                                         const ProgramType t)
    : Iptables::Program(iptables_code_chainforwarder, index,
                        (t == ProgramType::INGRESS)
                            ? ChainNameEnum::INVALID_INGRESS
                            : ChainNameEnum::INVALID_EGRESS,
                        outer, t) {
  load();
}

Iptables::ChainForwarder::~ChainForwarder() {}

std::string Iptables::ChainForwarder::defaultActionString(ChainNameEnum chain) {
  return "";
}

std::string Iptables::ChainForwarder::getCode() {
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

  /*Replacing hops*/
  if (hops_.find("_NEXT_HOP_INPUT_1") == hops_.end()) {
    /*Bootloading the ingress chain. Let the module loop on itself for now. */
    replaceAll(no_macro_code, "_NEXT_HOP_INPUT_1", std::to_string(index_));
  } else {
    replaceAll(no_macro_code, "_NEXT_HOP_INPUT_1",
               std::to_string(hops_["_NEXT_HOP_INPUT_1"]->getIndex()));
  }

  if (hops_.find("_NEXT_HOP_FORWARD_1") == hops_.end()) {
    /*Bootloading the egress chain. Let the module loop on itself for now. */
    replaceAll(no_macro_code, "_NEXT_HOP_FORWARD_1", std::to_string(index_));
  } else {
    replaceAll(no_macro_code, "_NEXT_HOP_FORWARD_1",
               std::to_string(hops_["_NEXT_HOP_FORWARD_1"]->getIndex()));
  }

  if (hops_.find("_NEXT_HOP_OUTPUT_1") == hops_.end()) {
    /*Bootloading the egress chain. Let the module loop on itself for now. */
    replaceAll(no_macro_code, "_NEXT_HOP_OUTPUT_1", std::to_string(index_));
  } else {
    replaceAll(no_macro_code, "_NEXT_HOP_OUTPUT_1",
               std::to_string(hops_["_NEXT_HOP_OUTPUT_1"]->getIndex()));
  }

  if (program_type_ == ProgramType::INGRESS) {
    replaceAll(no_macro_code, "call_bpf_program", "call_ingress_program");
  } else if (program_type_ == ProgramType::EGRESS) {
    replaceAll(no_macro_code, "call_bpf_program", "call_egress_program");
  }

  return no_macro_code;
}
