#include "MetricConfig.h"
#include "../Dynmon.h"

MetricConfig::MetricConfig(PathConfig &parent, const MetricConfigJsonObject &conf)
    : MetricConfigBase(parent) {
  if (conf.nameIsSet())
    m_name = conf.getName();
  if (conf.mapNameIsSet())
    m_mapName = conf.getMapName();
  addExtractionOptions(conf.getExtractionOptions());
  if (conf.openMetricsMetadataIsSet())
    addOpenMetricsMetadata(conf.getOpenMetricsMetadata());
  else
    m_openMetricsMetadata = std::shared_ptr<OpenMetricsMetadata>(nullptr);
}

std::string MetricConfig::getName() {
  return m_name;
}

std::string MetricConfig::getMapName() {
  return m_mapName;
}

std::shared_ptr<ExtractionOptions> MetricConfig::getExtractionOptions() {
  return m_extractionOptions;
}

void MetricConfig::addExtractionOptions(const ExtractionOptionsJsonObject &options) {
  if (m_extractionOptions == nullptr)
    m_extractionOptions = std::make_shared<ExtractionOptions>(*this, options);
}

void MetricConfig::replaceExtractionOptions(const ExtractionOptionsJsonObject &conf) {
  MetricConfigBase::replaceExtractionOptions(conf);
}

void MetricConfig::delExtractionOptions() {
  m_extractionOptions = nullptr;
}

std::shared_ptr<OpenMetricsMetadata> MetricConfig::getOpenMetricsMetadata() {
  return m_openMetricsMetadata;
}

void MetricConfig::addOpenMetricsMetadata(const OpenMetricsMetadataJsonObject &metadata) {
  if (m_openMetricsMetadata == nullptr)
    m_openMetricsMetadata = std::make_shared<OpenMetricsMetadata>(*this, metadata);
}

void MetricConfig::replaceOpenMetricsMetadata(const OpenMetricsMetadataJsonObject &conf) {
  MetricConfigBase::replaceOpenMetricsMetadata(conf);
}

void MetricConfig::delOpenMetricsMetadata() {
  m_openMetricsMetadata = nullptr;
}
