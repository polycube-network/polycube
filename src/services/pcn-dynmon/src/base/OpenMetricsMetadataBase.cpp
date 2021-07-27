#include "OpenMetricsMetadataBase.h"
#include "../Dynmon.h"

OpenMetricsMetadataBase::OpenMetricsMetadataBase(MetricConfig &parent)
    : parent_(parent) {}

void OpenMetricsMetadataBase::update(const OpenMetricsMetadataJsonObject &conf) {
  if (conf.labelsIsSet())
    for (auto &i : conf.getLabels()) {
      auto name = i.getName();
      auto m = getLabel(name);
      m->update(i);
    }
}

OpenMetricsMetadataJsonObject OpenMetricsMetadataBase::toJsonObject() {
  OpenMetricsMetadataJsonObject conf;
  conf.setHelp(getHelp());
  conf.setType(getType());
  for (auto &i : getLabelsList())
    conf.addLabel(i->toJsonObject());
  return conf;
}

void OpenMetricsMetadataBase::addLabelsList(const std::vector<LabelJsonObject> &conf) {
  for (auto &i : conf) {
    std::string name_ = i.getName();
    addLabel(name_, i);
  }
}

void OpenMetricsMetadataBase::replaceLabel(const std::string &name, const LabelJsonObject &conf) {
  delLabel(name);
  std::string name_ = conf.getName();
  addLabel(name_, conf);
}

void OpenMetricsMetadataBase::delLabelsList() {
  auto elements = getLabelsList();
  for (auto &i : elements) {
    std::string name_ = i->getName();
    delLabel(name_);
  }
}

std::shared_ptr<spdlog::logger> OpenMetricsMetadataBase::logger() {
  return parent_.logger();
}
