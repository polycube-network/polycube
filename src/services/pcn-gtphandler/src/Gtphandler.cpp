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


#include "Gtphandler.h"
#include "Gtphandler_dp_ingress.h"
#include "Gtphandler_dp_egress.h"


using namespace polycube::service;

Gtphandler::Gtphandler(const std::string name, const GtphandlerJsonObject &conf)
  : TransparentCube(conf.getBase(), {}, {}),
    GtphandlerBase(name) {
  logger()->info("Creating Gtphandler instance");

  add_program(getWrapperCode() + gtphandler_code_ingress, 0, ProgramType::INGRESS);
  add_program(getWrapperCode() + gtphandler_code_egress, 0, ProgramType::EGRESS);

  addUserEquipmentList(conf.getUserEquipment());

  ParameterEventCallback cb_ip = [&](const std::string &parameter, const std::string &value) {
    local_ip_ = utils::get_ip_from_string(value);
    logger()->info("Local IP set to {0}", local_ip_);

    if (local_mac_ != "" && local_ip_ != "") {
      reloadCode();
    }
  };
  subscribe_parent_parameter("ip", cb_ip);

  ParameterEventCallback cb_mac = [&](const std::string &parameter, const std::string &value) {
    local_mac_ = value;
    logger()->info("Local MAC set to {0}", local_mac_);

    if (local_mac_ != "" && local_ip_ != "") {
      reloadCode();
    }
  };
  subscribe_parent_parameter("mac", cb_mac);
}


Gtphandler::~Gtphandler() {
  logger()->info("Destroying Gtphandler instance");
  unsubscribe_parent_parameter("ip");
  unsubscribe_parent_parameter("mac");
}

void Gtphandler::packet_in(polycube::service::Direction direction,
    polycube::service::PacketInMetadata &md,
    const std::vector<uint8_t> &packet) {
    logger()->debug("Packet received");
}

std::shared_ptr<UserEquipment> Gtphandler::getUserEquipment(const std::string &ip) {
  if (user_equipments_.count(ip) == 0) {
    throw std::runtime_error("No user equipment with the given ip");
  }

  return user_equipments_.at(ip);
}

std::vector<std::shared_ptr<UserEquipment>> Gtphandler::getUserEquipmentList() {
  std::vector<std::shared_ptr<UserEquipment>> user_equipments_v;

  user_equipments_v.reserve(user_equipments_v.size());

  for (auto const &entry : user_equipments_) {
    user_equipments_v.push_back(entry.second);
  }

  return user_equipments_v;
}

void Gtphandler::addUserEquipment(const std::string &ip, const UserEquipmentJsonObject &conf) {
  if (user_equipments_.count(ip) != 0) {
    throw std::runtime_error("User equipment with the given ip already registered");
  }

  if (user_equipments_.size() == MAX_USER_EQUIPMENTS) {
    throw std::runtime_error("Maximum number of user equipments reached");
  }

  user_equipments_[ip] = std::make_shared<UserEquipment>(*this, conf);
}

// Basic default implementation, place your extension here (if needed)
void Gtphandler::addUserEquipmentList(const std::vector<UserEquipmentJsonObject> &conf) {
  // call default implementation in base class
  GtphandlerBase::addUserEquipmentList(conf);
}

// Basic default implementation, place your extension here (if needed)
void Gtphandler::replaceUserEquipment(const std::string &ip, const UserEquipmentJsonObject &conf) {
  // call default implementation in base class
  GtphandlerBase::replaceUserEquipment(ip, conf);
}

void Gtphandler::delUserEquipment(const std::string &ip) {
  if (user_equipments_.count(ip) == 0) {
    throw std::runtime_error("No user equipment with the given ip");
  }

  user_equipments_.erase(ip);
}

// Basic default implementation, place your extension here (if needed)
void Gtphandler::delUserEquipmentList() {
  // call default implementation in base class
  GtphandlerBase::delUserEquipmentList();
}

std::string Gtphandler::getWrapperCode() {
  std::ostringstream wrapper_code;
  wrapper_code << "#define MAX_USER_EQUIPMENTS " << MAX_USER_EQUIPMENTS << "\n";

  if (local_ip_ != "") {
    wrapper_code << "#define LOCAL_IP " << utils::ip_string_to_nbo_uint(local_ip_) << "\n";
  }

  if (local_mac_ != "") {
    wrapper_code << "#define LOCAL_MAC " << utils::mac_string_to_nbo_uint(local_mac_) << "\n";
  }
  
  return wrapper_code.str();
}

void Gtphandler::reloadCode() {
  std::string wrapper_code = getWrapperCode();

  std::string ingress_code = wrapper_code + gtphandler_code_ingress;
  std::string egress_code = wrapper_code + gtphandler_code_egress;

  reload(ingress_code, 0, ProgramType::INGRESS);
  reload(egress_code, 0, ProgramType::EGRESS);

  logger()->info("Dataplane code reloaded");
}