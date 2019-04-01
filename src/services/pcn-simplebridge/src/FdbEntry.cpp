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

#include "FdbEntry.h"
#include <time.h>
#include <cinttypes>
#include "Simplebridge.h"

using namespace polycube::service;

FdbEntry::FdbEntry(Fdb &parent, const FdbEntryJsonObject &conf)
    : parent_(parent) {
  logger()->info("Creating FdbEntry instance");
}

FdbEntry::FdbEntry(Fdb &parent, const std::string &address, uint32_t entry_age,
                   uint32_t out_port)
    : parent_(parent),
      address_(address),
      entry_age_(entry_age),
      port_no_(out_port) {
  auto port = parent.parent_.get_port(port_no_);
  port_name_ = port->name();
}

FdbEntry::~FdbEntry() {}

void FdbEntry::update(const FdbEntryJsonObject &conf) {
  // This method updates all the object/parameter in FdbEntry object specified
  // in the conf JsonObject.
  // You can modify this implementation.
  if (conf.portIsSet()) {
    setPort(conf.getPort());
  }
}

FdbEntryJsonObject FdbEntry::toJsonObject() {
  FdbEntryJsonObject conf;

  conf.setAge(getAge());
  conf.setPort(getPort());
  conf.setAddress(getAddress());

  return conf;
}

void FdbEntry::create(Fdb &parent, const std::string &address,
                      const FdbEntryJsonObject &conf) {
  parent.logger()->debug(
      "[FdbEntry] Received request to create new filtering database entry");
  parent.logger()->debug("[FdbEntry] mac: {0}", address);

  if (!conf.portIsSet()) {
    parent.logger()->error(
        "[FdbEntry] You should specify the output port for a STATIC entry");
    throw std::runtime_error(
        "You should specify the output port for a STATIC entry");
  }

  uint32_t port_index;
  try {
    port_index = parent.parent_.get_port(conf.getPort())->index();
  } catch (...) {
    throw std::runtime_error("Port " + conf.getPort() + " doesn't exist");
  }

  struct timespec now_timespec;
  clock_gettime(CLOCK_MONOTONIC, &now_timespec);

  uint64_t key = utils::mac_string_to_be_uint(address);
  fwd_entry value{
      .timestamp = (uint32_t)now_timespec.tv_sec, .port = port_index,
  };

  try {
    auto fwdtable =
        parent.parent_.get_hash_table<uint64_t, fwd_entry>("fwdtable");
    fwdtable.set(key, value);
    parent.logger()->debug("[FdbEntry] Entry inserted");
  } catch (std::exception &e) {
    parent.logger()->error(
        "[FdbEntry] Error while inserting entry in the table. Reason {0}",
        e.what());
    throw;
  }
}

std::shared_ptr<FdbEntry> FdbEntry::constructFromMap(Fdb &parent,
                                                     const std::string &address,
                                                     const fwd_entry &value) {
  struct timespec now_timespec;
  clock_gettime(CLOCK_MONOTONIC, &now_timespec);
  uint32_t now = now_timespec.tv_sec;

  uint64_t key = utils::mac_string_to_be_uint(address);
  uint32_t timestamp = value.timestamp;

  //Here I'm going to check if the entry in the filtering database is too old.\
  //In this case, I'll not show the entry
  if ((now - timestamp) > parent.getAgingTime()) {
    parent.logger()->debug("Ignoring old entry: now {0}, last_seen: {1}", now,
                           timestamp);
    auto fwdtable =
        parent.parent_.get_hash_table<uint64_t, fwd_entry>("fwdtable");
    fwdtable.remove(key);
    return nullptr;
  }
  uint32_t entry_age = now - timestamp;
  uint32_t port_no = value.port;
  return std::make_shared<FdbEntry>(parent, address, entry_age, port_no);
}

std::shared_ptr<FdbEntry> FdbEntry::getEntry(Fdb &parent,
                                             const std::string &address) {
  // This method retrieves the pointer to FdbEntry object specified by its keys.
  parent.logger()->debug("[FdbEntry] Received request to read map entry");
  parent.logger()->debug("[FdbEntry] mac: {0}", address);

  auto fwdtable =
      parent.parent_.get_hash_table<uint64_t, fwd_entry>("fwdtable");
  uint64_t key = utils::mac_string_to_be_uint(address);

  try {
    auto entry = constructFromMap(parent, address, fwdtable.get(key));
    if (!entry) {
      throw std::runtime_error("Map entry not found");
    }
    return std::move(entry);
  } catch (std::exception &e) {
    parent.logger()->error("[FdbEntry] Unable to read the map key. {0}",
                           e.what());
    throw std::runtime_error("Map entry not found");
  }

  parent.logger()->debug(
      "[FdbEntry] Filtering database entry read successfully");
}

