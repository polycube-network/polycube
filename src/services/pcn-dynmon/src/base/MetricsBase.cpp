#include "MetricsBase.h"
#include "../Dynmon.h"

MetricsBase::MetricsBase(Dynmon &parent)
    : parent_(parent) {}

void MetricsBase::update(const MetricsJsonObject &conf) {
  if (conf.ingressMetricsIsSet())
    for (auto &i : conf.getIngressMetrics()) {
      auto name = i.getName();
      auto m = getIngressMetric(name);
      m->update(i);
    }
  if (conf.egressMetricsIsSet())
    for (auto &i : conf.getEgressMetrics()) {
      auto name = i.getName();
      auto m = getEgressMetric(name);
      m->update(i);
    }
}

MetricsJsonObject MetricsBase::toJsonObject() {
  MetricsJsonObject conf;
  for (auto &i : getIngressMetricsList())
    conf.addIngressMetric(i->toJsonObject());
  for (auto &i : getEgressMetricsList())
    conf.addEgressMetric(i->toJsonObject());
  return conf;
}

void MetricsBase::addIngressMetricsList(const std::vector<MetricJsonObject> &conf) {
  for (auto &i : conf) {
    std::string name_ = i.getName();
    addIngressMetric(name_, i);
  }
}

void MetricsBase::replaceIngressMetric(const std::string &name, const MetricJsonObject &conf) {
  delIngressMetric(name);
  std::string name_ = conf.getName();
  addIngressMetric(name_, conf);
}

void MetricsBase::delIngressMetricsList() {
  auto elements = getIngressMetricsList();
  for (auto &i : elements) {
    std::string name_ = i->getName();
    delIngressMetric(name_);
  }
}

void MetricsBase::addEgressMetricsList(const std::vector<MetricJsonObject> &conf) {
  for (auto &i : conf) {
    std::string name_ = i.getName();
    addEgressMetric(name_, i);
  }
}

void MetricsBase::replaceEgressMetric(const std::string &name, const MetricJsonObject &conf) {
  delEgressMetric(name);
  std::string name_ = conf.getName();
  addEgressMetric(name_, conf);
}

void MetricsBase::delEgressMetricsList() {
  auto elements = getEgressMetricsList();
  for (auto &i : elements) {
    std::string name_ = i->getName();
    delEgressMetric(name_);
  }
}

std::shared_ptr<spdlog::logger> MetricsBase::logger() {
  return parent_.logger();
}
