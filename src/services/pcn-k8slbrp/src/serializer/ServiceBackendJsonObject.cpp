/**
* k8slbrp API generated from k8slbrp.yang
*
* NOTE: This file is auto generated by polycube-codegen
* https://github.com/polycube-network/polycube-codegen
*/


/* Do not edit this file manually */



#include "ServiceBackendJsonObject.h"
#include <regex>

namespace polycube {
namespace service {
namespace model {

ServiceBackendJsonObject::ServiceBackendJsonObject() {
  m_nameIsSet = false;
  m_ipIsSet = false;
  m_portIsSet = false;
  m_weightIsSet = false;
}

ServiceBackendJsonObject::ServiceBackendJsonObject(const nlohmann::json &val) :
  JsonObjectBase(val) {
  m_nameIsSet = false;
  m_ipIsSet = false;
  m_portIsSet = false;
  m_weightIsSet = false;


  if (val.count("name")) {
    setName(val.at("name").get<std::string>());
  }

  if (val.count("ip")) {
    setIp(val.at("ip").get<std::string>());
  }

  if (val.count("port")) {
    setPort(val.at("port").get<uint16_t>());
  }

  if (val.count("weight")) {
    setWeight(val.at("weight").get<uint16_t>());
  }
}

nlohmann::json ServiceBackendJsonObject::toJson() const {
  nlohmann::json val = nlohmann::json::object();
  if (!getBase().is_null()) {
    val.update(getBase());
  }

  if (m_nameIsSet) {
    val["name"] = m_name;
  }

  if (m_ipIsSet) {
    val["ip"] = m_ip;
  }

  if (m_portIsSet) {
    val["port"] = m_port;
  }

  if (m_weightIsSet) {
    val["weight"] = m_weight;
  }

  return val;
}

std::string ServiceBackendJsonObject::getName() const {
  return m_name;
}

void ServiceBackendJsonObject::setName(std::string value) {
  m_name = value;
  m_nameIsSet = true;
}

bool ServiceBackendJsonObject::nameIsSet() const {
  return m_nameIsSet;
}

void ServiceBackendJsonObject::unsetName() {
  m_nameIsSet = false;
}

std::string ServiceBackendJsonObject::getIp() const {
  return m_ip;
}

void ServiceBackendJsonObject::setIp(std::string value) {
  m_ip = value;
  m_ipIsSet = true;
}

bool ServiceBackendJsonObject::ipIsSet() const {
  return m_ipIsSet;
}



uint16_t ServiceBackendJsonObject::getPort() const {
  return m_port;
}

void ServiceBackendJsonObject::setPort(uint16_t value) {
  m_port = value;
  m_portIsSet = true;
}

bool ServiceBackendJsonObject::portIsSet() const {
  return m_portIsSet;
}



uint16_t ServiceBackendJsonObject::getWeight() const {
  return m_weight;
}

void ServiceBackendJsonObject::setWeight(uint16_t value) {
  m_weight = value;
  m_weightIsSet = true;
}

bool ServiceBackendJsonObject::weightIsSet() const {
  return m_weightIsSet;
}

void ServiceBackendJsonObject::unsetWeight() {
  m_weightIsSet = false;
}


}
}
}