void FdbEntry::removeEntry(Fdb &parent, const std::string &address) {
  parent.logger()->debug("[FdbEntry] Received request to read map entry");
  parent.logger()->debug("[FdbEntry] mac: {0}", address);

  std::shared_ptr<FdbEntry> fdb_entry;

  try {
    fdb_entry = FdbEntry::getEntry(parent, address);
  } catch (std::exception &e) {
    parent.logger()->error("[FdbEntry] Unable to read the map key. {0}",
                           e.what());
    throw std::runtime_error("Filtering database entry not found");
  }

  uint64_t key = utils::mac_string_to_be_uint(address);

  try {
    auto fwdtable =
        parent.parent_.get_hash_table<uint64_t, fwd_entry>("fwdtable");
    fwdtable.remove(key);
  } catch (...) {
    throw std::runtime_error("[MapEntry] does not exist");
  }
}

std::vector<std::shared_ptr<FdbEntry>> FdbEntry::get(Fdb &parent) {
  // This methods get the pointers to all the FdbEntry objects in Fdb.
  std::vector<std::shared_ptr<FdbEntry>> fdb_entry_list;

  try {
    auto fwdtable =
        parent.parent_.get_hash_table<uint64_t, fwd_entry>("fwdtable");
    auto fdb = fwdtable.get_all();

    for (auto &pair : fdb) {
      auto map_key = pair.first;
      auto map_entry = pair.second;

      std::string mac_address = utils::be_uint_to_mac_string(map_key);
      auto entry = constructFromMap(parent, mac_address, map_entry);
      if (!entry)
        continue;
      fdb_entry_list.push_back(std::move(entry));
    }
  } catch (std::exception &e) {
    parent.logger()->error(
        "[FdbEntry] Error while trying to get the Filtering database table: "
        "{0}",
        e.what());
    throw std::runtime_error("Unable to get filtering database");
  }

  return fdb_entry_list;
}

void FdbEntry::remove(Fdb &parent) {
  parent.logger()->debug(
      "[FdbEntry] Removing all entries from the filtering database table");

  try {
    auto fwdtable =
        parent.parent_.get_hash_table<uint64_t, fwd_entry>("fwdtable");
    fwdtable.remove_all();
  } catch (std::exception &e) {
    parent.logger()->error(
        "[FdbEntry] Error while removing entries from filtering database "
        "table: {0}",
        e.what());
    throw std::runtime_error(
        "Error while removing all entries from filtering database table");
  }

  parent.logger()->info(
      "[MapEntry] All entries removed from the filtering database table");
}

uint32_t FdbEntry::getAge() {
  // This method retrieves the age value.
  return entry_age_;
}

std::string FdbEntry::getPort() {
  // This method retrieves the port value.
  return port_name_;
}

void FdbEntry::setPort(const std::string &value) {
  // This method set the port value.
  uint32_t port_index;
  try {
    port_index = parent_.parent_.get_port(value)->index();
  } catch (...) {
    throw std::runtime_error("Port " + value + " doesn't exist");
  }

  uint64_t map_key = utils::mac_string_to_be_uint(address_);
  fwd_entry map_value{
      .timestamp = 0, .port = port_index,
  };

  try {
    auto fwdtable =
        parent_.parent_.get_hash_table<uint64_t, fwd_entry>("fwdtable");
    fwdtable.set(map_key, map_value);
    parent_.logger()->debug("[FdbEntry] Port updated");
  } catch (std::exception &e) {
    parent_.logger()->error(
        "[FdbEntry] Error while inserting entry in the table. Reason {0}",
        e.what());
    throw;
  }
}

std::string FdbEntry::getAddress() {
  // This method retrieves the address value.
  return address_;
}

std::shared_ptr<spdlog::logger> FdbEntry::logger() {
  return parent_.logger();
}
