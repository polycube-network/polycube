#include "DataplaneMetrics.h"
#include "Dyntmon.h"

DataplaneMetrics::DataplaneMetrics(Dataplane &parent,
                                   const DataplaneMetricsJsonObject &conf)
    : DataplaneMetricsBase(parent) {
  logger()->trace("[DataplaneMetrics] DataplaneMetrics()");
  if (conf.nameIsSet())
    name_ = conf.getName();
  if (conf.mapNameIsSet())
    mapName_ = conf.getMapName();
  addOpenMetricsMetadata(conf.getOpenMetricsMetadata());
}

DataplaneMetrics::~DataplaneMetrics() {}

std::string DataplaneMetrics::getName() {
  logger()->trace("[DataplaneMetrics] getName()");
  return name_;
}

std::string DataplaneMetrics::getMapName() {
  logger()->trace("[DataplaneMetrics] getMapName()");
  return mapName_;
}

void DataplaneMetrics::setMapName(const std::string &value) {
  logger()->trace("[DataplaneMetrics] setMapName()");
  mapName_ = value;
}

std::shared_ptr<DataplaneMetricsOpenMetricsMetadata>
DataplaneMetrics::getOpenMetricsMetadata() {
  logger()->trace("[DataplaneMetrics] getOpenMetricsMetadata()");
  return metadata_;
}

void DataplaneMetrics::addOpenMetricsMetadata(
    const DataplaneMetricsOpenMetricsMetadataJsonObject &value) {
  logger()->trace("[DataplaneMetrics] addOpenMetricsMetadata()");
  if (metadata_ == nullptr)
    metadata_ =
        std::make_shared<DataplaneMetricsOpenMetricsMetadata>(*this, value);
}

void DataplaneMetrics::replaceOpenMetricsMetadata(
    const DataplaneMetricsOpenMetricsMetadataJsonObject &conf) {
  logger()->trace("[DataplaneMetrics] replaceOpenMetricsMetadata()");
  delOpenMetricsMetadata();
  addOpenMetricsMetadata(conf);
}

void DataplaneMetrics::delOpenMetricsMetadata() {
  logger()->trace("[DataplaneMetrics] delOpenMetricsMetadata()");
  metadata_ = nullptr;
}
