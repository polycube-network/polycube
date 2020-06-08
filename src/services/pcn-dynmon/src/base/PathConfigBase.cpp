#include "PathConfigBase.h"
#include "../Dynmon.h"

PathConfigBase::PathConfigBase(DataplaneConfig &parent)
    : parent_(parent) {}

void PathConfigBase::update(const PathConfigJsonObject &conf) {
  if (conf.nameIsSet())
    setName(conf.getName());
  if (conf.codeIsSet())
    setCode(conf.getCode());
  if (conf.metricConfigsIsSet())
    for (auto &i : conf.getMetricConfigs()) {
      auto name = i.getName();
      auto m = getMetricConfig(name);
      m->update(i);
    }
}

PathConfigJsonObject PathConfigBase::toJsonObject() {
  PathConfigJsonObject conf;
  conf.setName(getName());
  conf.setCode(getCode());
  for (auto &i : getMetricConfigsList())
    conf.addDataplaneConfigEgressMetricConfigs(i->toJsonObject());
  return conf;
}

void PathConfigBase::addMetricConfigsList(const std::vector<MetricConfigJsonObject> &conf) {
  for (auto &i : conf) {
    std::string name_ = i.getName();
    addMetricConfig(name_, i);
  }
}

void PathConfigBase::replaceMetricConfig(const std::string &name, const MetricConfigJsonObject &conf) {
  delMetricConfig(name);
  std::string name_ = conf.getName();
  addMetricConfig(name_, conf);
}

void PathConfigBase::delMetricConfigsList() {
  auto elements = getMetricConfigsList();
  for (auto &i : elements) {
    std::string name_ = i->getName();
    delMetricConfig(name_);
  }
}

std::shared_ptr<spdlog::logger> PathConfigBase::logger() {
  return parent_.logger();
}
