#pragma once

#include "JsonObjectBase.h"
#include "MetricJsonObject.h"
#include <vector>

namespace polycube {
  namespace service {
    namespace model {

      /**
       *  Metrics
       */
      class MetricsJsonObject : public JsonObjectBase {
       public:
        MetricsJsonObject();
        MetricsJsonObject(const nlohmann::json &json);
        ~MetricsJsonObject() final = default;
        nlohmann::json toJson() const final;

        /**
         *  Collected metrics from the ingress dataplane path
         */
        const std::vector<MetricJsonObject> &getIngressMetrics() const;
        void addIngressMetric(MetricJsonObject value);
        bool ingressMetricsIsSet() const;
        void unsetIngressMetrics();

        /**
         *  Collected metrics from the egress dataplane path
         */
        const std::vector<MetricJsonObject> &getEgressMetrics() const;
        void addEgressMetric(MetricJsonObject value);
        bool egressMetricsIsSet() const;
        void unsetEgressMetrics();

       private:
        std::vector<MetricJsonObject> m_ingressMetrics{};
        bool m_ingressMetricsIsSet;
        std::vector<MetricJsonObject> m_egressMetrics{};
        bool m_egressMetricsIsSet;
      };
    }// namespace model
  }// namespace service
}// namespace polycube
