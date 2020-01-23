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

// Modify these methods with your own implementation

#include "Nat.h"
#include "Nat_dp.h"
#include "Nat_dp_common.h"
#include "Nat_dp_egress.h"
#include "Nat_dp_ingress.h"

Nat::Nat(const std::string name, const NatJsonObject &conf)
    : TransparentCube(
          conf.getBase(),
          {generate_code() + nat_code_common + nat_code_ingress + nat_code},
          {generate_code() + nat_code_common + nat_code_egress + nat_code}) {
  logger()->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [Nat] [%n] [%l] %v");
  logger()->info("Creating Nat instance");

  addRule(conf.getRule());
  //addNattingTableList(conf.getNattingTable());

  ParameterEventCallback cb = [&](const std::string &parameter, const std::string &value) {
    external_ip_ = utils::get_ip_from_string(value);
    logger()->debug("parent IP has been updated to {}", external_ip_);
    if (rule_->getMasquerade()->getEnabled()) {
      rule_->getMasquerade()->inject(utils::ip_string_to_nbo_uint(external_ip_));
    }
  };
  subscribe_parent_parameter("ip", cb);
}

Nat::~Nat() {
  unsubscribe_parent_parameter("ip");
}

void Nat::update(const NatJsonObject &conf) {
  // This method updates all the object/parameter in Nat object specified in the
  // conf JsonObject.
  // You can modify this implementation.
  TransparentCube::set_conf(conf.getBase());

  if (conf.ruleIsSet()) {
    auto m = getRule();
    m->update(conf.getRule());
  }
  if (conf.nattingTableIsSet()) {
    for (auto &i : conf.getNattingTable()) {
      auto internalSrc = i.getInternalSrc();
      auto internalDst = i.getInternalDst();
      auto internalSport = i.getInternalSport();
      auto internalDport = i.getInternalDport();
      auto proto = i.getProto();
      auto m = getNattingTable(internalSrc, internalDst, internalSport,
                               internalDport, proto);
      m->update(i);
    }
  }
}

NatJsonObject Nat::toJsonObject() {
  NatJsonObject conf;
  conf.setBase(TransparentCube::to_json());

  conf.setRule(getRule()->toJsonObject());

  for (auto &i : getNattingTableList()) {
    conf.addNattingTable(i->toJsonObject());
  }
  return conf;
}

void Nat::packet_in(polycube::service::Direction direction,
                    polycube::service::PacketInMetadata &md,
                    const std::vector<uint8_t> &packet) {
  logger()->info("packet in event");
}

std::string Nat::generate_code() {
  std::ostringstream defines;

  defines << "#define NAT_SRC (" << (int)NattingTableOriginatingRuleEnum::SNAT
          << ")" << std::endl;
  defines << "#define NAT_DST (" << (int)NattingTableOriginatingRuleEnum::DNAT
          << ")" << std::endl;
  defines << "#define NAT_MSQ ("
          << (int)NattingTableOriginatingRuleEnum::MASQUERADE << ")"
          << std::endl;
  defines << "#define NAT_PFW ("
          << (int)NattingTableOriginatingRuleEnum::PORTFORWARDING << ")"
          << std::endl;
  return defines.str() /*+ nat_code*/;
}

std::string Nat::getExternalIpString() {
  return external_ip_;
}

std::string Nat::proto_from_int_to_string(const uint8_t proto) {
  switch (proto) {
  case IPPROTO_ICMP:
    return "icmp";
  case IPPROTO_TCP:
    return "tcp";
  case IPPROTO_UDP:
    return "udp";
  default:
    // Never happens
    return "unknown";
  }
}

uint8_t Nat::proto_from_string_to_int(const std::string &proto) {
  if (proto == "icmp" || proto == "ICMP") {
    return IPPROTO_ICMP;
  }
  if (proto == "tcp" || proto == "TCP") {
    return IPPROTO_TCP;
  }
  if (proto == "udp" || proto == "UDP") {
    return IPPROTO_UDP;
  }
  return -1;
}

