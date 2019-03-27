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

#include "FwdTable.h"
#include "K8switch.h"

using namespace polycube::service;

FwdTable::FwdTable(K8switch &parent, const FwdTableJsonObject &conf)
    : parent_(parent) {
  logger()->info("Creating FwdTable instance");
}

FwdTable::FwdTable(K8switch &parent, const std::string &ip,
                   const std::string &mac, const std::string &port)
    : parent_(parent), ip_(ip), mac_(mac), port_(port) {}

FwdTable::~FwdTable() {}

void FwdTable::update(const FwdTableJsonObject &conf) {
  if (conf.macIsSet()) {
    setMac(conf.getMac());
  }
  if (conf.portIsSet()) {
    setPort(conf.getPort());
  }
}

FwdTableJsonObject FwdTable::toJsonObject() {
  FwdTableJsonObject conf;

  conf.setAddress(getAddress());
  conf.setMac(getMac());
  conf.setPort(getPort());

  return conf;
}

std::string FwdTable::getAddress() {
  return ip_;
}

std::string FwdTable::getMac() {
  return mac_;
}

void FwdTable::setMac(const std::string &value) {
  // This method set the mac value.
  throw std::runtime_error("[FwdTable]: Method setMac not implemented");
}

std::string FwdTable::getPort() {
  return port_;
}

void FwdTable::setPort(const std::string &value) {
  // This method set the port value.
  throw std::runtime_error("[FwdTable]: Method setPort not implemented");
}

std::shared_ptr<spdlog::logger> FwdTable::logger() {
  return parent_.logger();
}

// TODO: this function should evolve and consider also a netmask
uint32_t FwdTable::get_index(const std::string &ip) {
  auto dot_index = ip.find_last_of('.');
  return std::stol(ip.substr(dot_index + 1, std::string::npos));
}
