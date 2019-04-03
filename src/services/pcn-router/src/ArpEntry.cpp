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

#include "ArpEntry.h"
#include "Router.h"

ArpEntry::ArpEntry(Router &parent, const ArpEntryJsonObject &conf)
    : parent_(parent) {
  logger()->info("Creating ArpEntry instance");
}

ArpEntry::ArpEntry(Router &parent, const std::string &mac,
                   const std::string &ip, const std::string &interface)
    : parent_(parent), mac_(mac), ip_(ip), interface_(interface) {}

ArpEntry::~ArpEntry() {}

void ArpEntry::update(const ArpEntryJsonObject &conf) {
  // This method updates all the object/parameter in ArpEntry object specified
  // in the conf JsonObject.
  // You can modify this implementation.

  setInterface(conf.getInterface());

  setMac(conf.getMac());
}

ArpEntryJsonObject ArpEntry::toJsonObject() {
  ArpEntryJsonObject conf;

  conf.setInterface(getInterface());

  conf.setMac(getMac());

  conf.setAddress(getAddress());

  return conf;
}

std::string ArpEntry::getInterface() {
  // This method retrieves the interface value.
  return interface_;
}

void ArpEntry::setInterface(const std::string &value) {
  // This method set the interface value.
  throw std::runtime_error("[ArpEntry]: Method setInterface not implemented");
}

std::string ArpEntry::getMac() {
  // This method retrieves the mac value.
  return mac_;
}

void ArpEntry::setMac(const std::string &value) {
  // This method set the mac value.
  throw std::runtime_error("[ArpEntry]: Method setMac not implemented");
}

std::string ArpEntry::getAddress() {
  // This method retrieves the address value.
  return ip_;
}

std::shared_ptr<spdlog::logger> ArpEntry::logger() {
  return parent_.logger();
}
