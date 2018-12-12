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

#include "SessionTable.h"
#include "Iptables.h"

using namespace polycube::service;

SessionTable::SessionTable(Iptables &parent, const SessionTableJsonObject &conf)
    : parent_(parent) {
  // logger()->trace("Creating SessionTable instance");
  fields = conf;
}

SessionTable::~SessionTable() {}

void SessionTable::update(const SessionTableJsonObject &conf) {}

SessionTableJsonObject SessionTable::toJsonObject() {
  SessionTableJsonObject conf;

  conf.setSrc(getSrc());

  conf.setDst(getDst());

  conf.setState(getState());

  conf.setL4proto(getL4proto());

  conf.setDport(getDport());

  conf.setSport(getSport());

  return conf;
}

void SessionTable::create(Iptables &parent, const std::string &src,
                          const std::string &dst, const std::string &l4proto,
                          const uint16_t &sport, const uint16_t &dport,
                          const SessionTableJsonObject &conf) {
  // This method creates the actual SessionTable object given thee key param.
  // Please remember to call here the create static method for all sub-objects
  // of SessionTable.

  throw std::runtime_error("[SessionTable]: Method create not allowed");
}

std::shared_ptr<SessionTable> SessionTable::getEntry(
    Iptables &parent, const std::string &src, const std::string &dst,
    const std::string &l4proto, const uint16_t &sport, const uint16_t &dport) {
  // This method retrieves the pointer to SessionTable object specified by its
  // keys.
  throw std::runtime_error("[SessionTable]: Method getEntry not allowed");
}

void SessionTable::removeEntry(Iptables &parent, const std::string &src,
                               const std::string &dst,
                               const std::string &l4proto,
                               const uint16_t &sport, const uint16_t &dport) {
  // This method removes the single SessionTable object specified by its keys.
  // Remember to call here the remove static method for all-sub-objects of
  // SessionTable.
  throw std::runtime_error("[SessionTable]: Method removeEntry not allowed");
}

std::vector<std::shared_ptr<SessionTable>> SessionTable::get(Iptables &parent) {
  std::vector<std::pair<ct_k, ct_v>> connections =
      dynamic_cast<Iptables::ConntrackLabel *>(
          parent
              .programs_[std::make_pair(ModulesConstants::CONNTRACKLABEL_INGRESS,
                                       ChainNameEnum::INVALID_INGRESS)])
          ->getMap();

  std::vector<std::shared_ptr<SessionTable>> session_table;
  SessionTableJsonObject conf;

  for (auto connection : connections) {
    auto key = connection.first;
    auto value = connection.second;

    if (value.ipRev) {
      conf.setSrc(utils::be_uint_to_ip_string(key.dstIp));
      conf.setDst(utils::be_uint_to_ip_string(key.srcIp));
    } else {
      conf.setSrc(utils::be_uint_to_ip_string(key.srcIp));
      conf.setDst(utils::be_uint_to_ip_string(key.dstIp));
    }

    conf.setL4proto(ChainRule::protocolFromIntToString(key.l4proto));

    if (value.portRev) {
      conf.setSport(ntohs(key.dstPort));
      conf.setDport(ntohs(key.srcPort));
    } else {
      conf.setSport(ntohs(key.srcPort));
      conf.setDport(ntohs(key.dstPort));
    }

    conf.setState(stateFromNumberToString(value.state));
    // conf.setEta(from_ttl_to_eta(value.ttl, value.state, key.l4proto));

    session_table.push_back(
        std::shared_ptr<SessionTable>(new SessionTable(parent, conf)));
  }
  return session_table;
}

void SessionTable::remove(Iptables &parent) {
  // This method removes all SessionTable objects in Iptables.
  // Remember to call here the remove static method for all-sub-objects of
  // SessionTable.
  throw std::runtime_error("[SessionTable]: Method remove not allowed");
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

std::string SessionTable::stateFromNumberToString(int state) {
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
