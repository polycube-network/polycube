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

#include "SessionTable.h"
#include "ChainRule.h"
#include "Firewall.h"

using namespace polycube::service;

SessionTable::SessionTable(Firewall &parent, const SessionTableJsonObject &conf)
    : SessionTableBase(parent) {
  fields = conf;
}

SessionTable::~SessionTable() {}

void SessionTable::update(const SessionTableJsonObject &conf) {}

SessionTableJsonObject SessionTable::toJsonObject() {
  SessionTableJsonObject conf;

  conf.setSrc(getSrc());

  conf.setDst(getDst());

  conf.setState(getState());

  conf.setEta(getEta());

  conf.setL4proto(getL4proto());

  conf.setDport(getDport());

  conf.setSport(getSport());

  return conf;
}

uint32_t SessionTable::getEta() {
  return fields.getEta();
}

std::string SessionTable::getSrc() {
  return fields.getSrc();
}

std::string SessionTable::getDst() {
  return fields.getDst();
}

std::string SessionTable::getState() {
  return fields.getState();
}

std::string SessionTable::getL4proto() {
  return fields.getL4proto();
}

uint16_t SessionTable::getDport() {
  return fields.getDport();
}

uint16_t SessionTable::getSport() {
  return fields.getSport();
}

std::shared_ptr<spdlog::logger> SessionTable::logger() {
  return parent_.logger();
}

std::string SessionTable::state_from_number_to_string(int state) {
  switch (state) {
  case (NEW):
    return "NEW";
  case (ESTABLISHED):
    return "ESTABLISHED";
  case (SYN_SENT):
    return "SYN_SENT";
  case (SYN_RECV):
    return "SYN_RECV";
  case (FIN_WAIT_1):
    return "FIN_WAIT_1";
  case (FIN_WAIT_2):
    return "FIN_WAIT_2";
  case (LAST_ACK):
    return "LAST_ACK";
  case (TIME_WAIT):
    return "TIME_WAIT";
  }
  throw std::runtime_error("[SessionTable]: Error!");
}

uint32_t SessionTable::from_ttl_to_eta(uint64_t ttl, uint16_t state,
                                       uint16_t l4proto) {
  if (state == NEW) {
    if (l4proto == IPPROTO_UDP) {
      ttl = ttl - UDP_NEW_TIMEOUT;
    } else {
      ttl = ttl - ICMP_TIMEOUT;
    }
  }
  if (state == ESTABLISHED) {
    if (l4proto == IPPROTO_TCP) {
      ttl = ttl - TCP_ESTABLISHED;
    } else {
      ttl = ttl - UDP_ESTABLISHED_TIMEOUT;
    }
  }
  if (state == SYN_SENT) {
    ttl = ttl - TCP_SYN_SENT;
  }
  if (state == SYN_RECV) {
    ttl = ttl - TCP_SYN_RECV;
  }
  if (state == FIN_WAIT_1 || state == FIN_WAIT_2) {
    ttl = ttl - TCP_FIN_WAIT;
  }
  if (state == LAST_ACK) {
    ttl = ttl - TCP_LAST_ACK;
  }
  if (state == TIME_WAIT) {
    ttl = ttl - TCP_TIME_WAIT;
  }

  ttl = ttl / 1000000000;
  std::chrono::nanoseconds uptime(0u);
  double uptime_seconds;
  if (!(std::ifstream("/proc/uptime", std::ios::in) >> uptime_seconds)) {
    return 0;
  } else {
    return uptime_seconds - ttl;
  }

return 0;
}

//uint64_t SessionTable::hex_string_to_uint64(const std::string &str) {
//  uint64_t x;
//  std::stringstream ss;
//  ss << std::hex << str;
//  ss >> x;
//
//  return x;
//}
