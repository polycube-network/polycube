#include "MetricJsonObject.h"
#include <regex>

namespace polycube {
  namespace service {
    namespace model {

      MetricJsonObject::MetricJsonObject() {
        m_nameIsSet = false;
        m_valueIsSet = false;
        m_timestampIsSet = false;
      }

      MetricJsonObject::MetricJsonObject(const nlohmann::json &val) : JsonObjectBase(val) {
        m_nameIsSet = false;
        m_valueIsSet = false;
        m_timestampIsSet = false;
        if (val.count("name"))
          setName(val.at("name").get<std::string>());
        if (val.count("value"))
          setValue(val.at("value"));
        if (val.count("timestamp"))
          setTimestamp(val.at("timestamp").get<int64_t>());
      }

      nlohmann::json MetricJsonObject::toJson() const {
        nlohmann::json val = nlohmann::json::object();
        if (!getBase().is_null())
          val.update(getBase());
        if (m_nameIsSet)
          val["name"] = m_name;
        if (m_valueIsSet)
          val["value"] = m_value;
        if (m_timestampIsSet)
          val["timestamp"] = m_timestamp;
        return val;
      }

      std::string MetricJsonObject::getName() const {
        return m_name;
      }

      void MetricJsonObject::setName(std::string value) {
        m_name = value;
        m_nameIsSet = true;
      }

      bool MetricJsonObject::nameIsSet() const {
        return m_nameIsSet;
      }

      nlohmann::json MetricJsonObject::getValue() const {
        return m_value;
      }

      void MetricJsonObject::setValue(nlohmann::json value) {
        m_value = value;
        m_valueIsSet = true;
      }

      bool MetricJsonObject::valueIsSet() const {
        return m_valueIsSet;
      }

      void MetricJsonObject::unsetValue() {
        m_valueIsSet = false;
      }

      int64_t MetricJsonObject::getTimestamp() const {
        return m_timestamp;
      }

      void MetricJsonObject::setTimestamp(int64_t value) {
        m_timestamp = value;
        m_timestampIsSet = true;
      }

      bool MetricJsonObject::timestampIsSet() const {
        return m_timestampIsSet;
      }

      void MetricJsonObject::unsetTimestamp() {
        m_timestampIsSet = false;
      }
    }// namespace model
  }// namespace service
}// namespace polycube
