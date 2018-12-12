/*
 * Copyright 2018 The Polycube Authors
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
#include "datapaths/Iptables_Ddos_dp.h"

struct ddosKey {
  bool src_ip_set = false;
  bool dst_ip_set = false;
  bool src_port_set = false;
  bool dst_port_set = false;
  bool l4proto_set = false;

  uint32_t src_ip;
  uint32_t dst_ip;
  uint16_t src_port;
  uint16_t dst_port;
  uint8_t l4proto;

  void *toData(char *buffer) {
    int size = 0;
    if (src_ip_set) {
      memcpy(buffer + size, &src_ip, sizeof(uint32_t));
      size += sizeof(uint32_t);
    }
    if (dst_ip_set) {
      memcpy(buffer + size, &dst_ip, sizeof(uint32_t));
      size += sizeof(uint32_t);
    }
    if (src_port_set) {
      uint16_t be = htons(src_port);
      memcpy(buffer + size, &be, sizeof(uint16_t));
      size += sizeof(uint16_t);
    }
    if (dst_port_set) {
      uint16_t be = htons(dst_port);
      memcpy(buffer + size, &be, sizeof(uint16_t));
      size += sizeof(uint16_t);
    }
    if (l4proto_set) {
      memcpy(buffer + size, &l4proto, sizeof(uint8_t));
      size += sizeof(uint8_t);
    }
    return buffer;
  }
};

Iptables::Ddos::Ddos(const int &index, Iptables &outer,
                     const std::map<struct DdosRule, struct DdosValue> &ddos,
                     DdosType type)
    : Iptables::Program(iptables_code_ddos, index,
                        ChainNameEnum::INVALID_INGRESS, outer) {
  type_ = type;
  ddos_ = ddos;
  load();
}

Iptables::Ddos::~Ddos() {}

std::string Iptables::Ddos::defaultActionString(ChainNameEnum chain) {
  return "";
}

std::string Iptables::Ddos::getCode() {
  std::string no_macro_code = code_;

  struct DdosRule ddos_key;
  for (auto ele : ddos_) {
    ddos_key = ele.first;
    break;
  }

  std::string enabled;
  std::string disabled;

  if (CHECK_BIT(ddos_key.setFields, DdosConst::SRCIP)) {
    replaceAll(no_macro_code, "_SRCIP", std::to_string(1));
    enabled += "SRCIP ";
  } else {
    replaceAll(no_macro_code, "_SRCIP", std::to_string(0));
    disabled += "SRCIP ";
  }

  if (CHECK_BIT(ddos_key.setFields, DdosConst::DSTIP)) {
    replaceAll(no_macro_code, "_DSTIP", std::to_string(1));
    enabled += "DSTIP ";
  } else {
    replaceAll(no_macro_code, "_DSTIP", std::to_string(0));
    disabled += "DSTIP ";
  }

  if (CHECK_BIT(ddos_key.setFields, DdosConst::SRCPORT)) {
    replaceAll(no_macro_code, "_SRCPORT", std::to_string(1));
    enabled += "SRCPORT ";
  } else {
    replaceAll(no_macro_code, "_SRCPORT", std::to_string(0));
    disabled += "SRCPORT ";
  }

  if (CHECK_BIT(ddos_key.setFields, DdosConst::DSTPORT)) {
    enabled += "DSTPORT ";
    replaceAll(no_macro_code, "_DSTPORT", std::to_string(1));
  } else {
    replaceAll(no_macro_code, "_DSTPORT", std::to_string(0));
    disabled += "DSTPORT ";
  }

  if (CHECK_BIT(ddos_key.setFields, DdosConst::L4PROTO)) {
    replaceAll(no_macro_code, "_L4PROTO", std::to_string(1));
    enabled += "L4PROTO ";
  } else {
    replaceAll(no_macro_code, "_L4PROTO", std::to_string(0));
    disabled += "L4PROTO ";
  }

  replaceAll(no_macro_code, "_CHAINSELECTOR",
             std::to_string(ModulesConstants::CHAINSELECTOR_INGRESS));
  replaceAll(no_macro_code, "_CONNTRACK_LABEL_INGRESS",
             std::to_string(ModulesConstants::CONNTRACKLABEL_INGRESS));

  if (program_type_ == ProgramType::INGRESS){
    replaceAll(no_macro_code, "call_bpf_program", "call_ingress_program");
  } else if (program_type_ == ProgramType::EGRESS){
    replaceAll(no_macro_code, "call_bpf_program", "call_egress_program");
  }

  iptables_.logger()->debug("DDoS fields ENABLED: {0} - DISABLED {1} ", enabled,
                           disabled);

  return no_macro_code;
}

uint64_t Iptables::Ddos::getPktsCount(int rule_number) {
  std::string table_name = "pkts_ddos";

  try {
    std::lock_guard<std::mutex> guard(program_mutex_);
    uint64_t pkts = 0;

    auto pkts_table = iptables_.get_percpuarray_table<uint64_t>(table_name, index_);
    auto values = pkts_table.get(rule_number);

    return std::accumulate(values.begin(), values.end(), pkts);
  } catch (...) {
    throw std::runtime_error("Counter not available.");
  }
}

uint64_t Iptables::Ddos::getBytesCount(int rule_number) {
  std::string table_name = "bytes_ddos";

  try {
    std::lock_guard<std::mutex> guard(program_mutex_);
    auto bytes_table =
        iptables_.get_percpuarray_table<uint64_t>(table_name, index_);
    auto values = bytes_table.get(rule_number);
    uint64_t bytes = 0;
    return std::accumulate(values.begin(), values.end(), bytes);
  } catch (...) {
    throw std::runtime_error("Counter not available.");
  }
}

void Iptables::Ddos::flushCounters(int rule_number) {
  std::string pkts_table_name = "pkts_ddos";
  std::string bytes_table_name = "bytes_ddos";

  try {
    auto pkts_table =
        iptables_.get_percpuarray_table<uint64_t>(pkts_table_name, index_);
    auto bytes_table =
        iptables_.get_percpuarray_table<uint64_t>(bytes_table_name, index_);

    pkts_table.set(rule_number, 0);
    bytes_table.set(rule_number, 0);
  } catch (std::exception &e) {
    throw std::runtime_error("Counters not available: " +
                             std::string(e.what()));
  }
}

void Iptables::Ddos::updateTableValue(struct DdosRule ddos_key,
                                      struct DdosValue ddos_value) {
  std::vector<std::string> key_vector;
  struct ddosKey key;

  if (CHECK_BIT(ddos_key.setFields, DdosConst::SRCIP)) {
    key.src_ip_set = true;
    key.src_ip = ddos_key.src_ip;
  }

  if (CHECK_BIT(ddos_key.setFields, DdosConst::DSTIP)) {
    key.dst_ip_set = true;
    key.dst_ip = ddos_key.dst_ip;
  }

  if (CHECK_BIT(ddos_key.setFields, DdosConst::SRCPORT)) {
    key.src_port_set = true;
    key.src_port = ddos_key.src_port;
  }

  if (CHECK_BIT(ddos_key.setFields, DdosConst::DSTPORT)) {
    key.dst_port_set = true;
    key.dst_port = ddos_key.dst_port;
  }

  if (CHECK_BIT(ddos_key.setFields, DdosConst::L4PROTO)) {
    key.l4proto_set = true;
    key.l4proto = ddos_key.l4proto;
  }

  char buffer[2 * sizeof(uint32_t) +
              2 * sizeof(uint16_t) /*+ sizeof(uint8_t)*/] = {0};
  auto table = iptables_.get_raw_table("ddosTable", index_);
  table.set(key.toData(buffer), &ddos_value);
}

void Iptables::Ddos::updateMap(
    const std::map<struct DdosRule, struct DdosValue> &ddos) {
  iptables_.logger()->info("DDoS # offloaded rules: {0} ", ddos.size());

  for (auto ele : ddos) {
    updateTableValue(ele.first, ele.second);
  }
}
