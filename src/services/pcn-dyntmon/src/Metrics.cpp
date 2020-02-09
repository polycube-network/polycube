#include "Metrics.h"
#include "Dyntmon.h"

Metrics::Metrics(Dyntmon &parent, const MetricsJsonObject &conf)
    : MetricsBase(parent) {
  logger()->trace("[Metrics] Metrics()");
  if (conf.nameIsSet())
    name_ = conf.getName();
  if (conf.valueIsSet())
    value_ = conf.getValue();
  if (conf.timestampIsSet())
    timestamp_ = conf.getTimestamp();
}

Metrics::Metrics(Dyntmon &parent, std::string name, std::string value,
                 const int64_t &timestamp)
    : MetricsBase(parent),
      name_(std::move(name)),
      value_(std::move(value)),
      timestamp_(timestamp) {}

Metrics::~Metrics() {}

std::string Metrics::getName() {
  logger()->trace("[Metrics] getName()");
  return name_;
}

std::string Metrics::getValue() {
  logger()->trace("[Metrics] getValue()");
  return value_;
}

int64_t Metrics::getTimestamp() {
  logger()->trace("[Metrics] getTimestamp()");
  return timestamp_;
}
