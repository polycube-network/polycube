#include "Metrics.h"
#include "../Dynmon.h"

Metrics::Metrics(Dynmon &parent) : MetricsBase(parent) {}

Metrics::Metrics(Dynmon &parent, const MetricsJsonObject &conf)
    : MetricsBase(parent) {
  addIngressMetricsList(conf.getIngressMetrics());
  addEgressMetricsList(conf.getEgressMetrics());
}

std::shared_ptr<Metric> Metrics::getIngressMetric(const std::string &name) {
  for (auto &metric : m_ingressMetrics)
    if (metric->getName() == name)
      return metric;
  throw std::runtime_error("Unable to find ingress metric \"" + name + "\"");
}

std::vector<std::shared_ptr<Metric>> Metrics::getIngressMetricsList() {
  return m_ingressMetrics;
}

void Metrics::addIngressMetric(const std::string &name, const MetricJsonObject &conf) {
  for (auto &metric : m_ingressMetrics)
    if (metric->getName() == name)
      throw std::runtime_error("Ingress metric \"" + name + "\" already exists");
  m_ingressMetrics.push_back(std::make_shared<Metric>(parent_, conf));
}

void Metrics::addIngressMetricUnsafe(std::shared_ptr<Metric> metric) {
  m_ingressMetrics.push_back(metric);
}

void Metrics::addIngressMetricsList(const std::vector<MetricJsonObject> &conf) {
  MetricsBase::addIngressMetricsList(conf);
}

void Metrics::replaceIngressMetric(const std::string &name, const MetricJsonObject &conf) {
  MetricsBase::replaceIngressMetric(name, conf);
}

void Metrics::delIngressMetric(const std::string &name) {
  m_ingressMetrics.erase(
      std::remove_if(
          m_ingressMetrics.begin(), m_ingressMetrics.end(),
          [name](const std::shared_ptr<Metric>
                     &entry) { return entry->getName() == name; }),
      m_ingressMetrics.end());
}

void Metrics::delIngressMetricsList() {
  m_ingressMetrics.clear();
}

std::shared_ptr<Metric> Metrics::getEgressMetric(const std::string &name) {
  for (auto &metric : m_egressMetrics)
    if (metric->getName() == name)
      return metric;
  throw std::runtime_error("Unable to find ingress metric \"" + name + "\"");
}

std::vector<std::shared_ptr<Metric>> Metrics::getEgressMetricsList() {
  return m_egressMetrics;
}

void Metrics::addEgressMetric(const std::string &name, const MetricJsonObject &conf) {
  for (auto &metric : m_egressMetrics)
    if (metric->getName() == name)
      throw std::runtime_error("Egress metric \"" + name + "\" already exists");
  m_egressMetrics.push_back(std::make_shared<Metric>(parent_, conf));
}

void Metrics::addEgressMetricUnsafe(std::shared_ptr<Metric> metric) {
  m_egressMetrics.push_back(metric);
}

void Metrics::addEgressMetricsList(const std::vector<MetricJsonObject> &conf) {
  MetricsBase::addEgressMetricsList(conf);
}

void Metrics::replaceEgressMetric(const std::string &name, const MetricJsonObject &conf) {
  MetricsBase::replaceEgressMetric(name, conf);
}

void Metrics::delEgressMetric(const std::string &name) {
  m_egressMetrics.erase(
      std::remove_if(
          m_egressMetrics.begin(), m_egressMetrics.end(),
          [name](const std::shared_ptr<Metric>
                     &entry) { return entry->getName() == name; }),
      m_egressMetrics.end());
}

void Metrics::delEgressMetricsList() {
  m_egressMetrics.clear();
}
