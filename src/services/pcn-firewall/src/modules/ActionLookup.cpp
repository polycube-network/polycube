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
#include "datapaths/Firewall_ActionLookup_dp.h"

Firewall::ActionLookup::ActionLookup(const int &index,
                                     const ChainNameEnum &direction,
                                     Firewall &outer)
    : Firewall::Program(firewall_code_actionlookup, index, direction, outer) {
  load();
}

Firewall::ActionLookup::~ActionLookup() {}

std::string Firewall::ActionLookup::getCode() {
  std::string noMacroCode = code;

  /*Replacing the maximum number of rules*/
  replaceAll(noMacroCode, "_MAXRULES", std::to_string(firewall.maxRules / 64));

  /*Replacing nrElements*/
  replaceAll(noMacroCode, "_NR_ELEMENTS",
             std::to_string(FROM_NRULES_TO_NELEMENTS(
                 firewall.getChain(direction)->getNrRules())));

  /*Pointing to the module in charge of updating the conn table and forwarding*/
  replaceAll(noMacroCode, "_CONNTRACKTABLEUPDATE",
             std::to_string(3 + ModulesConstants::NR_MODULES * 4 + 1));

  /*Replacing direction suffix*/
  if (direction == ChainNameEnum::INGRESS)
    replaceAll(noMacroCode, "_DIRECTION", "Ingress");
  else
    replaceAll(noMacroCode, "_DIRECTION", "Egress");

  /*Replacing ports*/
  replaceAll(noMacroCode, "_INGRESSPORT",
             std::to_string(firewall.getIngressPortIndex()));
  replaceAll(noMacroCode, "_EGRESSPORT",
             std::to_string(firewall.getEgressPortIndex()));

  return noMacroCode;
}

uint64_t Firewall::ActionLookup::getPktsCount(int ruleNumber) {
  std::string tableName = "pkts";

  if (direction == ChainNameEnum::INGRESS)
    tableName += "Ingress";
  else if (direction == ChainNameEnum::EGRESS)
    tableName += "Egress";

  try {
    uint64_t pkts = 0;
    auto pktsTable = firewall.get_percpuarray_table<uint64_t>(tableName, index);
    auto values = pktsTable.get(ruleNumber);

    return std::accumulate(values.begin(), values.end(), pkts);
  } catch (...) {
    throw std::runtime_error("Counter not available.");
  }
}

uint64_t Firewall::ActionLookup::getBytesCount(int ruleNumber) {
  std::string tableName = "bytes";

  if (direction == ChainNameEnum::INGRESS)
    tableName += "Ingress";
  else if (direction == ChainNameEnum::EGRESS)
    tableName += "Egress";

  try {
    uint64_t bytes = 0;
    auto bytesTable = firewall.get_percpuarray_table<uint64_t>(tableName, index);
    auto values = bytesTable.get(ruleNumber);

    return std::accumulate(values.begin(), values.end(), bytes);
  } catch (...) {
    throw std::runtime_error("Counter not available.");
  }
}

void Firewall::ActionLookup::flushCounters(int ruleNumber) {
  std::string pktsTableName = "pkts";
  std::string bytesTableName = "bytes";

  if (direction == ChainNameEnum::INGRESS) {
    pktsTableName += "Ingress";
    bytesTableName += "Ingress";
  } else if (direction == ChainNameEnum::EGRESS) {
    pktsTableName += "Egress";
    bytesTableName += "Egress";
  }

  try {
    auto pktsTable = firewall.get_percpuarray_table<uint64_t>(pktsTableName, index);
    auto bytesTable = firewall.get_percpuarray_table<uint64_t>(bytesTableName, index);

    pktsTable.set(ruleNumber, 0);
    bytesTable.set(ruleNumber, 0);
  } catch (std::exception &e) {
    throw std::runtime_error("Counters not available: " +
                             std::string(e.what()));
  }
}

bool Firewall::ActionLookup::updateTableValue(int ruleNumber, int action) {
  std::string tableName = "actions";

  if (direction == ChainNameEnum::INGRESS)
    tableName += "Ingress";
  else if (direction == ChainNameEnum::EGRESS)
    tableName += "Egress";
  else
    return false;
  try {
    auto actionsTable = firewall.get_array_table<int>(tableName, index);
    actionsTable.set(ruleNumber, action);
  } catch (...) {
    return false;
  }
  return true;
}
