#include "OpenMetricsMetadataJsonObject.h"
#include <regex>

namespace polycube {
  namespace service {
    namespace model {

      OpenMetricsMetadataJsonObject::OpenMetricsMetadataJsonObject() {
        m_helpIsSet = false;
        m_typeIsSet = false;
        m_labelsIsSet = false;
      }

      OpenMetricsMetadataJsonObject::OpenMetricsMetadataJsonObject(const nlohmann::json &val) : JsonObjectBase(val) {
        m_helpIsSet = false;
        m_typeIsSet = false;
        m_labelsIsSet = false;
        if (val.count("help"))
          setHelp(val.at("help").get<std::string>());
        if (val.count("type"))
          setType(string_to_MetricTypeEnum(val.at("type").get<std::string>()));
        if (val.count("labels")) {
          for (auto &item : val["labels"]) {
            LabelJsonObject newItem{item};
            m_labels.push_back(newItem);
          }
          m_labelsIsSet = true;
        }
      }

      nlohmann::json OpenMetricsMetadataJsonObject::toJson() const {
        nlohmann::json val = nlohmann::json::object();
        if (!getBase().is_null())
          val.update(getBase());
        if (m_helpIsSet)
          val["help"] = m_help;
        if (m_typeIsSet)
          val["type"] = MetricTypeEnum_to_string(m_type);

        {
          nlohmann::json jsonArray;
          for (auto &item : m_labels)
            jsonArray.push_back(JsonObjectBase::toJson(item));

          if (jsonArray.size() > 0)
            val["labels"] = jsonArray;
        }
        return val;
      }

      std::string OpenMetricsMetadataJsonObject::getHelp() const {
        return m_help;
      }

      void OpenMetricsMetadataJsonObject::setHelp(std::string value) {
        m_help = value;
        m_helpIsSet = true;
      }

      bool OpenMetricsMetadataJsonObject::helpIsSet() const {
        return m_helpIsSet;
      }

      MetricTypeEnum OpenMetricsMetadataJsonObject::getType() const {
        return m_type;
      }

      void OpenMetricsMetadataJsonObject::setType(MetricTypeEnum value) {
        m_type = value;
        m_typeIsSet = true;
      }

      bool OpenMetricsMetadataJsonObject::typeIsSet() const {
        return m_typeIsSet;
      }

      std::string OpenMetricsMetadataJsonObject::MetricTypeEnum_to_string(const MetricTypeEnum &value) {
        switch (value) {
          case MetricTypeEnum::COUNTER:
            return std::string("counter");
          case MetricTypeEnum::GAUGE:
            return std::string("gauge");
          case MetricTypeEnum::HISTOGRAM:
            return std::string("histogram");
          case MetricTypeEnum::SUMMARY:
            return std::string("summary");
          case MetricTypeEnum::UNTYPED:
            return std::string("untyped");
          default:
            throw std::runtime_error("Bad Metric type");
        }
      }

      MetricTypeEnum OpenMetricsMetadataJsonObject::string_to_MetricTypeEnum(const std::string &str) {
        if (JsonObjectBase::iequals("counter", str))
          return MetricTypeEnum::COUNTER;
        if (JsonObjectBase::iequals("gauge", str))
          return MetricTypeEnum::GAUGE;
        if (JsonObjectBase::iequals("histogram", str))
          return MetricTypeEnum::HISTOGRAM;
        if (JsonObjectBase::iequals("summary", str))
          return MetricTypeEnum::SUMMARY;
        if (JsonObjectBase::iequals("untyped", str))
          return MetricTypeEnum::UNTYPED;
        throw std::runtime_error("Metric type is invalid");
      }
      const std::vector<LabelJsonObject> &OpenMetricsMetadataJsonObject::getLabels() const {
        return m_labels;
      }

      void OpenMetricsMetadataJsonObject::addLabel(LabelJsonObject value) {
        m_labels.push_back(value);
        m_labelsIsSet = true;
      }

      bool OpenMetricsMetadataJsonObject::labelsIsSet() const {
        return m_labelsIsSet;
      }

      void OpenMetricsMetadataJsonObject::unsetLabels() {
        m_labelsIsSet = false;
      }
    }// namespace model
  }// namespace service
}// namespace polycube
