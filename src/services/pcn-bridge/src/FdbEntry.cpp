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

#include "FdbEntry.h"
#include "Bridge.h"

FdbEntry::FdbEntry(Fdb &parent, const FdbEntryJsonObject &conf)
    : FdbEntryBase(parent) {
  vlan_ = conf.getVlan();
  mac_ = conf.getMac();
  type_ = conf.getType();

  port_name_ = conf.getPort();
  port_ = parent_.parent_.get_port(port_name_)->index();
}

FdbEntry::FdbEntry(Fdb &parent, uint16_t vlan, const std::string &address,
                   uint32_t entry_age, uint32_t out_port,
                   const FdbEntryTypeEnum type)
    : FdbEntryBase(parent),
      vlan_(vlan),
      mac_(address),
      age_(entry_age),
      port_(out_port),
      type_(type) {
  auto port = parent_.parent_.get_port(out_port);
  port_name_ = port->name();
}

FdbEntry::~FdbEntry() {}

uint16_t FdbEntry::getVlan() {
  return vlan_;
}

std::string FdbEntry::getMac() {
  return mac_;
}

FdbEntryTypeEnum FdbEntry::getType() {
  return type_;
}

std::string FdbEntry::getPort() {
  return port_name_;
}

void FdbEntry::setPort(const std::string &value) {
  if (type_ == FdbEntryTypeEnum::DYNAMIC) {
    throw std::runtime_error(
        "Cannot set the port of a dynamic entry. Create a static one.");
  }

  uint32_t port_index;
  try {
    port_index = parent_.parent_.get_port(value)->index();
  } catch (...) {
    throw std::runtime_error("Port " + value + " doesn't exist");
  }

  fwd_key key{
      .vlan = vlan_,
      .mac = polycube::service::utils::mac_string_to_be_uint(mac_),
  };

  struct timespec now_timespec;
  clock_gettime(CLOCK_MONOTONIC, &now_timespec);

  fwd_entry entry{
      .timestamp = (uint32_t)now_timespec.tv_sec,
      .port = port_index,
      .type = static_cast<enum entry_type>(type_),
  };

  try {
    auto fwdtable =
        parent_.parent_.get_hash_table<fwd_key, fwd_entry>("fwdtable");

    fwdtable.set(key, entry);
    logger()->debug("[FdbEntry] Port updated");
  } catch (std::exception &e) {
    logger()->error(
        "[FdbEntry] Error while inserting entry in the table. Reason {0}",
        e.what());
    throw;
  }

  // update fields of this object
  age_ = 0;
  port_ = port_index;
  port_name_ = value;
}

uint32_t FdbEntry::getAge() {
  return age_;
}

std::shared_ptr<FdbEntry> FdbEntry::constructFromMap(Fdb &parent,
                                                     const fwd_key &key,
                                                     const fwd_entry &value) {
  struct timespec now_timespec;
  clock_gettime(CLOCK_MONOTONIC, &now_timespec);
  uint32_t now = now_timespec.tv_sec;

  uint32_t timestamp = value.timestamp;
  FdbEntryTypeEnum type = static_cast<FdbEntryTypeEnum>(value.type);

  //Here I'm going to check if the entry in the filtering database is too old.\
  //In this case, I'll not show the entry
  if (type == FdbEntryTypeEnum::DYNAMIC &&
      (now - timestamp) > parent.getAgingTime()) {
    parent.logger()->debug(
        "Ignoring old dynamic entry: now {0}, last_seen: {1}", now, timestamp);

    auto fwdtable =
        parent.parent_.get_hash_table<fwd_key, fwd_entry>("fwdtable");
    fwdtable.remove(key);
    return nullptr;
  }

  uint16_t vlan = key.vlan;
  std::string mac = polycube::service::utils::be_uint_to_mac_string(key.mac);
  uint32_t entry_age = now - timestamp;
  uint32_t port_no = value.port;

  return std::make_shared<FdbEntry>(parent, vlan, mac, entry_age, port_no,
                                    type);
}
