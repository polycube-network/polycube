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
#include "datapaths/Firewall_L4ProtocolLookup_dp.h"

Firewall::L4ProtocolLookup::L4ProtocolLookup(const int &index,
                                             const ChainNameEnum &direction,
                                             Firewall &outer)
    : Firewall::Program(firewall_code_l4protolookup, index, direction, outer) {
  load();
}

Firewall::L4ProtocolLookup::~L4ProtocolLookup() {}

std::string Firewall::L4ProtocolLookup::getCode() {
  std::string noMacroCode = code;

  /*Replacing the maximum number of rules*/
  replaceAll(noMacroCode, "_MAXRULES", std::to_string(firewall.maxRules / 64));

  /*Replacing hops*/
  for (auto const &hop : hops) {
    replaceAll(noMacroCode, hop.first,
               std::to_string((hop.second)->getIndex()));
  }
  replaceAll(noMacroCode, "_NEXT_HOP_1", std::to_string(index + 1));

  /*Replacing nrElements*/
  replaceAll(noMacroCode, "_NR_ELEMENTS",
             std::to_string(FROM_NRULES_TO_NELEMENTS(
                 firewall.getChain(direction)->getNrRules())));

  /*Replacing the default action*/
  replaceAll(noMacroCode, "_DEFAULTACTION", defaultActionString());

  return noMacroCode;
}

bool Firewall::L4ProtocolLookup::updateTableValue(
    uint8_t proto, const std::vector<uint64_t> &value) {
  std::string tableName = "transportProto";

  try {
    auto table = firewall.get_raw_table(tableName, index, getProgramType());
    table.set(&proto, value.data());
  } catch (...) {
    return false;
  }
  return true;
}

void Firewall::L4ProtocolLookup::updateMap(
    std::map<int, std::vector<uint64_t>> &protocols) {
  for (auto ele : protocols) {
    updateTableValue(ele.first, ele.second);
  }
}
