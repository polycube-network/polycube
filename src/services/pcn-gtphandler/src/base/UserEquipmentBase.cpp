/**
* gtphandler API generated from gtphandler.yang
*
* NOTE: This file is auto generated by polycube-codegen
* https://github.com/polycube-network/polycube-codegen
*/


/* Do not edit this file manually */


#include "UserEquipmentBase.h"
#include "../Gtphandler.h"


UserEquipmentBase::UserEquipmentBase(Gtphandler &parent)
    : parent_(parent) {}

UserEquipmentBase::~UserEquipmentBase() {}

void UserEquipmentBase::update(const UserEquipmentJsonObject &conf) {

  if (conf.tunnelEndpointIsSet()) {
    setTunnelEndpoint(conf.getTunnelEndpoint());
  }
}

UserEquipmentJsonObject UserEquipmentBase::toJsonObject() {
  UserEquipmentJsonObject conf;

  conf.setIp(getIp());
  conf.setTunnelEndpoint(getTunnelEndpoint());

  return conf;
}

std::shared_ptr<spdlog::logger> UserEquipmentBase::logger() {
  return parent_.logger();
}
