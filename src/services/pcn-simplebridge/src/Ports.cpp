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

#include "Ports.h"
#include "Simplebridge.h"

Ports::Ports(polycube::service::Cube<Ports> &parent,
             std::shared_ptr<polycube::service::PortIface> port,
             const PortsJsonObject &conf)
    : PortsBase(parent, port) {}

Ports::~Ports() {
  try {
    auto fwdtable = parent_.get_hash_table<uint64_t, fwd_entry>("fwdtable");
    auto fdb = fwdtable.get_all();

    for (auto &pair : fdb) {
      auto map_key = pair.first;
      auto map_entry = pair.second;
      if (map_entry.port == index()) {
        fwdtable.remove(map_key);
        std::string mac_address = utils::nbo_uint_to_mac_string(map_key);
        logger()->debug("Removing entry {0} associated to port {1}",
                        mac_address, name());
      }
    }
  } catch (std::exception &e) {
    logger()->error(
        "Error while trying to get the Filtering database table: {0}",
        e.what());
  }
}
