#pragma once

#include "../models/MetricConfig.h"
#include "../serializer/PathConfigJsonObject.h"
#include <spdlog/spdlog.h>

using namespace polycube::service::model;

class DataplaneConfig;

class PathConfigBase {
 public:
  PathConfigBase(DataplaneConfig &parent);
  ~PathConfigBase() = default;
  virtual void update(const PathConfigJsonObject &conf);
  virtual PathConfigJsonObject toJsonObject();

  /**
   *  Configuration name
   */
  virtual std::string getName() = 0;
  virtual void setName(const std::string &value) = 0;

  /**
   *  Ingress eBPF source code
   */
  virtual std::string getCode() = 0;
  virtual void setCode(const std::string &value) = 0;

  /**
   *  Exported Metric Configs
   */
  virtual std::shared_ptr<MetricConfig> getMetricConfig(const std::string &name) = 0;
  virtual std::vector<std::shared_ptr<MetricConfig>> getMetricConfigsList() = 0;
  virtual void addMetricConfig(const std::string &name, const MetricConfigJsonObject &conf) = 0;
  virtual void addMetricConfigsList(const std::vector<MetricConfigJsonObject> &conf);
  virtual void replaceMetricConfig(const std::string &name, const MetricConfigJsonObject &conf);
  virtual void delMetricConfig(const std::string &name) = 0;
  virtual void delMetricConfigsList();

  std::shared_ptr<spdlog::logger> logger();

 protected:
  DataplaneConfig &parent_;
};
