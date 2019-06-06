/*
 * Copyright 2019 The Polycube Authors
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

#include "Fdb.h"
#include "Bridge.h"

Fdb::Fdb(Bridge &parent, const FdbJsonObject &conf) : FdbBase(parent) {
  logger()->info("Creating Fdb instance");

  agingTime_ = conf.getAgingTime();

  if (conf.entryIsSet()) {
    Fdb::addEntryList(conf.getEntry());
  }
}

Fdb::~Fdb() {
  logger()->info("Destroying Fdb instance");
}

uint32_t Fdb::getAgingTime() {
  return agingTime_;
}

void Fdb::setAgingTime(const uint32_t &value) {
  if (agingTime_ == value)
    return;

  try {
    parent_.reloadCodeWithAgingTime(value);
  } catch (std::exception &e) {
    logger()->error("Failed to reload the code with the new aging time {0}",
                    value);
  }
  agingTime_ = value;
}

std::shared_ptr<FdbEntry> Fdb::getEntry(const uint16_t &vlan,
                                        const std::string &mac) {
  std::lock_guard<std::mutex> guard(fdb_mutex_);

  logger()->debug("[Fdb] Received request to read map entry");
  logger()->debug("[Fdb] vlan: {0} mac: {1}", vlan, mac);

  auto fwdtable = parent_.get_hash_table<fwd_key, fwd_entry>("fwdtable");
  fwd_key key{
      .vlan = vlan,
      .mac = polycube::service::utils::mac_string_to_be_uint(mac),
  };

  try {
    auto entry = FdbEntry::constructFromMap(*this, key, fwdtable.get(key));
    if (!entry) {
      throw std::runtime_error("Map entry not found");
    }

    return std::move(entry);
  } catch (std::exception &e) {
    logger()->error("[Fdb] Unable to read the map key. {0}", e.what());
    throw std::runtime_error("Map entry not found");
  }

  logger()->debug("[Fdb] Filtering database entry read successfully");
}

std::vector<std::shared_ptr<FdbEntry>> Fdb::getEntryList() {
  std::lock_guard<std::mutex> guard(fdb_mutex_);

  std::vector<std::shared_ptr<FdbEntry>> fdb_entry_list;

  try {
    auto fwdtable = parent_.get_hash_table<fwd_key, fwd_entry>("fwdtable");
    auto fdb = fwdtable.get_all();

    for (auto &pair : fdb) {
      auto map_key = pair.first;
      auto map_entry = pair.second;

      auto entry = FdbEntry::constructFromMap(*this, map_key, map_entry);
      if (!entry)
        continue;

      fdb_entry_list.push_back(std::move(entry));
    }
  } catch (std::exception &e) {
    logger()->error(
        "[Fdb] Error while trying to get the Filtering database table: "
        "{0}",
        e.what());
    throw std::runtime_error("Unable to get filtering database");
  }

  return fdb_entry_list;
}

void Fdb::addEntry(const uint16_t &vlan, const std::string &mac,
                   const FdbEntryJsonObject &conf) {
  std::lock_guard<std::mutex> guard(fdb_mutex_);

  if (vlan == 0 || vlan > 4094) {
    throw std::runtime_error("Not a valid VLAN");
  }

  logger()->debug("[Fdb] vlan: {0} mac: {1}", vlan, mac);

  if (!conf.portIsSet()) {
    logger()->error(
        "[Fdb] You should specify the output port for a STATIC entry");
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
  uint32_t timestamp = now_timespec.tv_sec;

  fwd_key key{
      .vlan = vlan,
      .mac = polycube::service::utils::mac_string_to_be_uint(mac),
  };

  fwd_entry value{
      .timestamp = timestamp,
      .port = port_index,
      .type = STATIC,
  };

  try {
    auto fwdtable = parent_.get_hash_table<fwd_key, fwd_entry>("fwdtable");
    fwdtable.set(key, value);
    logger()->debug("[Fdb] Entry inserted");
  } catch (std::exception &e) {
    logger()->error(
        "[Fdb] Error while inserting entry in the table. Reason {0}", e.what());
    throw;
  }
}

void Fdb::addEntryList(const std::vector<FdbEntryJsonObject> &conf) {
  FdbBase::addEntryList(conf);
}

void Fdb::replaceEntry(const uint16_t &vlan, const std::string &mac,
                       const FdbEntryJsonObject &conf) {
  FdbBase::replaceEntry(vlan, mac, conf);
}

void Fdb::delEntry(const uint16_t &vlan, const std::string &mac) {
  std::lock_guard<std::mutex> guard(fdb_mutex_);

  logger()->debug("[Fdb] vlan: {0} mac: {1}", vlan, mac);

  std::shared_ptr<FdbEntry> fdb_entry;

  try {
    fdb_entry = getEntry(vlan, mac);
  } catch (std::exception &e) {
    logger()->error("[Fdb] Unable to read the map key. {0}", e.what());
    throw std::runtime_error("Filtering database entry not found");
  }

  fwd_key key{
      .vlan = vlan,
      .mac = polycube::service::utils::mac_string_to_be_uint(mac),
  };

  try {
    auto fwdtable = parent_.get_hash_table<fwd_key, fwd_entry>("fwdtable");
    fwdtable.remove(key);
  } catch (...) {
    throw std::runtime_error("[MapEntry] does not exist");
  }
}

void Fdb::delEntryList() {
  std::lock_guard<std::mutex> guard(fdb_mutex_);

  auto fwdtable = parent_.get_hash_table<fwd_key, fwd_entry>("fwdtable");
  fwdtable.remove_all();
}

void Fdb::flush() {
  try {
    delEntryList();
  } catch (std::exception &e) {
    logger()->error("Error while flushing the filtering database. Reason: {0}",
                    e.what());
  }
}

void Fdb::flushByPort(int port) {
  std::lock_guard<std::mutex> guard(fdb_mutex_);

  logger()->trace("Flushing the fdb entries associated to port {0}", port);

  auto fwdtable = parent_.get_hash_table<fwd_key, fwd_entry>("fwdtable");
  auto fdb = fwdtable.get_all();

  for (auto &pair : fdb) {
    auto map_key = pair.first;
    auto map_entry = pair.second;

    if (map_entry.port == port)
      fwdtable.remove(map_key);
  }
}

void Fdb::flushOldEntries(uint16_t vlan, uint32_t maxAge) {
  std::lock_guard<std::mutex> guard(fdb_mutex_);

  logger()->trace("Flushing old fdb entries associated to vlan {0}", vlan);

  auto fwdtable = parent_.get_hash_table<fwd_key, fwd_entry>("fwdtable");
  auto fdb = fwdtable.get_all();

  struct timespec now_timespec;
  clock_gettime(CLOCK_MONOTONIC, &now_timespec);
  uint32_t now = now_timespec.tv_sec;

  for (auto &pair : fdb) {
    auto map_key = pair.first;
    auto map_entry = pair.second;

    if (map_entry.type == STATIC)
      continue;
    if (map_key.vlan != vlan)
      continue;

    if (now - map_entry.timestamp <= maxAge)
      continue;

    fwdtable.remove(map_key);
  }
}
