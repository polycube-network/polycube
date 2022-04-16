/**
* gtphandler API generated from gtphandler.yang
*
* NOTE: This file is auto generated by polycube-codegen
* https://github.com/polycube-network/polycube-codegen
*/


/* Do not edit this file manually */



#include "GtphandlerJsonObject.h"
#include <regex>

namespace polycube {
namespace service {
namespace model {

GtphandlerJsonObject::GtphandlerJsonObject() {
  m_nameIsSet = false;
  m_userEquipmentIsSet = false;
}

GtphandlerJsonObject::GtphandlerJsonObject(const nlohmann::json &val) :
  JsonObjectBase(val) {
  m_nameIsSet = false;
  m_userEquipmentIsSet = false;


  if (val.count("name")) {
    setName(val.at("name").get<std::string>());
  }

  if (val.count("user-equipment")) {
    for (auto& item : val["user-equipment"]) {
      UserEquipmentJsonObject newItem{ item };
      m_userEquipment.push_back(newItem);
    }

    m_userEquipmentIsSet = true;
  }
}

nlohmann::json GtphandlerJsonObject::toJson() const {
  nlohmann::json val = nlohmann::json::object();
  if (!getBase().is_null()) {
    val.update(getBase());
  }

  if (m_nameIsSet) {
    val["name"] = m_name;
  }

  {
    nlohmann::json jsonArray;
    for (auto& item : m_userEquipment) {
      jsonArray.push_back(JsonObjectBase::toJson(item));
    }

    if (jsonArray.size() > 0) {
      val["user-equipment"] = jsonArray;
    }
  }

  return val;
}

std::string GtphandlerJsonObject::getName() const {
  return m_name;
}

void GtphandlerJsonObject::setName(std::string value) {
  m_name = value;
  m_nameIsSet = true;
}

bool GtphandlerJsonObject::nameIsSet() const {
  return m_nameIsSet;
}



const std::vector<UserEquipmentJsonObject>& GtphandlerJsonObject::getUserEquipment() const{
  return m_userEquipment;
}

void GtphandlerJsonObject::addUserEquipment(UserEquipmentJsonObject value) {
  m_userEquipment.push_back(value);
  m_userEquipmentIsSet = true;
}


bool GtphandlerJsonObject::userEquipmentIsSet() const {
  return m_userEquipmentIsSet;
}

void GtphandlerJsonObject::unsetUserEquipment() {
  m_userEquipmentIsSet = false;
}


}
}
}

