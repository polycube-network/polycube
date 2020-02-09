#pragma once

#include "../base/DataplaneMetricsOpenMetricsMetadataBase.h"
#include "DataplaneMetricsOpenMetricsMetadataLabel.h"

class DataplaneMetrics;

using namespace polycube::service::model;

class DataplaneMetricsOpenMetricsMetadata
    : public DataplaneMetricsOpenMetricsMetadataBase {
 public:
  DataplaneMetricsOpenMetricsMetadata(
      DataplaneMetrics &parent,
      const DataplaneMetricsOpenMetricsMetadataJsonObject &conf);
  virtual ~DataplaneMetricsOpenMetricsMetadata();

  /// <summary>
  /// Metric description
  /// </summary>
  std::string getHelp() override;
  void setHelp(const std::string &value) override;

  /// <summary>
  /// Metric type
  /// </summary>
  DataplaneMetricsOpenMetricsMetadataTypeEnum getType() override;
  void setType(
      const DataplaneMetricsOpenMetricsMetadataTypeEnum &value) override;

  /// <summary>
  /// Label attached to the metric
  /// </summary>
  std::shared_ptr<DataplaneMetricsOpenMetricsMetadataLabel> getLabel(
      const std::string &name) override;

  /// <summary>
  /// Labels attached to the metric
  /// </summary>
  std::vector<std::shared_ptr<DataplaneMetricsOpenMetricsMetadataLabel>>
  getLabelList() override;
  void addLabel(
      const std::string &name,
      const DataplaneMetricsOpenMetricsMetadataLabelJsonObject &conf) override;
  void addLabelList(
      const std::vector<DataplaneMetricsOpenMetricsMetadataLabelJsonObject>
          &conf) override;
  void replaceLabel(
      const std::string &name,
      const DataplaneMetricsOpenMetricsMetadataLabelJsonObject &conf) override;
  void delLabel(const std::string &name) override;
  void delLabelList() override;

 private:
  std::string help_;
  DataplaneMetricsOpenMetricsMetadataTypeEnum type_;
  std::vector<std::shared_ptr<DataplaneMetricsOpenMetricsMetadataLabel>> labels_;
};
