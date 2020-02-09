#include "DataplaneMetricsOpenMetricsMetadataLabel.h"
#include "Dyntmon.h"

DataplaneMetricsOpenMetricsMetadataLabel::
    DataplaneMetricsOpenMetricsMetadataLabel(
        DataplaneMetricsOpenMetricsMetadata &parent,
        const DataplaneMetricsOpenMetricsMetadataLabelJsonObject &conf)
    : DataplaneMetricsOpenMetricsMetadataLabelBase(parent) {
  logger()->trace("[DataplaneMetricsOpenMetricsMetadataLabel] DataplaneMetricsOpenMetricsMetadataLabel()");
  if (conf.nameIsSet())
    name_ = conf.getName();
  if (conf.valueIsSet())
    value_ = conf.getValue();
}

DataplaneMetricsOpenMetricsMetadataLabel::~DataplaneMetricsOpenMetricsMetadataLabel() {}

std::string DataplaneMetricsOpenMetricsMetadataLabel::getName() {
  logger()->trace("[DataplaneMetricsOpenMetricsMetadataLabel] getName()");
  return name_;
}

std::string DataplaneMetricsOpenMetricsMetadataLabel::getValue() {
  logger()->trace("[DataplaneMetricsOpenMetricsMetadataLabel] getValue()");
  return value_;
}

void DataplaneMetricsOpenMetricsMetadataLabel::setValue(const std::string &value) {
  logger()->trace("[DataplaneMetricsOpenMetricsMetadataLabel] setValue()");
  value_ = value;
}
