#include "LabelBase.h"
#include "../Dynmon.h"

LabelBase::LabelBase(OpenMetricsMetadata &parent)
    : parent_(parent) {}

void LabelBase::update(const LabelJsonObject &conf) {}

LabelJsonObject LabelBase::toJsonObject() {
  LabelJsonObject conf;
  conf.setName(getName());
  conf.setValue(getValue());
  return conf;
}

std::shared_ptr<spdlog::logger> LabelBase::logger() {
  return parent_.logger();
}
