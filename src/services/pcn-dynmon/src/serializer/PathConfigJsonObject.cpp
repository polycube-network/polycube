#include "PathConfigJsonObject.h"
#include <regex>

namespace polycube {
  namespace service {
    namespace model {

      PathConfigJsonObject::PathConfigJsonObject() {
        m_nameIsSet = false;
        m_codeIsSet = false;
        m_metricConfigsIsSet = false;
      }

      PathConfigJsonObject::PathConfigJsonObject(const nlohmann::json &val) : JsonObjectBase(val) {
        m_nameIsSet = false;
        m_codeIsSet = false;
        m_metricConfigsIsSet = false;
        if (val.count("name"))
          setName(val.at("name").get<std::string>());
        if (val.count("code"))
          setCode(val.at("code").get<std::string>());
        if (val.count("metric-configs")) {
          for (auto &item : val["metric-configs"]) {
            MetricConfigJsonObject newItem{item};
            m_metricConfigs.push_back(newItem);
          }
          m_metricConfigsIsSet = true;
        }
      }

      nlohmann::json PathConfigJsonObject::toJson() const {
        nlohmann::json val = nlohmann::json::object();
        if (!getBase().is_null())
          val.update(getBase());
        if (m_nameIsSet)
          val["name"] = m_name;
        if (m_codeIsSet)
          val["code"] = m_code;

        {
          nlohmann::json jsonArray;
          for (auto &item : m_metricConfigs)
            jsonArray.push_back(JsonObjectBase::toJson(item));
          if (jsonArray.size() > 0)
            val["metric-configs"] = jsonArray;
        }
        return val;
      }

      std::string PathConfigJsonObject::getName() const {
        return m_name;
      }

      void PathConfigJsonObject::setName(std::string value) {
        m_name = value;
        m_nameIsSet = true;
      }

      bool PathConfigJsonObject::nameIsSet() const {
        return m_nameIsSet;
      }

      void PathConfigJsonObject::unsetName() {
        m_nameIsSet = false;
      }

      std::string PathConfigJsonObject::getCode() const {
        return m_code;
      }

      void PathConfigJsonObject::setCode(std::string value) {
        m_code = value;
        m_codeIsSet = true;
      }

      bool PathConfigJsonObject::codeIsSet() const {
        return m_codeIsSet;
      }

      const std::vector<MetricConfigJsonObject> &PathConfigJsonObject::getMetricConfigs() const {
        return m_metricConfigs;
      }

      void PathConfigJsonObject::addDataplaneConfigEgressMetricConfigs(MetricConfigJsonObject value) {
        m_metricConfigs.push_back(value);
        m_metricConfigsIsSet = true;
      }

      bool PathConfigJsonObject::metricConfigsIsSet() const {
        return m_metricConfigsIsSet;
      }

      void PathConfigJsonObject::unsetMetricConfigs() {
        m_metricConfigsIsSet = false;
      }
    }// namespace model
  }// namespace service
}// namespace polycube
