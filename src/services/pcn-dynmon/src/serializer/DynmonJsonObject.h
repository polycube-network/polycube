#pragma once

#include "DataplaneConfigJsonObject.h"
#include "JsonObjectBase.h"
#include "MetricsJsonObject.h"
#include "polycube/services/cube.h"

namespace polycube {
  namespace service {
    namespace model {

      /**
       *  Dynmon
       */
      class DynmonJsonObject : public JsonObjectBase {
       public:
        DynmonJsonObject();
        DynmonJsonObject(const nlohmann::json &json);
        ~DynmonJsonObject() final = default;
        nlohmann::json toJson() const final;

        /**
         *  Name of the dynmon service
         */
        std::string getName() const;
        void setName(std::string value);
        bool nameIsSet() const;

        /**
         *  Dataplane configuration
         */
        DataplaneConfigJsonObject getDataplaneConfig() const;
        void setDataplaneConfig(DataplaneConfigJsonObject value);
        bool dataplaneConfigIsSet() const;
        void unsetDataplaneConfig();

        /**
         *  Collected metrics in JSON format
         */
        MetricsJsonObject getMetrics() const;
        void setMetrics(MetricsJsonObject value);
        bool metricsIsSet() const;
        void unsetMetrics();

        /**
         *  Collected metrics in OpenMetrics Format of both ingress and egress paths
         */
        std::string getOpenMetrics() const;
        void setOpenMetrics(std::string value);
        bool openMetricsIsSet() const;
        void unsetOpenMetrics();

       private:
        std::string m_name;
        bool m_nameIsSet;
        DataplaneConfigJsonObject m_dataplaneConfig;
        bool m_dataplaneConfigIsSet;
        MetricsJsonObject m_metrics;
        bool m_metricsIsSet;
        std::string m_openMetrics;
        bool m_openMetricsIsSet;
      };
    }// namespace model
  }// namespace service
}// namespace polycube
