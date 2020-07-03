#include "MetricBase.h"
#include "../Dynmon.h"

MetricBase::MetricBase(Dynmon &parent)
    : parent_(parent) {}

void MetricBase::update(const MetricJsonObject &conf) {}

MetricJsonObject MetricBase::toJsonObject() {
  MetricJsonObject conf;
  conf.setName(getName());
  conf.setValue(getValue());
  conf.setTimestamp(getTimestamp());
  return conf;
}

std::shared_ptr<spdlog::logger> MetricBase::logger() {
  return parent_.logger();
}
