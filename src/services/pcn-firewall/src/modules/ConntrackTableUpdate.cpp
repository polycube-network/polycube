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
#include "datapaths/Firewall_ConntrackTableUpdate_dp.h"

#include <chrono>
#include <thread>
#include "polycube/common.h"

Firewall::ConntrackTableUpdate::ConntrackTableUpdate(const int &index,
    const ChainNameEnum &direction, Firewall &outer)
    : Firewall::Program(firewall_code_conntracktableupdate, index,
                        direction, outer) {

  load();


  // launch threads only if are in INGRESS ConntrackTableUpdate

  // Launch timestamp thread & update
  quit_thread_ = false;

  if (getProgramType() == ProgramType::INGRESS) {
    timestamp_update_thread_ =
            std::thread(&ConntrackTableUpdate::updateTimestampTimer, this);
  }
}

Firewall::ConntrackTableUpdate::~ConntrackTableUpdate() {
  quitAndJoin();
}

std::string Firewall::ConntrackTableUpdate::getCode() {
  std::string noMacroCode = code;

  /*Replacing the maximum number of rules*/
  replaceAll(noMacroCode, "_MAXRULES", std::to_string(FROM_NRULES_TO_NELEMENTS(firewall.maxRules)));

  /*Replacing hops*/
  replaceAll(noMacroCode, "_NEXT_HOP_1", std::to_string(index + 1));

  replaceAll(noMacroCode, "_CONNTRACK_MODE",
             std::to_string(firewall.conntrackMode));

  return noMacroCode;
}

void Firewall::ConntrackTableUpdate::quitAndJoin() {
  if (getProgramType() == ProgramType::INGRESS) {
    quit_thread_ = true;
    timestamp_update_thread_.join();
  }
}

// Update timestamp every second
void Firewall::ConntrackTableUpdate::updateTimestampTimer() {
  for (;;) {
    sleep(1);
    if (quit_thread_)
      break;
    updateTimestamp();
  }
}

// this method is in charge to update timestamp in
//'timestamp' percpu array in dataplane.
// this method should be called by a separatate thread
// once per second.
void Firewall::ConntrackTableUpdate::updateTimestamp() {
  try {
    // TODO CHeck it works fine
    // iptables.logger()->debug("get system timestamp");

    // get timestamp from system
    std::chrono::nanoseconds ns =
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::system_clock::now().time_since_epoch());
    uint64_t nano = ns.count();
    auto timestamp_table = firewall.get_percpuarray_table<uint64_t>(
            "timestamp", index, getProgramType());
    int index = 0;
    timestamp_table.set(index, nano);
  } catch (...) {
  }
}
