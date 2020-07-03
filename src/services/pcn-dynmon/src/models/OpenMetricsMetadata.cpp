#include "OpenMetricsMetadata.h"

#include "../Dynmon.h"
#include <utility>

OpenMetricsMetadata::OpenMetricsMetadata(MetricConfig &parent)
    : OpenMetricsMetadataBase(parent) {}

OpenMetricsMetadata::OpenMetricsMetadata(MetricConfig &parent,
                                         const OpenMetricsMetadataJsonObject &conf)
    : OpenMetricsMetadataBase(parent) {
  if (conf.helpIsSet())
    m_help = conf.getHelp();
  if (conf.typeIsSet())
    m_type = conf.getType();
  if (conf.labelsIsSet())
    addLabelsList(conf.getLabels());
}

std::string OpenMetricsMetadata::getHelp() {
  return m_help;
}

void OpenMetricsMetadata::setHelp(const std::string &help) {
  m_help = help;
}

MetricTypeEnum OpenMetricsMetadata::getType() {
  return m_type;
}

void OpenMetricsMetadata::setType(const MetricTypeEnum &type) {
  m_type = type;
}

std::shared_ptr<Label> OpenMetricsMetadata::getLabel(const std::string &name) {
  for (auto &label : m_labels)
    if (label->getName() == name)
      return label;
  throw std::runtime_error("Unable to find label \"" + name + "\"");
}

std::vector<std::shared_ptr<Label>> OpenMetricsMetadata::getLabelsList() {
  return m_labels;
}

void OpenMetricsMetadata::addLabel(const std::string &name, const LabelJsonObject &conf) {
  for (auto &label : m_labels)
    if (label->getName() == name)
      throw std::runtime_error("Label \"" + name + "\" already present");
  m_labels.push_back(std::make_shared<Label>(*this, conf));
}

void OpenMetricsMetadata::addLabelsList(const std::vector<LabelJsonObject> &conf) {
  OpenMetricsMetadataBase::addLabelsList(conf);
}

void OpenMetricsMetadata::replaceLabel(const std::string &name, const LabelJsonObject &conf) {
  OpenMetricsMetadataBase::replaceLabel(name, conf);
}

void OpenMetricsMetadata::delLabel(const std::string &name) {
  m_labels.erase(
      std::remove_if(
          m_labels.begin(), m_labels.end(),
          [name](const std::shared_ptr<Label>
                     &entry) { return entry->getName() == name; }),
      m_labels.end());
}

void OpenMetricsMetadata::delLabelsList() {
  m_labels.clear();
}