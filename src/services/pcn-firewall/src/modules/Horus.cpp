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

#include "../Firewall.h"
#include "datapaths/Firewall_Horus_dp.h"

struct horusKey {
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

Firewall::Horus::Horus(
    const int &index, Firewall &outer, const ChainNameEnum &direction,
    const std::map<struct HorusRule, struct HorusValue> &horus)
    : Firewall::Program(firewall_code_horus, index,
                        direction, outer) {
  horus_ = horus;
  load();
}

Firewall::Horus::~Horus() {}

std::string Firewall::Horus::defaultActionString() {
  return "";
}

std::string Firewall::Horus::getCode() {
  std::string no_macro_code = code;

  struct HorusRule horus_key;
  for (auto ele : horus_) {
    horus_key = ele.first;
    break;
  }

  std::string enabled;
  std::string disabled;

  if (CHECK_BIT(horus_key.setFields, HorusConst::SRCIP)) {
    replaceAll(no_macro_code, "_SRCIP", std::to_string(1));
    enabled += "SRCIP ";
  } else {
    replaceAll(no_macro_code, "_SRCIP", std::to_string(0));
    disabled += "SRCIP ";
  }

  if (CHECK_BIT(horus_key.setFields, HorusConst::DSTIP)) {
    replaceAll(no_macro_code, "_DSTIP", std::to_string(1));
    enabled += "DSTIP ";
  } else {
    replaceAll(no_macro_code, "_DSTIP", std::to_string(0));
    disabled += "DSTIP ";
  }

  if (CHECK_BIT(horus_key.setFields, HorusConst::SRCPORT)) {
    replaceAll(no_macro_code, "_SRCPORT", std::to_string(1));
    enabled += "SRCPORT ";
  } else {
    replaceAll(no_macro_code, "_SRCPORT", std::to_string(0));
    disabled += "SRCPORT ";
  }

  if (CHECK_BIT(horus_key.setFields, HorusConst::DSTPORT)) {
    enabled += "DSTPORT ";
    replaceAll(no_macro_code, "_DSTPORT", std::to_string(1));
  } else {
    replaceAll(no_macro_code, "_DSTPORT", std::to_string(0));
    disabled += "DSTPORT ";
  }

  if (CHECK_BIT(horus_key.setFields, HorusConst::L4PROTO)) {
    replaceAll(no_macro_code, "_L4PROTO", std::to_string(1));
    enabled += "L4PROTO ";
  } else {
    replaceAll(no_macro_code, "_L4PROTO", std::to_string(0));
    disabled += "L4PROTO ";
  }

  replaceAll(no_macro_code, "_CONNTRACKLABEL",
             std::to_string(ModulesConstants::CONNTRACKLABEL));

  replaceAll(no_macro_code, "_MAX_RULE_SIZE_FOR_HORUS",
             std::to_string(HorusConst::MAX_RULE_SIZE_FOR_HORUS));


  if (firewall.getConntrack() == FirewallConntrackEnum::ON) {
    replaceAll(no_macro_code, "_CONNTRACK_ENABLED", std::to_string(1));
  } else {
    replaceAll(no_macro_code, "_CONNTRACK_ENABLED", std::to_string(0));
  }

  firewall.logger()->debug("HORUS fields ENABLED: {0} - DISABLED {1} ",
                            enabled, disabled);

  return no_macro_code;
}

uint64_t Firewall::Horus::getPktsCount(int rule_number) {
  std::string table_name = "pkts_horus";

  try {
//    std::lock_guard<std::mutex> guard(program_mutex_);
    uint64_t pkts = 0;

    auto pkts_table =
        firewall.get_percpuarray_table<uint64_t>(table_name, index, getProgramType());
    auto values = pkts_table.get(rule_number);

    return std::accumulate(values.begin(), values.end(), pkts);
  } catch (...) {
    throw std::runtime_error("Counter not available.");
  }
}

uint64_t Firewall::Horus::getBytesCount(int rule_number) {
  std::string table_name = "bytes_horus";

  try {
    auto bytes_table =
        firewall.get_percpuarray_table<uint64_t>(table_name, index, getProgramType());
    auto values = bytes_table.get(rule_number);
    uint64_t bytes = 0;
    return std::accumulate(values.begin(), values.end(), bytes);
  } catch (...) {
    throw std::runtime_error("Counter not available.");
  }
}

void Firewall::Horus::flushCounters(int rule_number) {
  std::string pkts_table_name = "pkts_horus";
  std::string bytes_table_name = "bytes_horus";

  try {
    auto pkts_table =
        firewall.get_percpuarray_table<uint64_t>(pkts_table_name, index, getProgramType());
    auto bytes_table =
        firewall.get_percpuarray_table<uint64_t>(bytes_table_name, index, getProgramType());

    pkts_table.set(rule_number, 0);
    bytes_table.set(rule_number, 0);
  } catch (std::exception &e) {
    throw std::runtime_error("Counters not available: " +
                             std::string(e.what()));
  }
}

void Firewall::Horus::updateTableValue(struct HorusRule horus_key,
                                       struct HorusValue horus_value) {
  std::vector<std::string> key_vector;
  struct horusKey key;

  if (CHECK_BIT(horus_key.setFields, HorusConst::SRCIP)) {
    key.src_ip_set = true;
    key.src_ip = horus_key.src_ip;
  }

  if (CHECK_BIT(horus_key.setFields, HorusConst::DSTIP)) {
    key.dst_ip_set = true;
    key.dst_ip = horus_key.dst_ip;
  }

  if (CHECK_BIT(horus_key.setFields, HorusConst::SRCPORT)) {
    key.src_port_set = true;
    key.src_port = horus_key.src_port;
  }

  if (CHECK_BIT(horus_key.setFields, HorusConst::DSTPORT)) {
    key.dst_port_set = true;
    key.dst_port = horus_key.dst_port;
  }

  if (CHECK_BIT(horus_key.setFields, HorusConst::L4PROTO)) {
    key.l4proto_set = true;
    key.l4proto = horus_key.l4proto;
  }

  char buffer[2 * sizeof(uint32_t) +
              2 * sizeof(uint16_t) + sizeof(uint8_t)] = {0};
  auto table = firewall.get_raw_table("horusTable", index, getProgramType());
  table.set(key.toData(buffer), &horus_value);
}

void Firewall::Horus::updateMap(
    const std::map<struct HorusRule, struct HorusValue> &horus) {
  firewall.logger()->info("HORUS # offloaded rules: {0} ", horus.size());

  for (auto ele : horus) {
    updateTableValue(ele.first, ele.second);
  }
}
