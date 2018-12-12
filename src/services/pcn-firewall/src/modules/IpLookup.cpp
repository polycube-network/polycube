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
#include "datapaths/Firewall_IpLookup_dp.h"

using namespace polycube::service;

/* definitions copied from datapath */
struct lpm_k {
  uint32_t netmask_len;
  uint32_t ip;
} __attribute__((packed));

Firewall::IpLookup::IpLookup(const int &index, const ChainNameEnum &direction,
                             const int &type, Firewall &outer)
    : Firewall::Program(firewall_code_iplookup, index, direction, outer) {
  this->type = type;
  load();
}

Firewall::IpLookup::~IpLookup() {}

std::string Firewall::IpLookup::getCode() {
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

  /*Replacing type*/
  if (type == SOURCE_TYPE)
    replaceAll(noMacroCode, "_TYPE", "src");
  else
    replaceAll(noMacroCode, "_TYPE", "dst");

  /*Replacing the default action*/
  replaceAll(noMacroCode, "_DEFAULTACTION", defaultActionString());
  return noMacroCode;
}

void Firewall::IpLookup::updateTableValue(uint8_t netmask, std::string ip,
                                          const std::vector<uint64_t> &value) {
  std::string tableName = "ip";

  if (type == SOURCE_TYPE) {
    tableName += "src";
  } else if (type == DESTINATION_TYPE) {
    tableName += "dst";
  }
  tableName += "Trie";

  if (direction == ChainNameEnum::INGRESS)
    tableName += "Ingress";
  else if (direction == ChainNameEnum::EGRESS)
    tableName += "Egress";

  lpm_k key {
    .netmask_len = netmask,
    .ip = utils::ip_string_to_be_uint(ip),
  };

  auto table = firewall.get_raw_table(tableName, index);
  table.set(&key, value.data());
}

void Firewall::IpLookup::updateTableValue(IpAddr ip,
                                          const std::vector<uint64_t> &value) {
  std::string tableName = "ip";

  if (type == SOURCE_TYPE) {
    tableName += "src";
  } else if (type == DESTINATION_TYPE) {
    tableName += "dst";
  }
  tableName += "Trie";

  if (direction == ChainNameEnum::INGRESS)
    tableName += "Ingress";
  else if (direction == ChainNameEnum::EGRESS)
    tableName += "Egress";

  lpm_k key {
    .netmask_len = ip.netmask,
    .ip = ip.ip, // TODO: is byteordering ok?
  };

  auto table = firewall.get_raw_table(tableName, index);
  table.set(&key, value.data());
}

void Firewall::IpLookup::updateMap(
    const std::map<struct IpAddr, std::vector<uint64_t>> &ips) {
  for (auto ele : ips) {
    updateTableValue(ele.first, ele.second);
  }
}
