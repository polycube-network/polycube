#pragma once

#include "../base/DataplaneMetricsOpenMetricsMetadataLabelBase.h"

class DataplaneMetricsOpenMetricsMetadata;

using namespace polycube::service::model;

class DataplaneMetricsOpenMetricsMetadataLabel : public DataplaneMetricsOpenMetricsMetadataLabelBase {
 public:
  DataplaneMetricsOpenMetricsMetadataLabel(DataplaneMetricsOpenMetricsMetadata &parent, const DataplaneMetricsOpenMetricsMetadataLabelJsonObject &conf);
  virtual ~DataplaneMetricsOpenMetricsMetadataLabel();

  /// <summary>
  /// Name of the label (e.g., &#39;method&#39;)
  /// </summary>
  std::string getName() override;

  /// <summary>
  /// Label value (e.g., &#39;POST&#39;)
  /// </summary>
  std::string getValue() override;
  void setValue(const std::string &value) override;

  private:
   std::string name_;
   std::string value_;
};
