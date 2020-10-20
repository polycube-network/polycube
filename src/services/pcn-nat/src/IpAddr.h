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

#pragma once

#include <inttypes.h>
#include "polycube/services/utils.h"

using namespace polycube::service;

struct IpAddr {
  uint32_t ip;
  uint8_t netmask;
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
    ip = utils::ip_string_to_nbo_uint(ip_) & ntohl(0xffffffff << (32 - netmask_));
    netmask = netmask_;
  }
  bool operator<(const IpAddr &that) const {
    return std::make_pair(this->ip, this->netmask) <
           std::make_pair(that.ip, that.netmask);
  }
};
