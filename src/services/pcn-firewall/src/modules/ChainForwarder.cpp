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

#include "../Firewall.h"
#include "datapaths/Firewall_ChainForwarder_dp.h"

Firewall::ChainForwarder::ChainForwarder(const int &index, Firewall &outer)
    : Firewall::Program(firewall_code_chainforwarder, index,
                        ChainNameEnum::INVALID, outer) {
  load();
}

Firewall::ChainForwarder::~ChainForwarder() {}

std::string Firewall::ChainForwarder::getCode() {
  std::string noMacroCode = code;

  /*Replacing the maximum number of rules*/
  replaceAll(noMacroCode, "_MAXRULES", std::to_string(firewall.maxRules / 64));
  /*Replacing hops*/
  if (hops.find("_NEXT_HOP_INGRESS_1") == hops.end()) {
    /*Bootloading the ingress chain. Let the module loop on itself for now. */
    replaceAll(noMacroCode, "_NEXT_HOP_INGRESS_1", std::to_string(index));
  } else {
    replaceAll(noMacroCode, "_NEXT_HOP_INGRESS_1",
               std::to_string(hops["_NEXT_HOP_INGRESS_1"]->getIndex()));
  }
  if (hops.find("_NEXT_HOP_EGRESS_1") == hops.end()) {
    /*Bootloading the egress chain. Let the module loop on itself for now. */
    replaceAll(noMacroCode, "_NEXT_HOP_EGRESS_1", std::to_string(index));
  } else {
    replaceAll(noMacroCode, "_NEXT_HOP_EGRESS_1",
               std::to_string(hops["_NEXT_HOP_EGRESS_1"]->getIndex()));
  }

  /*Replacing the default action*/
  replaceAll(noMacroCode, "_DEFAULTACTION_INGRESS",
             this->defaultActionString());
  replaceAll(noMacroCode, "_DEFAULTACTION_EGRESS", this->defaultActionString());

  /*Replacing nrElements*/
  try {
    replaceAll(noMacroCode, "_NR_ELEMENTS_INGRESS",
               std::to_string(FROM_NRULES_TO_NELEMENTS(
                   firewall.getChain(ChainNameEnum::INGRESS)->getNrRules())));
  } catch (...) {
    // Ingress chain not active.
    replaceAll(noMacroCode, "_NR_ELEMENTS_INGRESS",
               std::to_string(FROM_NRULES_TO_NELEMENTS(0)));
  }

  try {
    replaceAll(noMacroCode, "_NR_ELEMENTS_EGRESS",
               std::to_string(FROM_NRULES_TO_NELEMENTS(
                   firewall.getChain(ChainNameEnum::EGRESS)->getNrRules())));
  } catch (...) {
    // Egress chain not active.
    replaceAll(noMacroCode, "_NR_ELEMENTS_EGRESS",
               std::to_string(FROM_NRULES_TO_NELEMENTS(0)));
  }

  /*Replacing ports*/
  replaceAll(noMacroCode, "_INGRESSPORT",
             std::to_string(firewall.getIngressPortIndex()));
  replaceAll(noMacroCode, "_EGRESSPORT",
             std::to_string(firewall.getEgressPortIndex()));
  return noMacroCode;
}
