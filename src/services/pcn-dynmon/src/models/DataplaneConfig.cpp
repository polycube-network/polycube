#include "DataplaneConfig.h"
#include "../Dynmon.h"

DataplaneConfig::DataplaneConfig(Dynmon &parent)
    : DataplaneConfigBase(parent) {
  m_egressPathConfig = std::make_shared<PathConfig>(*this);
  m_ingressPathConfig = std::make_shared<PathConfig>(*this);
}

DataplaneConfig::DataplaneConfig(Dynmon &parent, const DataplaneConfigJsonObject &conf)
    : DataplaneConfigBase(parent) {
  if (conf.ingressPathIsSet())
    addIngressPathConfig(conf.getIngressPath());
  if (conf.egressPathIsSet())
    addEgressPathConfig(conf.getEgressPath());
}

std::shared_ptr<PathConfig> DataplaneConfig::getIngressPathConfig() {
  logger()->debug("[DataplaneConfig] getIngressPathConfig()");

  if (m_ingressPathConfig == nullptr)
    throw std::runtime_error("Ingress Path Config is null");
  return m_ingressPathConfig;
}

void DataplaneConfig::addIngressPathConfig(const PathConfigJsonObject &config) {
  if (m_ingressPathConfig == nullptr)
    m_ingressPathConfig = std::make_shared<PathConfig>(*this, config);
}

void DataplaneConfig::replaceIngressPathConfig(const PathConfigJsonObject &conf) {
  DataplaneConfigBase::replaceIngressPathConfig(conf);
}

void DataplaneConfig::delIngressPathConfig() {
  m_ingressPathConfig = nullptr;
}

std::shared_ptr<PathConfig> DataplaneConfig::getEgressPathConfig() {
  logger()->debug("[DataplaneConfig] getEgressPathConfig()");

  if (m_egressPathConfig == nullptr)
    throw std::runtime_error("Egress Path Config is null");
  return m_egressPathConfig;
}

void DataplaneConfig::addEgressPathConfig(const PathConfigJsonObject &config) {
  if (m_egressPathConfig == nullptr)
    m_egressPathConfig = std::make_shared<PathConfig>(*this, config);
}

void DataplaneConfig::replaceEgressPathConfig(const PathConfigJsonObject &conf) {
  DataplaneConfigBase::replaceEgressPathConfig(conf);
}

void DataplaneConfig::delEgressPathConfig() {
  m_egressPathConfig = nullptr;
}
