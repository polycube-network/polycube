#include "ExtractionOptionsBase.h"
#include "../Dynmon.h"

ExtractionOptionsBase::ExtractionOptionsBase(MetricConfig &parent)
    : parent_(parent) {}

void ExtractionOptionsBase::update(const ExtractionOptionsJsonObject &conf) {}

ExtractionOptionsJsonObject ExtractionOptionsBase::toJsonObject() {
  ExtractionOptionsJsonObject conf;
  conf.setSwapOnRead(getSwapOnRead());
  conf.setEmptyOnRead(getEmptyOnRead());
  return conf;
}

std::shared_ptr<spdlog::logger> ExtractionOptionsBase::logger() {
  return parent_.logger();
}
