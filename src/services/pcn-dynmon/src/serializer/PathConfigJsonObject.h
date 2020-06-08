#pragma once

#include "JsonObjectBase.h"
#include "MetricConfigJsonObject.h"
#include <vector>

namespace polycube {
  namespace service {
    namespace model {

      /**
       *  Path Config
       */
      class PathConfigJsonObject : public JsonObjectBase {
       public:
        PathConfigJsonObject();
        PathConfigJsonObject(const nlohmann::json &json);
        ~PathConfigJsonObject() final = default;
        nlohmann::json toJson() const final;

        /**
         *  Configuration name
         */
        std::string getName() const;
        void setName(std::string value);
        bool nameIsSet() const;
        void unsetName();

        /**
         *  Ingress eBPF source code
         */
        std::string getCode() const;
        void setCode(std::string value);
        bool codeIsSet() const;

        /**
         *  Exported Metric
         */
        const std::vector<MetricConfigJsonObject> &getMetricConfigs() const;
        void addDataplaneConfigEgressMetricConfigs(MetricConfigJsonObject value);
        bool metricConfigsIsSet() const;
        void unsetMetricConfigs();

       private:
        std::string m_name;
        bool m_nameIsSet;
        std::string m_code;
        bool m_codeIsSet;
        std::vector<MetricConfigJsonObject> m_metricConfigs;
        bool m_metricConfigsIsSet;
      };
    }// namespace model
  }// namespace service
}// namespace polycube