std::shared_ptr<Rule> Nat::getRule() {
  return rule_;
}

void Nat::addRule(const RuleJsonObject &value) {
  rule_ = std::make_shared<Rule>(*this, value);
}

void Nat::replaceRule(const RuleJsonObject &conf) {
  delRule();
  addRule(conf);
}

void Nat::delRule() {
  rule_ = nullptr;
}

std::shared_ptr<NattingTable> Nat::getNattingTable(
    const std::string &internalSrc, const std::string &internalDst,
    const uint16_t &internalSport, const uint16_t &internalDport,
    const std::string &proto) {
  try {
    auto table = get_hash_table<st_k, st_v>("egress_session_table");
    st_k map_key{
        .src_ip = utils::ip_string_to_nbo_uint(internalSrc),
        .dst_ip = utils::ip_string_to_nbo_uint(internalDst),
        .src_port = htons(internalSport),
        .dst_port = htons(internalDport),
        .proto = uint8_t(std::stol(proto)),
    };

    st_v value = table.get(map_key);

    std::string newIp = utils::nbo_uint_to_ip_string(value.new_ip);
    uint16_t newPort = value.new_port;
    uint8_t originatingRule = value.originating_rule_type;
    ;
    auto entry = std::make_shared<NattingTable>(
        *this, internalSrc, internalDst, internalSport, internalDport,
        proto_from_string_to_int(proto), newIp, newPort, originatingRule);
    return entry;
  } catch (std::exception &e) {
    throw std::runtime_error("Natting table entry not found");
  }
}

std::vector<std::shared_ptr<NattingTable>> Nat::getNattingTableList() {
  std::vector<std::shared_ptr<NattingTable>> entries;
  try {
    auto table = get_hash_table<st_k, st_v>("egress_session_table");
    auto map_entries = table.get_all();
    for (auto &pair : map_entries) {
      auto key = pair.first;
      auto value = pair.second;

      auto entry = std::make_shared<NattingTable>(
          *this, utils::nbo_uint_to_ip_string(key.src_ip),
          utils::nbo_uint_to_ip_string(key.dst_ip), ntohs(key.src_port),
          ntohs(key.dst_port), key.proto,
          utils::nbo_uint_to_ip_string(value.new_ip), ntohs(value.new_port),
          value.originating_rule_type);

      entries.push_back(entry);
    }
  } catch (std::exception &e) {
    throw std::runtime_error("Unable to get the natting table");
  }
  return entries;
}

void Nat::addNattingTable(const std::string &internalSrc,
                          const std::string &internalDst,
                          const uint16_t &internalSport,
                          const uint16_t &internalDport,
                          const std::string &proto,
                          const NattingTableJsonObject &conf) {
  throw std::runtime_error("Cannot manually create natting table entries");
}

void Nat::addNattingTableList(const std::vector<NattingTableJsonObject> &conf) {
  throw std::runtime_error("Cannot manually create natting table entries");
}

void Nat::replaceNattingTable(const std::string &internalSrc,
                              const std::string &internalDst,
                              const uint16_t &internalSport,
                              const uint16_t &internalDport,
                              const std::string &proto,
                              const NattingTableJsonObject &conf) {
  throw std::runtime_error("Cannot manually create natting table entries");
}

void Nat::delNattingTable(const std::string &internalSrc,
                          const std::string &internalDst,
                          const uint16_t &internalSport,
                          const uint16_t &internalDport,
                          const std::string &proto) {
  throw std::runtime_error(
      "Cannot manually remove single natting table entries");
}

void Nat::delNattingTableList() {
  auto egress_table = get_hash_table<st_k, st_v>("egress_session_table");
  egress_table.remove_all();
  auto ingress_table = get_hash_table<st_k, st_v>("ingress_session_table");
  ingress_table.remove_all();

  logger()->info("Flushed natting tables");
}
