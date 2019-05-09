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
#include "datapaths/Firewall_DefaultAction_dp.h"
#include "polycube/common.h"

Firewall::DefaultAction::DefaultAction(const int &index,
                                       const ChainNameEnum &direction,
                                       Firewall &outer)
    : Firewall::Program(firewall_code_defaultaction, index, direction,
                        outer) {
  load();
}

Firewall::DefaultAction::~DefaultAction() {}

std::string Firewall::DefaultAction::getCode() {
  std::string noMacroCode = code;

  if (firewall.getChain(direction)->getDefault() ==
      ActionEnum::DROP) {
    replaceAll(noMacroCode, "_ACTION", "return RX_DROP;");
  } else {
    replaceAll(noMacroCode, "_ACTION",
               "call_next_program(ctx, " +
                   std::to_string(3 + ModulesConstants::NR_MODULES * 2 + 1) +
                   "); "
                   "return RX_DROP;");
  }

  return noMacroCode;
}

uint64_t Firewall::DefaultAction::getPktsCount(ChainNameEnum chain) {
  std::string tableName = "pktsCounter";

  try {
    uint64_t pkts = 0;
    auto pktsTable = firewall.get_percpuarray_table<uint64_t>(tableName, index,
                                                              getProgramType());
    auto values = pktsTable.get(0);

    return std::accumulate(values.begin(), values.end(), pkts);
  } catch (...) {
    throw std::runtime_error("Counter not available.");
  }
}

uint64_t Firewall::DefaultAction::getBytesCount(ChainNameEnum chain) {
  std::string tableName = "bytesCounter";

  try {
    uint64_t bytes = 0;
    auto bytesTable =
        firewall.get_percpuarray_table<uint64_t>(tableName, index,
                                                 getProgramType());
    auto values = bytesTable.get(0);

    return std::accumulate(values.begin(), values.end(), bytes);
  } catch (...) {
    throw std::runtime_error("Counter not available.");
  }
}

void Firewall::DefaultAction::flushCounters(ChainNameEnum chain) {
  std::string pktsTableName = "pktsCounter";
  std::string bytesTableName = "bytesCounter";

  try {
    auto pktsTable =
        firewall.get_percpuarray_table<uint64_t>(pktsTableName, index,
                                                 getProgramType());
    auto bytesTable =
        firewall.get_percpuarray_table<uint64_t>(bytesTableName, index,
                                                 getProgramType());

    pktsTable.set(0, 0);
    bytesTable.set(0, 0);
  } catch (std::exception &e) {
    throw std::runtime_error("Counters not available: " +
                             std::string(e.what()));
  }
}
