#include "PathConfig.h"
#include "../Dynmon.h"

PathConfig::PathConfig(DataplaneConfig &parent)
    : PathConfigBase(parent) {
  m_name = DEFAULT_PATH_NAME;
  m_code = DEFAULT_PATH_CODE;
}

PathConfig::PathConfig(DataplaneConfig &parent, const PathConfigJsonObject &conf)
    : PathConfigBase(parent) {
  if (conf.codeIsSet())
    m_code = conf.getCode();
  else
    throw std::runtime_error("Path Config: missing code");
  if (conf.nameIsSet())
    m_name = conf.getName();
  else
    throw std::runtime_error("Path Config: missing configuration name");
  if (conf.metricConfigsIsSet())
    addMetricConfigsList(conf.getMetricConfigs());
}

std::string PathConfig::getName() {
  return m_name;
}

void PathConfig::setName(const std::string &name) {
  m_name = name;
}

std::string PathConfig::getCode() {
  return m_code;
}

void PathConfig::setCode(const std::string &code) {
  m_code = code;
}

std::shared_ptr<MetricConfig> PathConfig::getMetricConfig(const std::string &name) {
  for (auto &config : m_metricConfigs)
    if (config->getName() == name)
      return config;
  throw std::runtime_error("Metric config for \"" + name + "\" not found");
}

std::vector<std::shared_ptr<MetricConfig>> PathConfig::getMetricConfigsList() {
  return m_metricConfigs;
}

void PathConfig::addMetricConfig(const std::string &name, const MetricConfigJsonObject &conf) {
  for (auto &config : m_metricConfigs)
    if (config->getName() == name)
      throw std::runtime_error("Metric config for \"" + name + "\" already exists");
  m_metricConfigs.push_back(std::make_shared<MetricConfig>(*this, conf));
}

void PathConfig::addMetricConfigsList(const std::vector<MetricConfigJsonObject> &conf) {
  PathConfigBase::addMetricConfigsList(conf);
}

void PathConfig::replaceMetricConfig(const std::string &name, const MetricConfigJsonObject &conf) {
  PathConfigBase::replaceMetricConfig(name, conf);
}

void PathConfig::delMetricConfig(const std::string &name) {
  m_metricConfigs.erase(
      std::remove_if(
          m_metricConfigs.begin(), m_metricConfigs.end(),
          [name](const std::shared_ptr<MetricConfig>
                     &entry) { return entry->getName() == name; }),
      m_metricConfigs.end());
}

void PathConfig::delMetricConfigsList() {
  m_metricConfigs.clear();
}
