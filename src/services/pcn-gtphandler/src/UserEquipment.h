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


#pragma once


#include "../base/UserEquipmentBase.h"


struct user_equipment {
  uint32_t tunnel_endpoint;
  uint32_t teid;
};

class Gtphandler;


using namespace polycube::service::model;

class UserEquipment : public UserEquipmentBase {
 public:
  UserEquipment(Gtphandler &parent, const UserEquipmentJsonObject &conf);
  virtual ~UserEquipment();

  /// <summary>
  /// IP address of the User Equipment
  /// </summary>
  std::string getIp() override;

  /// <summary>
  /// IP address of the Base Station that connects the User Equipment
  /// </summary>
  std::string getTunnelEndpoint() override;
  void setTunnelEndpoint(const std::string &value) override;

  /// <summary>
  /// Tunnel Endpoint ID of the GTP tunnel used by the User Equipment
  /// </summary>
  uint32_t getTeid() override;
  void setTeid(const uint32_t &value) override;

  std::string toString();

 private:
  std::string ip_;
  std::string tunnel_endpoint_;
  uint32_t teid_;

  void updateDataplane();
};