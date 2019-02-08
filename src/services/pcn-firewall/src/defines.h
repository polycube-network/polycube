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
// Nr of modules for each chain: conntrackmatch, ipsrc, ipdst,
// l4proto, srcport, dstport, flags, bitscan, action

// Modules common between chains (at the beginning): Parser, conntracklabel,
// chainforwarder
// Modules common between chains (at the end): ConntrackTableUpdate

const uint8_t NR_MODULES = 9;

const uint8_t PARSER = 0;
const uint8_t CONNTRACKLABEL = 1;
const uint8_t CHAINFORWARDER = 2;

const uint8_t CONNTRACKMATCH = 3;
const uint8_t IPSOURCE = 4;
const uint8_t IPDESTINATION = 5;
const uint8_t L4PROTO = 6;
const uint8_t PORTSOURCE = 7;
const uint8_t PORTDESTINATION = 8;
const uint8_t TCPFLAGS = 9;
const uint8_t BITSCAN = 10;
const uint8_t ACTION = 11;

const uint8_t DEFAULTACTION = 12;
const uint8_t CONNTRACKTABLEUPDATE = 13;
}

namespace ConntrackModes {
const uint8_t DISABLED = 0; /* Conntrack label not injected at all. */
const uint8_t MANUAL = 1;   /* No automatic forward */
const uint8_t AUTOMATIC = 2;
}

enum Type { SOURCE_TYPE = 0, DESTINATION_TYPE = 1, INVALID_TYPE = 2 };
typedef enum Type Type;
enum ActionsInt { DROP_ACTION = 0, FORWARD_ACTION = 1, INVALID_ACTION = 2 };
typedef enum ActionsInt ActionsInt;

struct IpAddr {
  uint32_t ip;
  uint8_t netmask;
  uint32_t ruleId;
  std::string toString() const {
    return utils::be_uint_to_ip_string(ip) + "/" + std::to_string(netmask);
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
    ip = utils::ip_string_to_be_uint(ip_);
    netmask = netmask_;
  }
  bool operator<(const IpAddr &that) const {
    return std::make_pair(this->ip, this->netmask) <
           std::make_pair(that.ip, that.netmask);
  }
};

#define SET_BIT(number, x) number |= ((uint64_t)1 << x);
#define CHECK_BIT(number, x) ((number) & ((uint64_t)1 << (x)))
#define FROM_NRULES_TO_NELEMENTS(x) (x / 63 + (x % 63 != 0 ? 1 : 0))
