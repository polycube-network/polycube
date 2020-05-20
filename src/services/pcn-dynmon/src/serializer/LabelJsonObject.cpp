#include "LabelJsonObject.h"
#include <regex>

namespace polycube {
  namespace service {
    namespace model {

      LabelJsonObject::LabelJsonObject() {
        m_nameIsSet = false;
        m_valueIsSet = false;
      }

      LabelJsonObject::LabelJsonObject(const nlohmann::json &val) : JsonObjectBase(val) {
        m_nameIsSet = false;
        m_valueIsSet = false;
        if (val.count("name"))
          setName(val.at("name").get<std::string>());
        if (val.count("value"))
          setValue(val.at("value").get<std::string>());
      }

      nlohmann::json LabelJsonObject::toJson() const {
        nlohmann::json val = nlohmann::json::object();
        if (!getBase().is_null())
          val.update(getBase());
        if (m_nameIsSet)
          val["name"] = m_name;
        if (m_valueIsSet)
          val["value"] = m_value;
        return val;
      }

      std::string LabelJsonObject::getName() const {
        return m_name;
      }

      void LabelJsonObject::setName(std::string value) {
        m_name = value;
        m_nameIsSet = true;
      }

      bool LabelJsonObject::nameIsSet() const {
        return m_nameIsSet;
      }

      std::string LabelJsonObject::getValue() const {
        return m_value;
      }

      void LabelJsonObject::setValue(std::string value) {
        m_value = value;
        m_valueIsSet = true;
      }

      bool LabelJsonObject::valueIsSet() const {
        return m_valueIsSet;
      }
    }// namespace model
  }// namespace service
}// namespace polycube
