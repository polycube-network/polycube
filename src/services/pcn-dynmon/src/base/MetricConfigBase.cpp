#include "MetricConfigBase.h"
#include "../Dynmon.h"

MetricConfigBase::MetricConfigBase(PathConfig &parent)
    : parent_(parent) {}

void MetricConfigBase::update(const MetricConfigJsonObject &conf) {
  if (conf.extractionOptionsIsSet()) {
    auto m = getExtractionOptions();
    m->update(conf.getExtractionOptions());
  }
  if (conf.openMetricsMetadataIsSet()) {
    auto m = getOpenMetricsMetadata();
    m->update(conf.getOpenMetricsMetadata());
  }
}

MetricConfigJsonObject MetricConfigBase::toJsonObject() {
  MetricConfigJsonObject conf;
  conf.setName(getName());
  conf.setMapName(getMapName());
  conf.setExtractionOptions(getExtractionOptions()->toJsonObject());
  auto metadata = getOpenMetricsMetadata();
  if (getOpenMetricsMetadata() != nullptr)
    conf.setOpenMetricsMetadata(metadata->toJsonObject());
  return conf;
}

void MetricConfigBase::replaceExtractionOptions(const ExtractionOptionsJsonObject &conf) {
  delExtractionOptions();
  addExtractionOptions(conf);
}

void MetricConfigBase::replaceOpenMetricsMetadata(const OpenMetricsMetadataJsonObject &conf) {
  delOpenMetricsMetadata();
  addOpenMetricsMetadata(conf);
}

std::shared_ptr<spdlog::logger> MetricConfigBase::logger() {
  return parent_.logger();
}
