#pragma once

#include "../base/DataplaneMetricsOpenMetricsMetadataLabelsBase.h"

class DataplaneMetricsOpenMetricsMetadata;

using namespace polycube::service::model;

class DataplaneMetricsOpenMetricsMetadataLabels : public DataplaneMetricsOpenMetricsMetadataLabelsBase {
 public:
  DataplaneMetricsOpenMetricsMetadataLabels(DataplaneMetricsOpenMetricsMetadata &parent, const DataplaneMetricsOpenMetricsMetadataLabelsJsonObject &conf);
  virtual ~DataplaneMetricsOpenMetricsMetadataLabels();

  /**
   * Name of the label (e.g., "method")
   */
  std::string getName() override;

  /**
   * Label value (e.g., "POST")
   */
  std::string getValue() override;

private:
  std::string name_;
  std::string value_;
};
