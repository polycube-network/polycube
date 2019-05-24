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
//#include "../../../pcn-iptables/src/defines.h"

Firewall::Parser::Parser(const int &index, const ChainNameEnum &direction,
                         Firewall &outer)
    : Firewall::Program(firewall_code_parser, index, direction, outer) {

  reload();
}

Firewall::Parser::~Parser() {}

std::string Firewall::Parser::getCode() {
  std::string noMacroCode = code;

  /*Replacing the maximum number of rules*/
  replaceAll(noMacroCode, "_MAXRULES", std::to_string(FROM_NRULES_TO_NELEMENTS(firewall.maxRules)));

  replaceAll(noMacroCode, "_DEFAULTACTION", this->defaultActionString());

  replaceAll(noMacroCode, "_CONNTRACKLABEL", std::to_string(ModulesConstants::CONNTRACKLABEL));
  replaceAll(noMacroCode, "_CHAINFORWARDER", std::to_string(ModulesConstants::CHAINFORWARDER));

  if (firewall.getConntrack() == FirewallConntrackEnum::ON) {
    replaceAll(noMacroCode, "_CONNTRACK_ENABLED", std::to_string(1));
  } else {
    replaceAll(noMacroCode, "_CONNTRACK_ENABLED", std::to_string(0));
  }

  try {
    replaceAll(noMacroCode, "_NR_ELEMENTS",
             std::to_string(FROM_NRULES_TO_NELEMENTS(
                 firewall.getChain(direction)->getNrRules())));
  } catch (...) {
    // chain not active.
    replaceAll(noMacroCode, "_NR_ELEMENTS",
               std::to_string(FROM_NRULES_TO_NELEMENTS(0)));
  }

  return noMacroCode;
}
