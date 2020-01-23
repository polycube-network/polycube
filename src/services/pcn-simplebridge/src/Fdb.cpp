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

#include "Fdb.h"
#include "Simplebridge.h"

Fdb::Fdb(Simplebridge &parent, const FdbJsonObject &conf)
    : FdbBase(parent) {
  logger()->info("Creating Fdb instance");

  if (conf.agingTimeIsSet()) {
    setAgingTime(conf.getAgingTime());
  }

  if (conf.entryIsSet()) {
    // FIXME: What if this method doesn't succeed
    Fdb::addEntryList(conf.getEntry());
  }
}

Fdb::Fdb(Simplebridge &parent) : FdbBase(parent), agingTime_(0) {}

Fdb::~Fdb() {}

uint32_t Fdb::getAgingTime() {
  // This method retrieves the agingTime value.
  return agingTime_;
}

void Fdb::setAgingTime(const uint32_t &value) {
  // This method set the agingTime value.
  if (agingTime_ != value) {
    try {
      parent_.reloadCodeWithAgingtime(value);
    } catch (std::exception &e) {
      logger()->error("Failed to reload the code with the new aging time {0}",
                      value);
    }
    agingTime_ = value;
  } else {
    logger()->debug(
        "[Fdb] Request to set aging time but the value is the same as before");
  }
}

FdbFlushOutputJsonObject Fdb::flush() {
  FdbFlushOutputJsonObject result;
  try {
    delEntryList();
  } catch (std::exception &e) {
    logger()->error("Error while flushing the filtering database. Reason: {0}",
                    e.what());
    result.setFlushed(false);
    return result;
  }
  result.setFlushed(true);
  return result;
}

std::shared_ptr<FdbEntry> Fdb::getEntry(const std::string &address) {
  logger()->debug("[FdbEntry] Received request to read map entry");
  logger()->debug("[FdbEntry] mac: {0}", address);

  auto fwdtable = parent_.get_hash_table<uint64_t, fwd_entry>("fwdtable");
  uint64_t key = utils::mac_string_to_nbo_uint(address);

  try {
    auto entry = FdbEntry::constructFromMap(*this, address, fwdtable.get(key));
    if (!entry) {
      throw std::runtime_error("Map entry not found");
    }
    return std::move(entry);
  } catch (std::exception &e) {
    logger()->error("[FdbEntry] Unable to read the map key. {0}", e.what());
    throw std::runtime_error("Map entry not found");
  }

  logger()->debug("[FdbEntry] Filtering database entry read successfully");
}

std::vector<std::shared_ptr<FdbEntry>> Fdb::getEntryList() {
  std::vector<std::shared_ptr<FdbEntry>> fdb_entry_list;

  try {
    auto fwdtable = parent_.get_hash_table<uint64_t, fwd_entry>("fwdtable");
    auto fdb = fwdtable.get_all();

    for (auto &pair : fdb) {
      auto map_key = pair.first;
      auto map_entry = pair.second;

      std::string mac_address = utils::nbo_uint_to_mac_string(map_key);
      auto entry = FdbEntry::constructFromMap(*this, mac_address, map_entry);
      if (!entry)
        continue;
      fdb_entry_list.push_back(std::move(entry));
    }
  } catch (std::exception &e) {
    logger()->error(
        "[FdbEntry] Error while trying to get the Filtering database table: "
        "{0}",
        e.what());
    throw std::runtime_error("Unable to get filtering database");
  }

  return fdb_entry_list;
}

void Fdb::addEntry(const std::string &address, const FdbEntryJsonObject &conf) {
  logger()->debug(
      "[FdbEntry] Received request to create new filtering database entry");
  logger()->debug("[FdbEntry] mac: {0}", address);

  if (!conf.portIsSet()) {
    logger()->error(
        "[FdbEntry] You should specify the output port for a STATIC entry");
    throw std::runtime_error(
        "You should specify the output port for a STATIC entry");
  }

  uint32_t port_index;
  try {
    port_index = parent_.get_port(conf.getPort())->index();
  } catch (...) {
    throw std::runtime_error("Port " + conf.getPort() + " doesn't exist");
  }

  struct timespec now_timespec;
  clock_gettime(CLOCK_MONOTONIC, &now_timespec);

  uint64_t key = utils::mac_string_to_nbo_uint(address);
  fwd_entry value{
      .timestamp = (uint32_t)now_timespec.tv_sec, .port = port_index,
  };

  try {
    auto fwdtable = parent_.get_hash_table<uint64_t, fwd_entry>("fwdtable");
    fwdtable.set(key, value);
    logger()->debug("[FdbEntry] Entry inserted");
  } catch (std::exception &e) {
    logger()->error(
        "[FdbEntry] Error while inserting entry in the table. Reason {0}",
        e.what());
    throw;
  }
}

void Fdb::addEntryList(const std::vector<FdbEntryJsonObject> &conf) {
  FdbBase::addEntryList(conf);
}

void Fdb::replaceEntry(const std::string &address,
                       const FdbEntryJsonObject &conf) {
  FdbBase::replaceEntry(address, conf);
}

void Fdb::delEntry(const std::string &address) {
  logger()->debug("[FdbEntry] Received request to read map entry");
  logger()->debug("[FdbEntry] mac: {0}", address);

  std::shared_ptr<FdbEntry> fdb_entry;

  try {
    fdb_entry = getEntry(address);
  } catch (std::exception &e) {
    logger()->error("[FdbEntry] Unable to read the map key. {0}", e.what());
    throw std::runtime_error("Filtering database entry not found");
  }

  uint64_t key = utils::mac_string_to_nbo_uint(address);

  try {
    auto fwdtable = parent_.get_hash_table<uint64_t, fwd_entry>("fwdtable");
    fwdtable.remove(key);
  } catch (...) {
    throw std::runtime_error("[MapEntry] does not exist");
  }
}

void Fdb::delEntryList() {
  logger()->debug(
      "[FdbEntry] Removing all entries from the filtering database table");

  try {
    auto fwdtable = parent_.get_hash_table<uint64_t, fwd_entry>("fwdtable");
    fwdtable.remove_all();
  } catch (std::exception &e) {
    logger()->error(
        "[FdbEntry] Error while removing entries from filtering database "
        "table: {0}",
        e.what());
    throw std::runtime_error(
        "Error while removing all entries from filtering database table");
  }

  logger()->info(
      "[MapEntry] All entries removed from the filtering database table");
}
