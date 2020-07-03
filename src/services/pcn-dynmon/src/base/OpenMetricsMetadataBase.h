#pragma once

#include "../models/Label.h"
#include "../serializer/OpenMetricsMetadataJsonObject.h"
#include <spdlog/spdlog.h>

using namespace polycube::service::model;

class MetricConfig;

class OpenMetricsMetadataBase {
 public:
  explicit OpenMetricsMetadataBase(MetricConfig &parent);
  ~OpenMetricsMetadataBase() = default;
  virtual void update(const OpenMetricsMetadataJsonObject &conf);
  virtual OpenMetricsMetadataJsonObject toJsonObject();

  /**
   *  Metric description
   */
  virtual std::string getHelp() = 0;

  /**
   *  Metric type
   */
  virtual MetricTypeEnum getType() = 0;

  /**
   *  Label attached to the metric
   */
  virtual std::shared_ptr<Label> getLabel(const std::string &name) = 0;
  virtual std::vector<std::shared_ptr<Label>> getLabelsList() = 0;
  virtual void addLabel(const std::string &name, const LabelJsonObject &conf) = 0;
  virtual void addLabelsList(const std::vector<LabelJsonObject> &conf);
  virtual void replaceLabel(const std::string &name, const LabelJsonObject &conf);
  virtual void delLabel(const std::string &name) = 0;
  virtual void delLabelsList();

  std::shared_ptr<spdlog::logger> logger();

 protected:
  MetricConfig &parent_;
};
