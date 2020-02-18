#include "Metrics.h"
#include "Dynmon.h"

Metrics::Metrics(Dynmon &parent, const MetricsJsonObject &conf)
    : MetricsBase(parent) {
  if (conf.nameIsSet())
    name_ = conf.getName();
  if (conf.valueIsSet())
    value_ = conf.getValue();
  if (conf.timestampIsSet())
    timestamp_ = conf.getTimestamp();
}

Metrics::Metrics(Dynmon &parent, std::string name, std::string value,
                 const int64_t &timestamp)
    : MetricsBase(parent),
      name_(std::move(name)),
      value_(std::move(value)),
      timestamp_(timestamp) {}

Metrics::~Metrics() {}

std::string Metrics::getName() {
  return name_;
}

std::string Metrics::getValue() {
  return value_;
}

int64_t Metrics::getTimestamp() {
  return timestamp_;
}
