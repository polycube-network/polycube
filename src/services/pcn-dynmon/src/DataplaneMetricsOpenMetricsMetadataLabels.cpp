#include "DataplaneMetricsOpenMetricsMetadataLabels.h"
#include "Dynmon.h"

DataplaneMetricsOpenMetricsMetadataLabels::DataplaneMetricsOpenMetricsMetadataLabels(DataplaneMetricsOpenMetricsMetadata &parent, const DataplaneMetricsOpenMetricsMetadataLabelsJsonObject &conf)
    : DataplaneMetricsOpenMetricsMetadataLabelsBase(parent) {
  if (conf.nameIsSet())
    name_ = conf.getName();
  else
    throw std::runtime_error("[Dataplane][Metrics][OpenMetricsMetadata][Label] name not set");
  if (conf.valueIsSet())
    value_ = conf.getValue();
  else
    throw std::runtime_error("[Dataplane][Metrics][OpenMetricsMetadata][Label] value not set");
}

DataplaneMetricsOpenMetricsMetadataLabels::~DataplaneMetricsOpenMetricsMetadataLabels() {}

std::string DataplaneMetricsOpenMetricsMetadataLabels::getName() {
  return name_;
}

std::string DataplaneMetricsOpenMetricsMetadataLabels::getValue() {
  return value_;
}
