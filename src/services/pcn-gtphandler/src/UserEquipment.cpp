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

UserEquipment::UserEquipment(Gtphandler &parent, const UserEquipmentJsonObject &conf)
    : UserEquipmentBase(parent) {
  ip_ = conf.getIp();
  tunnel_endpoint_ = conf.getTunnelEndpoint();
  teid_ = conf.getTeid();

  updateDataplane();

  logger()->info("User equipment created {0}", toString());
}

UserEquipment::~UserEquipment() {
  parent_.get_hash_table<uint32_t, struct user_equipment>("user_equipments")
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

uint32_t UserEquipment::getTeid() {
  return teid_;
}

void UserEquipment::setTeid(const uint32_t &value) {
  if (teid_ == value) {
    return;
  }

  teid_ = value;

  updateDataplane();

  logger()->info("User equipment updated {0}", toString());
}

std::string UserEquipment::toString() {
  std::ostringstream out;

  out << "[ip=" << ip_
      << "; tunel-endpoint=" << tunnel_endpoint_
      << "; teid=" << teid_ << "]";

  return out.str();
}

void UserEquipment::updateDataplane() {
  struct user_equipment ue = {
    .tunnel_endpoint = utils::ip_string_to_nbo_uint(tunnel_endpoint_),
    .teid = htonl(teid_)
  };

  parent_.get_hash_table<uint32_t, struct user_equipment>("user_equipments")
         .set(utils::ip_string_to_nbo_uint(ip_), ue);
}