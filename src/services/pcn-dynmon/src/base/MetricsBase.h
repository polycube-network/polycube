#pragma once

#include "../models/Metric.h"
#include "../serializer/MetricsJsonObject.h"
#include <spdlog/spdlog.h>

using namespace polycube::service::model;

class Dynmon;

class MetricsBase {
 public:
  explicit MetricsBase(Dynmon &parent);
  ~MetricsBase() = default;
  virtual void update(const MetricsJsonObject &conf);
  virtual MetricsJsonObject toJsonObject();

  /**
   *  Collected metrics from the ingress dataplane path
   */
  virtual std::shared_ptr<Metric> getIngressMetric(const std::string &name) = 0;
  virtual std::vector<std::shared_ptr<Metric>> getIngressMetricsList() = 0;
  virtual void addIngressMetric(const std::string &name, const MetricJsonObject &conf) = 0;
  virtual void addIngressMetricsList(const std::vector<MetricJsonObject> &conf);
  virtual void replaceIngressMetric(const std::string &name, const MetricJsonObject &conf);
  virtual void delIngressMetric(const std::string &name) = 0;
  virtual void delIngressMetricsList();

  /**
   *  Collected metrics from the egress dataplane path
   */
  virtual std::shared_ptr<Metric> getEgressMetric(const std::string &name) = 0;
  virtual std::vector<std::shared_ptr<Metric>> getEgressMetricsList() = 0;
  virtual void addEgressMetric(const std::string &name, const MetricJsonObject &conf) = 0;
  virtual void addEgressMetricsList(const std::vector<MetricJsonObject> &conf);
  virtual void replaceEgressMetric(const std::string &name, const MetricJsonObject &conf);
  virtual void delEgressMetric(const std::string &name) = 0;
  virtual void delEgressMetricsList();

  std::shared_ptr<spdlog::logger> logger();

 protected:
  Dynmon &parent_;
};
