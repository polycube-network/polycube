#pragma once

#include "../base/PathConfigBase.h"
#include "Dynmon_dp.h"
#include "MetricConfig.h"

class DataplaneConfig;

using namespace polycube::service::model;

static const std::string DEFAULT_PATH_NAME = "Default path config";
static const std::string DEFAULT_PATH_CODE = dynmon_code;

class PathConfig : public PathConfigBase {
 public:
  explicit PathConfig(DataplaneConfig &parent);
  PathConfig(DataplaneConfig &parent, const PathConfigJsonObject &conf);
  ~PathConfig() = default;

  /**
   *  Configuration name
   */
  std::string getName() override;
  void setName(const std::string &value) override;

  /**
   *  Ingress eBPF source code
   */
  std::string getCode() override;
  void setCode(const std::string &);

  /**
   *  Exported Metric
   */
  std::shared_ptr<MetricConfig> getMetricConfig(const std::string &name) override;
  std::vector<std::shared_ptr<MetricConfig>> getMetricConfigsList() override;
  void addMetricConfig(const std::string &name, const MetricConfigJsonObject &conf) override;
  void addMetricConfigsList(const std::vector<MetricConfigJsonObject> &conf) override;
  void replaceMetricConfig(const std::string &name, const MetricConfigJsonObject &conf) override;
  void delMetricConfig(const std::string &name) override;
  void delMetricConfigsList() override;

 private:
  std::string m_name;
  std::string m_code;
  std::vector<std::shared_ptr<MetricConfig>> m_metricConfigs;
};
