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

// TODO: Modify these methods with your own implementation

#include "ArpTable.h"
#include "Router.h"

ArpTable::ArpTable(Router &parent, const ArpTableJsonObject &conf)
    : ArpTableBase(parent) {
  logger()->info("Creating ArpTable instance");
}

ArpTable::ArpTable(Router &parent, const std::string &mac,
                   const std::string &ip, const std::string &interface)
    : ArpTableBase(parent), mac_(mac), ip_(ip), interface_(interface) {}

ArpTable::~ArpTable() {}

std::string ArpTable::getAddress() {
  // This method retrieves the address value.
  return ip_;
}

std::string ArpTable::getMac() {
  // This method retrieves the mac value.
  return mac_;
}

void ArpTable::setMac(const std::string &value) {
  throw std::runtime_error("ArpTable::setMac: Method not implemented");
}

std::string ArpTable::getInterface() {
  // This method retrieves the interface value.
  return interface_;
}

void ArpTable::setInterface(const std::string &value) {
  throw std::runtime_error("ArpTable::setInterface: Method not implemented");
}
