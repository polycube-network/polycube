/*
 * Copyright 2019 The Polycube Authors
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
#include "datapaths/Iptables_ActionCache_dp.h"
//#include "polycube/common.h"

#define GARBAGE_COLLECTOR_INTERVAL 10

Iptables::ActionCache::ActionCache(const int &index, Iptables &outer,
                                   const ProgramType t)
    : Iptables::Program(iptables_code_actioncache, index,
                        (t == ProgramType::INGRESS)
                            ? ChainNameEnum::INVALID_INGRESS
                            : ChainNameEnum::INVALID_EGRESS,
                        outer, t) {
  load();

  quit_thread_ = false;

  if (program_type_ == ProgramType::INGRESS) {
    garbage_collecotor_thread_ =
        std::thread(&ActionCache::garbageCollectorTimer, this);
  }
}

Iptables::ActionCache::~ActionCache() {
  quitAndJoin();
}

// Update timestamp every second
void Iptables::ActionCache::garbageCollectorTimer() {
  for (;;) {
    sleep(GARBAGE_COLLECTOR_INTERVAL);
    if (quit_thread_)
      break;
    garbageCollector();
  }
}

void Iptables::ActionCache::quitAndJoin() {
  if (program_type_ == ProgramType::INGRESS) {
    quit_thread_ = true;
    garbage_collecotor_thread_.join();
  }
}

// This method is invoked every 10 seconds
// and performs a cleanup of expired entries
void Iptables::ActionCache::garbageCollector() {
  try {
    std::lock_guard<std::mutex> guard(program_mutex_);

    iptables_.logger()->trace(
        "[ControlPlane] [garbageCollector] cleanup session table expired "
        "entries ");

    // get timestamp from system
    std::chrono::nanoseconds ns =
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::system_clock::now().time_since_epoch());
    uint64_t current_time_nano = ns.count();

    // iterate over sessions array
    auto session_table =
        iptables_.get_array_table<session_v>("session", index_, program_type_);

    auto session_table_offline = session_table.get_all();

    struct session_v clean_session;
    clean_session.setMask = 0;
    clean_session.actionMask = 0;

    for (std::vector<std::pair<uint32_t, session_v>>::iterator it =
             session_table_offline.begin();
         it != session_table_offline.end(); ++it) {
      if (it->second.setMask != 0) {
        if (it->second.ttl < current_time_nano) {
          // invalidate expired entries
          iptables_.logger()->trace(
              "[ControlPlane] [garbageCollector] clean sessionId: {0} ",
              it->first);
          session_table.set(it->first, clean_session);
        }
      }
    }

    // get an offline copy of sessions
    session_table_offline = session_table.get_all();

    // create a vector to access session in O(1)
    std::vector<session_v> session_table_vector;
    session_table_vector.reserve(SESSION_DIM + 1);

    for (std::vector<std::pair<uint32_t, session_v>>::iterator it =
             session_table_offline.begin();
         it != session_table_offline.end(); ++it) {
      session_table_vector[it->first] = it->second;
    }

    // iterate over tupletosession table
    auto tupletosession_table = iptables_.get_hash_table<tts_k, tts_v>(
        "tupletosession", index_, program_type_);

    auto tupletosession_table_offline = tupletosession_table.get_all();

    for (std::vector<std::pair<tts_k, tts_v>>::iterator it =
             tupletosession_table_offline.begin();
         it != tupletosession_table_offline.end(); ++it) {
      if (session_table_vector[it->second.sessionId].setMask == 0) {
        // if pointing to an expired session, the setMask should be already set
        // to 0
        // then remove the key pointing to expired entry
        iptables_.logger()->trace(
            "[ControlPlane] [garbageCollector] clean srcIp {0} dstIp {1} "
            "l4proto {2} sport {3} dport {4} ",
            utils::be_uint_to_ip_string(it->first.srcIp),
            utils::be_uint_to_ip_string(it->first.dstIp), it->first.l4proto,
            it->first.srcPort, it->first.dstPort);
        tupletosession_table.remove(it->first);
      }
    }
  } catch (...) {
  }
}

std::vector<std::pair<tts_k, tts_v>>
Iptables::ActionCache::getTupleToSessionMap() {
  auto table = iptables_.get_hash_table<tts_k, tts_v>("tupletosession", index_,
                                                      program_type_);
  return table.get_all();
}

std::vector<session_v> Iptables::ActionCache::getSessionMap() {
  auto table =
      iptables_.get_array_table<session_v>("session", index_, program_type_);
  std::vector<session_v> ret;
  ret.reserve(SESSION_DIM + 1);

  auto vec = table.get_all();

  for (std::vector<std::pair<uint32_t, session_v>>::iterator it = vec.begin();
       it != vec.end(); ++it) {
    ret[it->first] = it->second;
  }

  return ret;
}

std::string Iptables::ActionCache::getCode() {
  std::string no_macro_code = code_;

  /*Ingress or Egress logic*/
  if (program_type_ == ProgramType::INGRESS) {
    // if this is a parser for INPUT hook:
    replaceAll(no_macro_code, "_INGRESS_LOGIC", std::to_string(1));
    replaceAll(no_macro_code, "_EGRESS_LOGIC", std::to_string(0));
    replaceAll(no_macro_code, "_DIRECTION", "INGRESS");
  } else if (program_type_ == ProgramType::EGRESS) {
    // if this is a parser for OUTPUT hook:
    replaceAll(no_macro_code, "_INGRESS_LOGIC", std::to_string(0));
    replaceAll(no_macro_code, "_EGRESS_LOGIC", std::to_string(1));
    replaceAll(no_macro_code, "_DIRECTION", "EGRESS");
  }

  if (program_type_ == ProgramType::INGRESS) {
    replaceAll(no_macro_code, "call_bpf_program", "call_ingress_program");
  } else if (program_type_ == ProgramType::EGRESS) {
    replaceAll(no_macro_code, "call_bpf_program", "call_egress_program");
  }

  /* Replacing next hops_*/
  replaceAll(no_macro_code, "_CONNTRACKLABEL_INGRESS",
             std::to_string(ModulesConstants::CONNTRACKLABEL_INGRESS));
  replaceAll(no_macro_code, "_CONNTRACKLABEL_EGRESS",
             std::to_string(ModulesConstants::CONNTRACKLABEL_EGRESS));

  /*Ingress or Egress logic*/
  if (program_type_ == ProgramType::INGRESS) {
    replaceAll(no_macro_code, "_HOOK", "INGRESS");
  } else if (program_type_ == ProgramType::EGRESS) {
    replaceAll(no_macro_code, "_HOOK", "EGRESS");
  }

  return no_macro_code;
}
