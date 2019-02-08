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

void FwdTable::create(K8switch &parent, const std::string &address,
                      const FwdTableJsonObject &conf) {
  parent.logger()->debug("Creating fwd entry ip: {0} - mac: {1} - port: {2}",
                         address, conf.getMac(), conf.getPort());

  uint32_t ip_key = get_index(address);  // takes last part of the IP address
  auto port = parent.get_port(conf.getPort());

  pod p{
      .mac = utils::mac_string_to_be_uint(conf.getMac()),
      .port = uint16_t(port->index()),
  };

  auto fwd_table = parent.get_array_table<pod>("fwd_table");
  fwd_table.set(ip_key, p);
}

std::shared_ptr<FwdTable> FwdTable::getEntry(K8switch &parent,
                                             const std::string &address) {
  uint32_t ip_key = get_index(address);  // takes last part of the IP address

  try {
    auto fwd_table = parent.get_array_table<pod>("fwd_table");
    auto entry = fwd_table.get(ip_key);

    std::string mac = utils::be_uint_to_ip_string(entry.mac);
    std::string port(parent.get_port(entry.port)->name());
    return std::make_shared<FwdTable>(FwdTable(parent, address, mac, port));
  } catch (std::exception &e) {
    parent.logger()->error(
        "Unable to find FWD table entry for address {0}. {1}", address,
        e.what());
    throw std::runtime_error("FWD table entry not found");
  }
}

void FwdTable::removeEntry(K8switch &parent, const std::string &address) {
  parent.logger()->debug("Remove the FWD table entry for address {0}", address);

  uint32_t ip_key = get_index(address);  // takes last part of the IP address

  // fwd_table is implemented as an array, deleting means writing 0s
  auto fwd_table = parent.get_array_table<pod>("fwd_table");
  pod p{0};
  fwd_table.set(ip_key, p);
}

std::vector<std::shared_ptr<FwdTable>> FwdTable::get(K8switch &parent) {
  parent.logger()->debug("Getting the FWD table");

  std::vector<std::shared_ptr<FwdTable>> fwd_table_entries;

  // The FWD table is read from the data path
  try {
    auto fwd_table = parent.get_array_table<pod>("fwd_table");
    auto entries = fwd_table.get_all();

    for (auto &entry : entries) {
      auto key = entry.first;
      auto value = entry.second;

      // TODO: key is only the index, it should be converted back to the full IP
      // address
      std::string ip = utils::be_uint_to_ip_string(key);
      std::string mac = utils::be_uint_to_mac_string(value.mac);
      std::string port(parent.get_port(value.port)->name());

      fwd_table_entries.push_back(
          std::make_shared<FwdTable>(FwdTable(parent, ip, mac, port)));
    }
  } catch (std::exception &e) {
    parent.logger()->error("Error while trying to get the FWD table");
    throw std::runtime_error("Unable to get the FWD table list");
  }

  return fwd_table_entries;
}

void FwdTable::remove(K8switch &parent) {
  throw std::runtime_error("FwdTable::remove non supported");
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
