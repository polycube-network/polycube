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

void ArpEntry::create(Router &parent, const std::string &address,
                      const ArpEntryJsonObject &conf) {
  // This method creates the actual ArpEntry object given thee key param.
  // Please remember to call here the create static method for all sub-objects
  // of ArpEntry.

  parent.logger()->debug(
      "Creating ARP entry [ip: {0} - mac: {1} - interface: {2}", address,
      conf.getMac(), conf.getInterface());

  uint64_t mac = utils::mac_string_to_be_uint(conf.getMac());
  uint32_t index = parent.get_port(conf.getInterface())->index();

  // FIXME: Check if entry already exists?
  auto arp_table = parent.get_hash_table<uint32_t, arp_entry>("arp_table");
  arp_table.set(utils::ip_string_to_be_uint(address),
                arp_entry{.mac = mac, .port = index});
}

std::shared_ptr<ArpEntry> ArpEntry::getEntry(Router &parent,
                                             const std::string &address) {
  // This method retrieves the pointer to ArpEntry object specified by its keys.

  parent.logger()->debug("Getting the ARP table entry for address: {0}",
                         address);

  uint32_t ip_key = utils::ip_string_to_be_uint(address);

  try {
    auto arp_table = parent.get_hash_table<uint32_t, arp_entry>("arp_table");

    arp_entry entry = arp_table.get(ip_key);

    std::string mac = utils::be_uint_to_mac_string(entry.mac);

    auto port = parent.get_port(entry.port);

    parent.logger()->debug(
        "Returning entry [ip: {0} - mac: {1} - interface: {2}]", address, mac,
        port->name());

    return std::make_shared<ArpEntry>(
        ArpEntry(parent, mac, address, port->name()));
  } catch (std::exception &e) {
    parent.logger()->error(
        "Unable to find ARP table entry for address {0}. {1}", address,
        e.what());
    throw std::runtime_error("ARP table entry not found");
  }
}

void ArpEntry::removeEntry(Router &parent, const std::string &address) {
  // This method removes the single ArpEntry object specified by its keys.
  // Remember to call here the remove static method for all-sub-objects of
  // ArpEntry.

  parent.logger()->debug("Remove the ARP table entry for address {0}", address);

  std::shared_ptr<ArpEntry> entry;

  try {
    entry = ArpEntry::getEntry(parent, address);
  } catch (std::exception &e) {
    parent.logger()->error(
        "Unable to remove the ARP table entry for address {0}. {1}", address,
        e.what());
    throw std::runtime_error("ARP table entry not found");
  }

  uint32_t key = utils::ip_string_to_be_uint(address);

  auto arp_table = parent.get_hash_table<uint32_t, arp_entry>("arp_table");

  try {
    arp_table.remove(key);
  } catch (...) {
    throw std::runtime_error("ARP table entry not found");
  }

  parent.logger()->debug(
      "ARP table entry deleted [ip: {0} - mac: {1} - interface: {2}]",
      entry->ip_, entry->mac_, entry->interface_);
}

std::vector<std::shared_ptr<ArpEntry>> ArpEntry::get(Router &parent) {
  // This methods get the pointers to all the ArpEntry objects in Router.

  parent.logger()->debug("Getting the ARP table");

  std::vector<std::shared_ptr<ArpEntry>> arp_table_entries;

  // The ARP table is read from the data path
  try {
    auto arp_table = parent.get_hash_table<uint32_t, arp_entry>("arp_table");
    auto arp_entries = arp_table.get_all();

    for (auto &entry : arp_entries) {
      auto key = entry.first;
      auto value = entry.second;

      std::string ip = utils::be_uint_to_ip_string(key);
      std::string mac = utils::be_uint_to_mac_string(value.mac);

      auto port = parent.get_port(value.port);

      parent.logger()->debug(
          "Returning entry [ip: {0} - mac: {1} - interface: {2}]", ip, mac,
          port->name());

      arp_table_entries.push_back(
          std::make_shared<ArpEntry>(ArpEntry(parent, mac, ip, port->name())));
    }
  } catch (std::exception &e) {
    parent.logger()->error("Error while trying to get the ARP table");
    throw std::runtime_error("Unable to get the ARP table list");
  }

  return arp_table_entries;
}

void ArpEntry::remove(Router &parent) {
  // This method removes all ArpEntry objects in Router.
  // Remember to call here the remove static method for all-sub-objects of
  // ArpEntry.

  parent.logger()->info("Removing all the entries in teh ARP table");

  try {
    auto arp_table = parent.get_hash_table<uint32_t, arp_entry>("arp_table");
    arp_table.remove_all();
  } catch (std::exception &e) {
    parent.logger()->error(
        "Error while removing all entries from the ARP table. {0}", e.what());
    throw std::runtime_error(
        "Error while removing all entries from the ARP table");
  }

  parent.logger()->info("The ARP table is empty");
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
