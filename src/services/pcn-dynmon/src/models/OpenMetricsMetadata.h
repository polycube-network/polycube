#pragma once

#include "../base/OpenMetricsMetadataBase.h"
#include "Label.h"

class MetricConfig;

using namespace polycube::service::model;

class OpenMetricsMetadata : public OpenMetricsMetadataBase {
 public:
  explicit OpenMetricsMetadata(MetricConfig &parent);
  OpenMetricsMetadata(MetricConfig &parent, const OpenMetricsMetadataJsonObject &conf);
  ~OpenMetricsMetadata() = default;

  /**
   *  Metric description
   */
  std::string getHelp() override;
  void setHelp(const std::string &);

  /**
   *  Metric type
   */
  MetricTypeEnum getType() override;
  void setType(const MetricTypeEnum &);

  /**
   *  Label attached to the metric
   */
  std::shared_ptr<Label> getLabel(const std::string &name) override;
  std::vector<std::shared_ptr<Label>> getLabelsList() override;
  void addLabel(const std::string &name, const LabelJsonObject &conf) override;
  void addLabelsList(const std::vector<LabelJsonObject> &conf) override;
  void replaceLabel(const std::string &name, const LabelJsonObject &conf) override;
  void delLabel(const std::string &name) override;
  void delLabelsList() override;

 private:
  std::string m_help;
  MetricTypeEnum m_type;
  std::vector<std::shared_ptr<Label>> m_labels;
};
