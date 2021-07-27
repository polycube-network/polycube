#include "DynmonBase.h"

DynmonBase::DynmonBase(const std::string name) : m_name(name) {
  logger()->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [Dynmon] [%n] [%l] %v");
}

void DynmonBase::update(const DynmonJsonObject &conf) {
  set_conf(conf.getBase());
  if (conf.dataplaneConfigIsSet()) {
    setDataplaneConfig(conf.getDataplaneConfig());
  }
}

DynmonJsonObject DynmonBase::toJsonObject() {
  DynmonJsonObject conf;
  conf.setBase(to_json());
  conf.setName(getName());
  auto dpConfig = getDataplaneConfig();
  if (dpConfig != nullptr)
    conf.setDataplaneConfig(dpConfig->toJsonObject());
  conf.setMetrics(getMetrics()->toJsonObject());
  return conf;
}
