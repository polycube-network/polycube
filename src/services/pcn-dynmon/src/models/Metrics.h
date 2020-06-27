#pragma once

#include "../base/MetricsBase.h"
#include "Metric.h"

class Dynmon;

using namespace polycube::service::model;

class Metrics : public MetricsBase {
 public:
  explicit Metrics(Dynmon &parent);
  Metrics(Dynmon &parent, const MetricsJsonObject &conf);
  ~Metrics() = default;

  /**
   *  Collected metrics from the ingress dataplane path
   */
  std::shared_ptr<Metric> getIngressMetric(const std::string &name) override;
  std::vector<std::shared_ptr<Metric>> getIngressMetricsList() override;
  void addIngressMetric(const std::string &name, const MetricJsonObject &conf) override;
  void addIngressMetricsList(const std::vector<MetricJsonObject> &conf) override;
  void replaceIngressMetric(const std::string &name, const MetricJsonObject &conf) override;
  void delIngressMetric(const std::string &name) override;
  void delIngressMetricsList() override;

  void addIngressMetricUnsafe(std::shared_ptr<Metric>);

  /**
   *  Collected metrics from the egress dataplane path
   */
  std::shared_ptr<Metric> getEgressMetric(const std::string &name) override;
  std::vector<std::shared_ptr<Metric>> getEgressMetricsList() override;
  void addEgressMetric(const std::string &name, const MetricJsonObject &conf) override;
  void addEgressMetricsList(const std::vector<MetricJsonObject> &conf) override;
  void replaceEgressMetric(const std::string &name, const MetricJsonObject &conf) override;
  void delEgressMetric(const std::string &name) override;
  void delEgressMetricsList() override;

  void addEgressMetricUnsafe(std::shared_ptr<Metric>);

 private:
  std::vector<std::shared_ptr<Metric>> m_ingressMetrics{};
  std::vector<std::shared_ptr<Metric>> m_egressMetrics{};
};
