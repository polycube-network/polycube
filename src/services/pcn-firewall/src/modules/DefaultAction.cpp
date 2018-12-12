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

#include "polycube/common.h"
#include "../Firewall.h"
#include "datapaths/Firewall_DefaultAction_dp.h"

Firewall::DefaultAction::DefaultAction(const int &index, Firewall &outer)
    : Firewall::Program(firewall_code_defaultaction, index,
                        ChainNameEnum::INVALID, outer) {
  load();
}

Firewall::DefaultAction::~DefaultAction() {}

std::string Firewall::DefaultAction::getCode() {
  std::string noMacroCode = code;

  if (firewall.getChain(ChainNameEnum::INGRESS)->getDefault() ==
      ActionEnum::DROP) {
    replaceAll(noMacroCode, "_ACTIONINGRESS", "return RX_DROP;");
  } else {
    replaceAll(noMacroCode, "_ACTIONINGRESS",
               "call_ingress_program(ctx, " +
                   std::to_string(3 + ModulesConstants::NR_MODULES * 4 + 1) +
                   "); "
                   "return RX_DROP;");
  }

  if (firewall.getChain(ChainNameEnum::EGRESS)->getDefault() ==
      ActionEnum::DROP) {
    replaceAll(noMacroCode, "_ACTIONEGRESS", "return RX_DROP;");
  } else {
    replaceAll(noMacroCode, "_ACTIONEGRESS",
               "call_ingress_program(ctx, " +
                   std::to_string(3 + ModulesConstants::NR_MODULES * 4 + 1) +
                   "); "
                   "return RX_DROP;");
  }

  /*Replacing ports*/
  replaceAll(noMacroCode, "_INGRESSPORT",
             std::to_string(firewall.getIngressPortIndex()));
  replaceAll(noMacroCode, "_EGRESSPORT",
             std::to_string(firewall.getEgressPortIndex()));
  return noMacroCode;

  return noMacroCode;
}

uint64_t Firewall::DefaultAction::getPktsCount(ChainNameEnum chain) {
  std::string tableName = "pkts";

  if (chain == ChainNameEnum::INGRESS)
    tableName += "Ingress";
  else if (chain == ChainNameEnum::EGRESS)
    tableName += "Egress";

  try {
    uint64_t pkts = 0;
    auto pktsTable = firewall.get_percpuarray_table<uint64_t>(tableName, index);
    auto values = pktsTable.get(0);

    return std::accumulate(values.begin(), values.end(), pkts);
  } catch (...) {
    throw std::runtime_error("Counter not available.");
  }
}

uint64_t Firewall::DefaultAction::getBytesCount(ChainNameEnum chain) {
  std::string tableName = "bytes";

  if (chain == ChainNameEnum::INGRESS)
    tableName += "Ingress";
  else if (chain == ChainNameEnum::EGRESS)
    tableName += "Egress";

  try {
    uint64_t bytes = 0;
    auto bytesTable = firewall.get_percpuarray_table<uint64_t>(tableName, index);
    auto values = bytesTable.get(0);

    return std::accumulate(values.begin(), values.end(), bytes);
  } catch (...) {
    throw std::runtime_error("Counter not available.");
  }
}

void Firewall::DefaultAction::flushCounters(ChainNameEnum chain) {
  std::string pktsTableName = "pkts";
  std::string bytesTableName = "bytes";

  if (chain == ChainNameEnum::INGRESS) {
    pktsTableName += "Ingress";
    bytesTableName += "Ingress";
  } else if (chain == ChainNameEnum::EGRESS) {
    pktsTableName += "Egress";
    bytesTableName += "Egress";
  }

  try {
    auto pktsTable = firewall.get_percpuarray_table<uint64_t>(pktsTableName, index);
    auto bytesTable = firewall.get_percpuarray_table<uint64_t>(bytesTableName, index);

    pktsTable.set(0, 0);
    bytesTable.set(0, 0);
  } catch (std::exception &e) {
    throw std::runtime_error("Counters not available: " +
                             std::string(e.what()));
  }
}
