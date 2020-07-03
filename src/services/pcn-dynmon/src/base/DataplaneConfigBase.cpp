#include "DataplaneConfigBase.h"
#include "../Dynmon.h"

DataplaneConfigBase::DataplaneConfigBase(Dynmon &parent)
    : parent_(parent) {}

void DataplaneConfigBase::update(const DataplaneConfigJsonObject &conf) {
  if (conf.ingressPathIsSet()) {
    auto m = getIngressPathConfig();
    m->update(conf.getIngressPath());
  }
  if (conf.egressPathIsSet()) {
    auto m = getEgressPathConfig();
    m->update(conf.getEgressPath());
  }
}

DataplaneConfigJsonObject DataplaneConfigBase::toJsonObject() {
  DataplaneConfigJsonObject conf;
  auto ingressPathConfig = getIngressPathConfig();
  if (ingressPathConfig != nullptr)
    conf.setIngressPath(ingressPathConfig->toJsonObject());
  auto egressPathConfig = getEgressPathConfig();
  if (egressPathConfig != nullptr)
    conf.setEgressPath(egressPathConfig->toJsonObject());
  return conf;
}

void DataplaneConfigBase::replaceIngressPathConfig(const PathConfigJsonObject &conf) {
  delIngressPathConfig();
  addIngressPathConfig(conf);
}

void DataplaneConfigBase::replaceEgressPathConfig(const PathConfigJsonObject &conf) {
  delEgressPathConfig();
  addEgressPathConfig(conf);
}

std::shared_ptr<spdlog::logger> DataplaneConfigBase::logger() {
  return parent_.logger();
}
