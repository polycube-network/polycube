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
