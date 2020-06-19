/**
* gtphandler API generated from gtphandler.yang
*
* NOTE: This file is auto generated by polycube-codegen
* https://github.com/polycube-network/polycube-codegen
*/


/* Do not edit this file manually */

/*
* GtphandlerBase.h
*
*
*/

#pragma once

#include "../serializer/GtphandlerJsonObject.h"

#include "../UserEquipment.h"

#include "polycube/services/transparent_cube.h"



#include "polycube/services/utils.h"
#include "polycube/services/fifo_map.hpp"

#include <spdlog/spdlog.h>

using namespace polycube::service::model;


class GtphandlerBase: public virtual polycube::service::TransparentCube {
 public:
  GtphandlerBase(const std::string name);
  
  virtual ~GtphandlerBase();
  virtual void update(const GtphandlerJsonObject &conf);
  virtual GtphandlerJsonObject toJsonObject();

  /// <summary>
  /// User Equipment connected with GTP tunnel
  /// </summary>
  virtual std::shared_ptr<UserEquipment> getUserEquipment(const std::string &ip) = 0;
  virtual std::vector<std::shared_ptr<UserEquipment>> getUserEquipmentList() = 0;
  virtual void addUserEquipment(const std::string &ip, const UserEquipmentJsonObject &conf) = 0;
  virtual void addUserEquipmentList(const std::vector<UserEquipmentJsonObject> &conf);
  virtual void replaceUserEquipment(const std::string &ip, const UserEquipmentJsonObject &conf);
  virtual void delUserEquipment(const std::string &ip) = 0;
  virtual void delUserEquipmentList();
};