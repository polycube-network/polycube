/**
* k8sdispatcher API generated from k8sdispatcher.yang
*
* NOTE: This file is auto generated by polycube-codegen
* https://github.com/polycube-network/polycube-codegen
*/


/* Do not edit this file manually */

/*
* PortsBase.h
*
*
*/

#pragma once

#include "../serializer/PortsJsonObject.h"




#include "polycube/services/cube.h"
#include "polycube/services/port.h"

#include "polycube/services/utils.h"
#include "polycube/services/fifo_map.hpp"

#include <spdlog/spdlog.h>

using namespace polycube::service::model;

class K8sdispatcher;
class Ports;

class PortsBase: public polycube::service::Port {
 public:
  PortsBase(polycube::service::Cube<Ports> &parent,
      std::shared_ptr<polycube::service::PortIface> port);
  
  virtual ~PortsBase();
  virtual void update(const PortsJsonObject &conf);
  virtual PortsJsonObject toJsonObject();

  /// <summary>
  /// Type of the k8sdispatcher port (e.g. BACKEND or FRONTEND)
  /// </summary>
  virtual PortsTypeEnum getType() = 0;
  virtual void setType(const PortsTypeEnum &value) = 0;

  /// <summary>
  /// IP address of the node interface (only for FRONTEND port)
  /// </summary>
  virtual std::string getIp() = 0;
  virtual void setIp(const std::string &value) = 0;

  std::shared_ptr<spdlog::logger> logger();
 protected:
  K8sdispatcher &parent_;
};
