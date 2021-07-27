#include "MetricConfigJsonObject.h"
#include <regex>

namespace polycube {
  namespace service {
    namespace model {

      MetricConfigJsonObject::MetricConfigJsonObject() {
        m_nameIsSet = false;
        m_mapNameIsSet = false;
        m_extractionOptionsIsSet = false;
        m_openMetricsMetadataIsSet = false;
      }

      MetricConfigJsonObject::MetricConfigJsonObject(const nlohmann::json &val) : JsonObjectBase(val) {
        m_nameIsSet = false;
        m_mapNameIsSet = false;
        m_extractionOptionsIsSet = false;
        m_openMetricsMetadataIsSet = false;
        if (val.count("name"))
          setName(val.at("name").get<std::string>());
        if (val.count("map-name"))
          setMapName(val.at("map-name").get<std::string>());
        if (val.count("extraction-options"))
          if (!val["extraction-options"].is_null()) {
            ExtractionOptionsJsonObject newItem{val["extraction-options"]};
            setExtractionOptions(newItem);
          }
        if (val.count("open-metrics-metadata"))
          if (!val["open-metrics-metadata"].is_null()) {
            OpenMetricsMetadataJsonObject newItem{val["open-metrics-metadata"]};
            setOpenMetricsMetadata(newItem);
          }
      }

      nlohmann::json MetricConfigJsonObject::toJson() const {
        nlohmann::json val = nlohmann::json::object();
        if (!getBase().is_null())
          val.update(getBase());
        if (m_nameIsSet)
          val["name"] = m_name;
        if (m_mapNameIsSet)
          val["map-name"] = m_mapName;
        if (m_extractionOptionsIsSet)
          val["extraction-options"] = JsonObjectBase::toJson(m_extractionOptions);
        if (m_openMetricsMetadataIsSet)
          val["open-metrics-metadata"] = JsonObjectBase::toJson(m_openMetricsMetadata);
        return val;
      }

      std::string MetricConfigJsonObject::getName() const {
        return m_name;
      }

      void MetricConfigJsonObject::setName(std::string value) {
        m_name = value;
        m_nameIsSet = true;
      }

      bool MetricConfigJsonObject::nameIsSet() const {
        return m_nameIsSet;
      }

      std::string MetricConfigJsonObject::getMapName() const {
        return m_mapName;
      }

      void MetricConfigJsonObject::setMapName(std::string value) {
        m_mapName = value;
        m_mapNameIsSet = true;
      }

      bool MetricConfigJsonObject::mapNameIsSet() const {
        return m_mapNameIsSet;
      }

      ExtractionOptionsJsonObject MetricConfigJsonObject::getExtractionOptions() const {
        return m_extractionOptions;
      }

      void MetricConfigJsonObject::setExtractionOptions(ExtractionOptionsJsonObject value) {
        m_extractionOptions = value;
        m_extractionOptionsIsSet = true;
      }

      bool MetricConfigJsonObject::extractionOptionsIsSet() const {
        return m_extractionOptionsIsSet;
      }

      void MetricConfigJsonObject::unsetExtractionOptions() {
        m_extractionOptionsIsSet = false;
      }

      OpenMetricsMetadataJsonObject MetricConfigJsonObject::getOpenMetricsMetadata() const {
        return m_openMetricsMetadata;
      }

      void MetricConfigJsonObject::setOpenMetricsMetadata(OpenMetricsMetadataJsonObject value) {
        m_openMetricsMetadata = value;
        m_openMetricsMetadataIsSet = true;
      }

      bool MetricConfigJsonObject::openMetricsMetadataIsSet() const {
        return m_openMetricsMetadataIsSet;
      }

      void MetricConfigJsonObject::unsetOpenMetricsMetadata() {
        m_openMetricsMetadataIsSet = false;
      }
    }// namespace model
  }// namespace service
}// namespace polycube
