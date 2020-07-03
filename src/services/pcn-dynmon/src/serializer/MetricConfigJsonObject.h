#pragma once

#include "ExtractionOptionsJsonObject.h"
#include "JsonObjectBase.h"
#include "OpenMetricsMetadataJsonObject.h"

namespace polycube {
  namespace service {
    namespace model {

      /**
       *  Metric Config
       */
      class MetricConfigJsonObject : public JsonObjectBase {
       public:
        MetricConfigJsonObject();
        MetricConfigJsonObject(const nlohmann::json &json);
        ~MetricConfigJsonObject() final = default;
        nlohmann::json toJson() const final;

        /**
         *  Name of the metric (e.g., number of HTTP requests)
         */
        std::string getName() const;
        void setName(std::string value);
        bool nameIsSet() const;

        /**
         *  Corresponding eBPF map name
         */
        std::string getMapName() const;
        void setMapName(std::string value);
        bool mapNameIsSet() const;

        /**
         *  Metric extraction options
         */
        ExtractionOptionsJsonObject getExtractionOptions() const;
        void setExtractionOptions(ExtractionOptionsJsonObject value);
        bool extractionOptionsIsSet() const;
        void unsetExtractionOptions();

        /**
         *  Open-Metrics metadata
         */
        OpenMetricsMetadataJsonObject getOpenMetricsMetadata() const;
        void setOpenMetricsMetadata(OpenMetricsMetadataJsonObject value);
        bool openMetricsMetadataIsSet() const;
        void unsetOpenMetricsMetadata();

       private:
        std::string m_name;
        bool m_nameIsSet;
        std::string m_mapName;
        bool m_mapNameIsSet;
        ExtractionOptionsJsonObject m_extractionOptions;
        bool m_extractionOptionsIsSet;
        OpenMetricsMetadataJsonObject m_openMetricsMetadata;
        bool m_openMetricsMetadataIsSet;
      };
    }// namespace model
  }// namespace service
}// namespace polycube
