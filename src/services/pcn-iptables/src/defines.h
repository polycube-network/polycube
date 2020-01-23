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

#pragma once
#include <inttypes.h>
#include "polycube/services/utils.h"

using namespace polycube::service;

namespace ModulesConstants {

// # of modules for each filtering pipeline
const uint8_t NR_MODULES = 10;
// # of initial modules, common across pipelines
const uint8_t NR_INITIAL_MODULES = 7;

// index of common modules
const uint8_t PARSER_INGRESS = 0;
const uint8_t PARSER_EGRESS = 0;

const uint8_t CHAINSELECTOR_INGRESS = 1;
const uint8_t CHAINSELECTOR_EGRESS = 1;

const uint8_t CONNTRACKLABEL_INGRESS = 2;
const uint8_t CONNTRACKLABEL_EGRESS = 2;

const uint8_t CHAINFORWARDER_INGRESS = 3;
const uint8_t CHAINFORWARDER_EGRESS = 3;

const uint8_t CONNTRACKTABLEUPDATE_INGRESS = 4;
const uint8_t CONNTRACKTABLEUPDATE_EGRESS = 4;

// index of others modules
const uint8_t HORUS_INGRESS = 5;
const uint8_t HORUS_INGRESS_SWAP = 6;

// offset of modules in filtering pipeline
const uint8_t CONNTRACKMATCH = 0;
const uint8_t IPSOURCE = 1;
const uint8_t IPDESTINATION = 2;
const uint8_t L4PROTO = 3;
const uint8_t PORTSOURCE = 4;
const uint8_t PORTDESTINATION = 5;
const uint8_t INTERFACE = 6;
const uint8_t TCPFLAGS = 7;
const uint8_t BITSCAN = 8;
const uint8_t ACTION = 9;
}

namespace ConntrackModes {
// TODO implement the possibility of disabling conntrack modules
// disable conntrack module
const uint8_t DISABLED = 0;
// enable accept established optimization
const uint8_t ON = 1;
// disable accept established optimization
const uint8_t OFF = 2;
}

enum Type {
  SOURCE_TYPE = 0,
  DESTINATION_TYPE = 1,
  IN_TYPE = 2,
  OUT_TYPE = 3,
  INVALID_TYPE = 4
};
typedef enum Type Type;
enum ActionsInt { DROP_ACTION = 0, ACCEPT_ACTION = 1, INVALID_ACTION = 2 };
typedef enum ActionsInt ActionsInt;

struct IpAddr {
  uint32_t ip;
  uint8_t netmask;
  uint32_t rule_id;
  std::string toString() const {
    return utils::nbo_uint_to_ip_string(ip) + "/" + std::to_string(netmask);
  }
  void fromString(std::string ipnetmask) {
    std::string ip_;
    uint8_t netmask_;

    std::size_t found = ipnetmask.find("/");
    if (found != std::string::npos) {
      std::string netmask = ipnetmask.substr(found + 1, std::string::npos);
      netmask_ = std::stol(netmask);
    } else {
      netmask_ = 32;
    }

    if (netmask_ > 32)
      throw std::runtime_error("Netmask can't be bigger than 32");

    ip_ = ipnetmask.substr(0, found);
    ip = utils::ip_string_to_nbo_uint(ip_);
    netmask = netmask_;
  }
  bool operator<(const IpAddr &that) const {
    return std::make_pair(this->ip, this->netmask) <
           std::make_pair(that.ip, that.netmask);
  }
};

// const used by horus optimization
namespace HorusConst {
const uint8_t SRCIP = 0;
const uint8_t DSTIP = 1;
const uint8_t L4PROTO = 2;
const uint8_t SRCPORT = 3;
const uint8_t DSTPORT = 4;

// apply Horus mitigator optimization if there are at least # rules
// matching the pattern, at ruleset begin
const uint32_t MIN_RULE_SIZE_FOR_HORUS = 1;
const uint32_t MAX_RULE_SIZE_FOR_HORUS = 2048;

// Enable Horus optimization
// We want to disable Horus by default while we decide a policy to apply it or
// not.
// The problem of incompatibility is semantic, this function could break
// iptables compatibility in INPUT/FORWARD chain
// const uint8_t ENABLE_HORUS = 0;
}

struct HorusRule {
  uint32_t src_ip;
  uint32_t dst_ip;
  uint8_t l4proto;
  uint16_t src_port;
  uint16_t dst_port;
  // bitvector representing set fields for current rule
  uint64_t setFields;

  bool operator<(const HorusRule &that) const {
    if (this->src_ip != that.src_ip)
      return (this->src_ip < that.src_ip);
    else if (this->dst_ip != that.dst_ip)
      return (this->dst_ip < that.dst_ip);
    else if (this->l4proto != that.l4proto)
      return (this->l4proto < that.l4proto);
    else if (this->src_port != that.src_port)
      return (this->src_port < that.src_port);
    else if (this->dst_port != that.dst_port)
      return (this->dst_port < that.dst_port);
    return false;
  }
} __attribute__((packed));

struct HorusValue {
  uint8_t action;
  uint32_t ruleID;
} __attribute__((packed));

#define SET_BIT(number, x) number |= ((uint64_t)1 << x);
#define CHECK_BIT(number, x) ((number) & ((uint64_t)1 << (x)))
#define FROM_NRULES_TO_NELEMENTS(x) (x / 63 + (x % 63 != 0 ? 1 : 0))
