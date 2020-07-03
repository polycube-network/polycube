#include "MetricsJsonObject.h"
#include <regex>

namespace polycube {
  namespace service {
    namespace model {

      MetricsJsonObject::MetricsJsonObject() {
        m_ingressMetricsIsSet = true;
        m_egressMetricsIsSet = true;
      }

      MetricsJsonObject::MetricsJsonObject(const nlohmann::json &val) : JsonObjectBase(val) {
        m_ingressMetricsIsSet = true;
        m_egressMetricsIsSet = true;
        if (val.count("ingress-metrics")) {
          for (auto &item : val["ingress-metrics"]) {
            MetricJsonObject newItem{item};
            m_ingressMetrics.push_back(newItem);
          }
          m_ingressMetricsIsSet = true;
        }

        if (val.count("egress-metrics")) {
          for (auto &item : val["egress-metrics"]) {
            MetricJsonObject newItem{item};
            m_egressMetrics.push_back(newItem);
          }
          m_egressMetricsIsSet = true;
        }
      }

      nlohmann::json MetricsJsonObject::toJson() const {
        nlohmann::json val = nlohmann::json::object();
        if (!getBase().is_null())
          val.update(getBase());

        {
          nlohmann::json jsonArray;
          for (auto &item : m_ingressMetrics)
            jsonArray.push_back(JsonObjectBase::toJson(item));
          if (jsonArray.size() > 0)
            val["ingress-metrics"] = jsonArray;
          else
            val["ingress-metrics"] = nlohmann::json::array();
        }

        {
          nlohmann::json jsonArray;
          for (auto &item : m_egressMetrics)
            jsonArray.push_back(JsonObjectBase::toJson(item));
          if (jsonArray.size() > 0)
            val["egress-metrics"] = jsonArray;
          else
            val["egress-metrics"] = nlohmann::json::array();
        }
        return val;
      }

      const std::vector<MetricJsonObject> &MetricsJsonObject::getIngressMetrics() const {
        return m_ingressMetrics;
      }

      void MetricsJsonObject::addIngressMetric(MetricJsonObject value) {
        m_ingressMetrics.push_back(value);
        m_ingressMetricsIsSet = true;
      }

      bool MetricsJsonObject::ingressMetricsIsSet() const {
        return m_ingressMetricsIsSet;
      }

      void MetricsJsonObject::unsetIngressMetrics() {
        m_ingressMetricsIsSet = false;
      }

      const std::vector<MetricJsonObject> &MetricsJsonObject::getEgressMetrics() const {
        return m_egressMetrics;
      }

      void MetricsJsonObject::addEgressMetric(MetricJsonObject value) {
        m_egressMetrics.push_back(value);
        m_egressMetricsIsSet = true;
      }

      bool MetricsJsonObject::egressMetricsIsSet() const {
        return m_egressMetricsIsSet;
      }

      void MetricsJsonObject::unsetEgressMetrics() {
        m_egressMetricsIsSet = false;
      }
    }// namespace model
  }// namespace service
}// namespace polycube
