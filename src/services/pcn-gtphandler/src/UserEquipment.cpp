/*
 * Copyright 2020 The Polycube Authors
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

#include "UserEquipment.h"
#include "Gtphandler.h"

using namespace polycube::service;

UserEquipment::UserEquipment(Gtphandler &parent,
                             const UserEquipmentJsonObject &conf)
    : UserEquipmentBase(parent) {
  ip_ = conf.getIp();
  tunnel_endpoint_ = conf.getTunnelEndpoint();

  updateDataplane();

  logger()->info("User equipment created {0}", toString());
}

UserEquipment::~UserEquipment() {
  parent_.get_hash_table<uint32_t, uint32_t>("user_equipment_map")
         .remove(utils::ip_string_to_nbo_uint(ip_));

  logger()->info("User equipment {0} deleted", ip_);
}

std::string UserEquipment::getIp() {
  return ip_;
}

std::string UserEquipment::getTunnelEndpoint() {
  return tunnel_endpoint_;
}

void UserEquipment::setTunnelEndpoint(const std::string &value) {
  if (tunnel_endpoint_ == value) {
    return;
  }

  tunnel_endpoint_ = value;

  updateDataplane();

  logger()->info("User equipment updated {0}", toString());
}

std::string UserEquipment::toString() {
  return toJsonObject().toJson().dump();
}

void UserEquipment::updateDataplane() {
  parent_.get_hash_table<uint32_t, uint32_t>("user_equipment_map")
         .set(utils::ip_string_to_nbo_uint(ip_),
              utils::ip_string_to_nbo_uint(tunnel_endpoint_));
}