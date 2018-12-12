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
#include "datapaths/Firewall_Parser_dp.h"

Firewall::Parser::Parser(const int &index, Firewall &outer)
    : Firewall::Program(firewall_code_parser, index, ChainNameEnum::INVALID,
                        outer) {
  reload();
}

Firewall::Parser::~Parser() {}

std::string Firewall::Parser::getCode() {
  std::string noMacroCode = code;

  /*Replacing the maximum number of rules*/
  replaceAll(noMacroCode, "_MAXRULES", std::to_string(firewall.maxRules / 64));

  if (firewall.conntrackMode == ConntrackModes::DISABLED) {
    // Skip the next module (conntrack), that is not loaded
    replaceAll(noMacroCode, "_NEXT_HOP", std::to_string(index + 2));
  } else {
    replaceAll(noMacroCode, "_NEXT_HOP", std::to_string(index + 1));
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
