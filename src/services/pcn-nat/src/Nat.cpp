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

Nat::Nat(const std::string name, const NatJsonObject &conf, CubeType type)
    : TransparentCube(
          name,
          {generate_code() + nat_code_common + nat_code_ingress + nat_code},
          {generate_code() + nat_code_common + nat_code_egress + nat_code},
          type, conf.getPolycubeLoglevel()) {
  logger()->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [Nat] [%n] [%l] %v");
  logger()->info("Creating Nat instance");

  addRule(conf.getRule());
  addNattingTableList(conf.getNattingTable());
}

Nat::~Nat() {}

void Nat::update(const NatJsonObject &conf) {
  // This method updates all the object/parameter in Nat object specified in the
  // conf JsonObject.
  // You can modify this implementation.
  if (conf.loglevelIsSet()) {
    setLoglevel(conf.getLoglevel());
  }
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
  conf.setName(getName());
  conf.setUuid(getUuid());
  conf.setType(getType());
  conf.setLoglevel(getLoglevel());
  conf.setRule(getRule()->toJsonObject());

  for (auto &i : getNattingTableList()) {
    conf.addNattingTable(i->toJsonObject());
  }
  return conf;
}

void Nat::packet_in(polycube::service::Sense sense,
                    polycube::service::PacketInMetadata &md,
                    const std::vector<uint8_t> &packet) {
  logger()->info("packet in event");
}

void Nat::attach() {
  try {
    auto temp = get_parent_parameter("ip");
    external_ip_ = temp.substr(1, temp.length() - 2);  // remove qoutes
    logger()->info("external ip is : {}", external_ip_);
  } catch (...) {
    logger()->warn("External IP not found. Is this enabled on a router?");
  }
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

std::vector<std::string> Nat::generate_code_vector() {
  throw std::runtime_error("Method not implemented");
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
