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
#include "datapaths/Firewall_BitScan_dp.h"

Firewall::BitScan::BitScan(const int &index, const ChainNameEnum &direction,
                           Firewall &outer)
    : Firewall::Program(firewall_code_bitscan, index, direction, outer) {
  load();
  loadIndex64();
}

Firewall::BitScan::~BitScan() {}

void Firewall::BitScan::loadIndex64() {
  const uint16_t index64[64] = {0,  47, 1,  56, 48, 27, 2,  60, 57, 49, 41, 37, 28,
                           16, 3,  61, 54, 58, 35, 52, 50, 42, 21, 44, 38, 32,
                           29, 23, 17, 11, 4,  62, 46, 55, 26, 59, 40, 36, 15,
                           53, 34, 51, 20, 43, 31, 22, 10, 45, 25, 39, 14, 33,
                           19, 30, 9,  24, 13, 18, 8,  12, 7,  6,  5,  63};

  auto table = firewall.get_array_table<uint16_t>("index64", index);

  for (uint32_t i = 0; i < 64; ++i) {
    table.set(i, index64[i]);
  }
}

std::string Firewall::BitScan::getCode() {
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

  /*Replacing tracing flag*/
  if (firewall.trace)
    replaceAll(noMacroCode, "_TRACE", std::to_string(1));
  else
    replaceAll(noMacroCode, "_TRACE", std::to_string(0));

  /*Replacing direction suffix*/
  if (direction == ChainNameEnum::INGRESS)
    replaceAll(noMacroCode, "_DIRECTION", "Ingress");
  else
    replaceAll(noMacroCode, "_DIRECTION", "Egress");

  /*Replacing the default action*/
  replaceAll(noMacroCode, "_DEFAULTACTION", defaultActionString());

  return noMacroCode;
}
