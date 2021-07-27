#include "DynmonJsonObject.h"
#include <regex>

namespace polycube {
  namespace service {
    namespace model {

      DynmonJsonObject::DynmonJsonObject() {
        m_nameIsSet = false;
        m_dataplaneConfigIsSet = false;
        m_metricsIsSet = true;
        m_openMetricsIsSet = false;
      }

      DynmonJsonObject::DynmonJsonObject(const nlohmann::json &val) : JsonObjectBase(val) {
        m_nameIsSet = false;
        m_dataplaneConfigIsSet = false;
        m_metricsIsSet = false;
        m_openMetricsIsSet = false;
        if (val.count("name"))
          setName(val.at("name").get<std::string>());

        if (val.count("dataplane-config"))
          if (!val["dataplane-config"].is_null()) {
            DataplaneConfigJsonObject newItem{val["dataplane-config"]};
            setDataplaneConfig(newItem);
          }
        if (val.count("metrics"))
          if (!val["metrics"].is_null()) {
            MetricsJsonObject newItem{val["metrics"]};
            setMetrics(newItem);
          }
        if (val.count("open-metrics"))
          setOpenMetrics(val.at("open-metrics").get<std::string>());
      }

      nlohmann::json DynmonJsonObject::toJson() const {
        nlohmann::json val = nlohmann::json::object();
        if (!getBase().is_null())
          val.update(getBase());
        if (m_nameIsSet)
          val["name"] = m_name;
        if (m_dataplaneConfigIsSet)
          val["dataplane-config"] = JsonObjectBase::toJson(m_dataplaneConfig);
        if (m_metricsIsSet)
          val["metrics"] = JsonObjectBase::toJson(m_metrics);
        if (m_openMetricsIsSet)
          val["open-metrics"] = m_openMetrics;
        return val;
      }

      std::string DynmonJsonObject::getName() const {
        return m_name;
      }

      void DynmonJsonObject::setName(std::string value) {
        m_name = value;
        m_nameIsSet = true;
      }

      bool DynmonJsonObject::nameIsSet() const {
        return m_nameIsSet;
      }

      DataplaneConfigJsonObject DynmonJsonObject::getDataplaneConfig() const {
        return m_dataplaneConfig;
      }

      void DynmonJsonObject::setDataplaneConfig(DataplaneConfigJsonObject value) {
        m_dataplaneConfig = value;
        m_dataplaneConfigIsSet = true;
      }

      bool DynmonJsonObject::dataplaneConfigIsSet() const {
        return m_dataplaneConfigIsSet;
      }

      void DynmonJsonObject::unsetDataplaneConfig() {
        m_dataplaneConfigIsSet = false;
      }

      MetricsJsonObject DynmonJsonObject::getMetrics() const {
        return m_metrics;
      }

      void DynmonJsonObject::setMetrics(MetricsJsonObject value) {
        m_metrics = value;
        m_metricsIsSet = true;
      }

      bool DynmonJsonObject::metricsIsSet() const {
        return m_metricsIsSet;
      }

      void DynmonJsonObject::unsetMetrics() {
        m_metricsIsSet = false;
      }

      std::string DynmonJsonObject::getOpenMetrics() const {
        return m_openMetrics;
      }

      void DynmonJsonObject::setOpenMetrics(std::string value) {
        m_openMetrics = value;
        m_openMetricsIsSet = true;
      }

      bool DynmonJsonObject::openMetricsIsSet() const {
        return m_openMetricsIsSet;
      }

      void DynmonJsonObject::unsetOpenMetrics() {
        m_openMetricsIsSet = false;
      }
    }// namespace model
  }// namespace service
}// namespace polycube
