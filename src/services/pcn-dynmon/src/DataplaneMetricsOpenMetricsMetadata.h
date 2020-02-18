#pragma once

#include "../base/DataplaneMetricsOpenMetricsMetadataBase.h"

#include "DataplaneMetricsOpenMetricsMetadataLabels.h"

class DataplaneMetrics;

using namespace polycube::service::model;

class DataplaneMetricsOpenMetricsMetadata : public DataplaneMetricsOpenMetricsMetadataBase {
 public:
  DataplaneMetricsOpenMetricsMetadata(DataplaneMetrics &parent, const DataplaneMetricsOpenMetricsMetadataJsonObject &conf);
  virtual ~DataplaneMetricsOpenMetricsMetadata();

  /**
   *  Metric description
   */
  std::string getHelp() override;

  /**
   * Metric type
   */
  DataplaneMetricsOpenMetricsMetadataTypeEnum getType() override;

  /**
   * Label attached to the metric
   */
  std::shared_ptr<DataplaneMetricsOpenMetricsMetadataLabels> getLabels(const std::string &name) override;
  std::vector<std::shared_ptr<DataplaneMetricsOpenMetricsMetadataLabels>> getLabelsList() override;
  void addLabels(const std::string &name, const DataplaneMetricsOpenMetricsMetadataLabelsJsonObject &conf) override;
  void addLabelsList(const std::vector<DataplaneMetricsOpenMetricsMetadataLabelsJsonObject> &conf) override;
  void replaceLabels(const std::string &name, const DataplaneMetricsOpenMetricsMetadataLabelsJsonObject &conf) override;
  void delLabels(const std::string &name) override;
  void delLabelsList() override;

private:
  std::string help_;
  DataplaneMetricsOpenMetricsMetadataTypeEnum type_;
  std::vector<std::shared_ptr<DataplaneMetricsOpenMetricsMetadataLabels>> labels_;
};
