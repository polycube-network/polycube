#include "DataplaneMetrics.h"
#include "Dynmon.h"

DataplaneMetrics::DataplaneMetrics(Dataplane &parent, const DataplaneMetricsJsonObject &conf)
    : DataplaneMetricsBase(parent) {
  if (conf.nameIsSet())
    name_ = conf.getName();
  else
    throw std::runtime_error("[Dataplane][Metrics] name not set");
  if (conf.mapNameIsSet())
    mapName_ = conf.getMapName();
  else
    throw std::runtime_error("[Dataplane][Metrics] map-name not set");
  if (conf.openMetricsMetadataIsSet())
    addOpenMetricsMetadata(conf.getOpenMetricsMetadata());
}

DataplaneMetrics::~DataplaneMetrics() {}

std::string DataplaneMetrics::getName() {
  return name_;
}

std::string DataplaneMetrics::getMapName() {
  return mapName_;
}

std::shared_ptr<DataplaneMetricsOpenMetricsMetadata> DataplaneMetrics::getOpenMetricsMetadata() {
  return metadata_;
}

void DataplaneMetrics::addOpenMetricsMetadata(const DataplaneMetricsOpenMetricsMetadataJsonObject &value) {
  metadata_ = std::make_shared<DataplaneMetricsOpenMetricsMetadata>(*this, value);
}

void DataplaneMetrics::replaceOpenMetricsMetadata(const DataplaneMetricsOpenMetricsMetadataJsonObject &conf) {
  DataplaneMetricsBase::replaceOpenMetricsMetadata(conf);
}

void DataplaneMetrics::delOpenMetricsMetadata() {
  metadata_ = nullptr;
}
