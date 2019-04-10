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
    : FdbEntryBase(parent) {}

FdbEntry::FdbEntry(Fdb &parent, const std::string &address, uint32_t entry_age,
                   uint32_t out_port)
    : FdbEntryBase(parent),
      address_(address),
      entry_age_(entry_age),
      port_no_(out_port) {
  auto port = parent.parent_.get_port(port_no_);
  port_name_ = port->name();
}

FdbEntry::~FdbEntry() {}

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
    logger()->debug("[FdbEntry] Port updated");
  } catch (std::exception &e) {
    logger()->error(
        "[FdbEntry] Error while inserting entry in the table. Reason {0}",
        e.what());
    throw;
  }
}

std::string FdbEntry::getAddress() {
  // This method retrieves the address value.
  return address_;
}
