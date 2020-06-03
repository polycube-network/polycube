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

#include "../base/GtphandlerBase.h"
#include "UserEquipment.h"

#define MAX_USER_EQUIPMENT 100000

using namespace polycube::service::model;

class Gtphandler : public GtphandlerBase {
 public:
  Gtphandler(const std::string name, const GtphandlerJsonObject &conf);
  virtual ~Gtphandler();

  void packet_in(polycube::service::Direction direction,
                 polycube::service::PacketInMetadata &md,
                 const std::vector<uint8_t> &packet) override;

  /// <summary>
  /// User Equipment connected with GTP tunnel
  /// </summary>
  std::shared_ptr<UserEquipment> getUserEquipment(
      const std::string &ip) override;
  std::vector<std::shared_ptr<UserEquipment>> getUserEquipmentList() override;
  void addUserEquipment(const std::string &ip,
                        const UserEquipmentJsonObject &conf) override;
  void addUserEquipmentList(
      const std::vector<UserEquipmentJsonObject> &conf) override;
  void replaceUserEquipment(const std::string &ip,
                            const UserEquipmentJsonObject &conf) override;
  void delUserEquipment(const std::string &ip) override;
  void delUserEquipmentList() override;

 private:
  std::string local_ip_;
  std::string local_mac_;
  std::unordered_map<std::string, std::shared_ptr<UserEquipment>>
      user_equipment_map_;

  std::string getWrapperCode();
  void reloadCode();
};
