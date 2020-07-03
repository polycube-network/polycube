#pragma once

#include "JsonObjectBase.h"
#include "LabelJsonObject.h"
#include <vector>

namespace polycube {
  namespace service {
    namespace model {

      enum class MetricTypeEnum {
        COUNTER,
        GAUGE,
        HISTOGRAM,
        SUMMARY,
        UNTYPED
      };

      /**
       *  OpenMetrics Metadata
       */
      class OpenMetricsMetadataJsonObject : public JsonObjectBase {
       public:
        OpenMetricsMetadataJsonObject();
        OpenMetricsMetadataJsonObject(const nlohmann::json &json);
        ~OpenMetricsMetadataJsonObject() final = default;
        nlohmann::json toJson() const final;

        /**
         *  Metric description
         */
        std::string getHelp() const;
        void setHelp(std::string value);
        bool helpIsSet() const;

        /**
         *  Metric type
         */
        MetricTypeEnum getType() const;
        void setType(MetricTypeEnum value);
        bool typeIsSet() const;
        static std::string MetricTypeEnum_to_string(const MetricTypeEnum &value);
        static MetricTypeEnum string_to_MetricTypeEnum(const std::string &str);

        /**
         *  Label attached to the metric
         */
        const std::vector<LabelJsonObject> &getLabels() const;
        void addLabel(LabelJsonObject value);
        bool labelsIsSet() const;
        void unsetLabels();

       private:
        std::string m_help;
        bool m_helpIsSet;
        MetricTypeEnum m_type;
        bool m_typeIsSet;
        std::vector<LabelJsonObject> m_labels;
        bool m_labelsIsSet;
      };
    }// namespace model
  }// namespace service
}// namespace polycube
