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

#include "../Iptables.h"
#include "datapaths/Iptables_ConntrackTableUpdate_dp.h"

#include <chrono>
#include <thread>
#include "polycube/common.h"

using namespace polycube::service;

Iptables::ConntrackTableUpdate::ConntrackTableUpdate(const int &index,
                                                     Iptables &outer,
                                                     const ProgramType program_type)
    : Iptables::Program(iptables_code_conntracktableupdate, index,
                        (program_type == ProgramType::INGRESS)
                            ? ChainNameEnum::INVALID_INGRESS
                            : ChainNameEnum::INVALID_EGRESS,
                        outer, program_type) {
  load();

  // launch threads only if are in INGRESS ConntrackTableUpdate

  // Launch timestamp thread & update
  quit_thread_ = false;
  // TODO Check if FIB_LOOKUP_OPTIMIZATION is enabled
  if (program_type_ == ProgramType::INGRESS) {
    // UPDATE DEVMAP
    // TODO Devmap is working only if index are 1<index<128
    // Implement a clever way to do that
    try {
      std::lock_guard<std::mutex> guard(program_mutex_);
      auto table = iptables_.get_raw_table("tx_port", index);
      for (uint32_t i = 1; i < 128; i++) {
        try {
          table.set(&i, &i);
        } catch (...) {
          throw;
        }
        // std::cout << "++UPDATE VALUE ++ " << std::to_string(i) << std::endl;
      }
    } catch (std::runtime_error e) {
      // std::cout << "++ ERROR " << e.what() << std::endl;
    }
  }

  if (program_type_ == ProgramType::INGRESS) {
    timestamp_update_thread_ =
        std::thread(&ConntrackTableUpdate::updateTimestampTimer, this);
  }
}

Iptables::ConntrackTableUpdate::~ConntrackTableUpdate() {}

void Iptables::ConntrackTableUpdate::quitAndJoin() {
  if (program_type_ == ProgramType::INGRESS) {
    quit_thread_ = true;
    timestamp_update_thread_.join();
  }
}

std::string Iptables::ConntrackTableUpdate::getCode() {
  std::string no_macro_code = code_;

  /*Replacing the maximum number of rules*/
  replaceAll(no_macro_code, "_MAXRULES",
             std::to_string(FROM_NRULES_TO_NELEMENTS(iptables_.max_rules_)));

  /*Replacing hops*/
  replaceAll(no_macro_code, "_NEXT_HOP_1", std::to_string(index_ + 1));

  replaceAll(no_macro_code, "_CONNTRACK_MODE",
             std::to_string(iptables_.conntrack_mode_));

  if (program_type_ == ProgramType::INGRESS) {
    replaceAll(no_macro_code, "_INGRESS_LOGIC", std::to_string(1));
    replaceAll(no_macro_code, "_EGRESS_LOGIC", std::to_string(0));
  } else if (program_type_ == ProgramType::EGRESS) {
    replaceAll(no_macro_code, "_INGRESS_LOGIC", std::to_string(0));
    replaceAll(no_macro_code, "_EGRESS_LOGIC", std::to_string(1));
  }

  if (program_type_ == ProgramType::INGRESS) {
    replaceAll(no_macro_code, "call_bpf_program", "call_ingress_program");
  } else if (program_type_ == ProgramType::EGRESS) {
    replaceAll(no_macro_code, "call_bpf_program", "call_egress_program");
  }

  replaceAll(no_macro_code, "_FIB_LOOKUP_ENABLED",
             iptables_.fibLookupEnabled() ? "1" : "0");

  return no_macro_code;
}

// Update timestamp every second
void Iptables::ConntrackTableUpdate::updateTimestampTimer() {
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
void Iptables::ConntrackTableUpdate::updateTimestamp() {
  try {
    std::lock_guard<std::mutex> guard(program_mutex_);

    // iptables.logger()->debug("get system timestamp");

    // get timestamp from system
    std::chrono::nanoseconds ns =
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::system_clock::now().time_since_epoch());
    uint64_t nano = ns.count();
    auto timestamp_table = iptables_.get_percpuarray_table<uint64_t>(
        "timestamp", index_, program_type_);
    int index = 0;
    timestamp_table.set(index, nano);
  } catch (...) {
  }
}
