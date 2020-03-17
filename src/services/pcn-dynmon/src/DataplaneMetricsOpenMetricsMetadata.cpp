#include "DataplaneMetricsOpenMetricsMetadata.h"
#include "Dynmon.h"

DataplaneMetricsOpenMetricsMetadata::DataplaneMetricsOpenMetricsMetadata(DataplaneMetrics &parent, const DataplaneMetricsOpenMetricsMetadataJsonObject &conf)
    : DataplaneMetricsOpenMetricsMetadataBase(parent) {
  if (conf.helpIsSet())
    help_ = conf.getHelp();
  else
    throw std::runtime_error("[Dataplane][Metrics][OpenMetricsMetadata] help not set");

  if (conf.typeIsSet())
    type_ = conf.getType();
  else
    throw std::runtime_error("[Dataplane][Metrics][OpenMetricsMetadata] type not set");

  if (conf.labelsIsSet())
    addLabelsList(conf.getLabels());
}


DataplaneMetricsOpenMetricsMetadata::~DataplaneMetricsOpenMetricsMetadata() {}

std::string DataplaneMetricsOpenMetricsMetadata::getHelp() {
  return help_;
}

DataplaneMetricsOpenMetricsMetadataTypeEnum DataplaneMetricsOpenMetricsMetadata::getType() {
  return type_;
}

std::shared_ptr<DataplaneMetricsOpenMetricsMetadataLabels> DataplaneMetricsOpenMetricsMetadata::getLabels(const std::string &name) {
  for (auto &it : labels_)
    if (it->getName() == name)
      return it;
  throw std::runtime_error("Label " + name + " does not exist");
}

std::vector<std::shared_ptr<DataplaneMetricsOpenMetricsMetadataLabels>> DataplaneMetricsOpenMetricsMetadata::getLabelsList() {
  return labels_;
}

void DataplaneMetricsOpenMetricsMetadata::addLabels(const std::string &name, const DataplaneMetricsOpenMetricsMetadataLabelsJsonObject &conf) {
  auto label = std::make_shared<DataplaneMetricsOpenMetricsMetadataLabels>(*this, conf);
  if (label == nullptr)
    throw std::runtime_error("This should not happen");
  for (auto &it : labels_)
    if (it->getName() == label->getName())
      throw std::runtime_error("Cannot insert duplicate label entry");
  labels_.push_back(label);
}

void DataplaneMetricsOpenMetricsMetadata::addLabelsList(const std::vector<DataplaneMetricsOpenMetricsMetadataLabelsJsonObject> &conf) {
  DataplaneMetricsOpenMetricsMetadataBase::addLabelsList(conf);
}

void DataplaneMetricsOpenMetricsMetadata::replaceLabels(const std::string &name, const DataplaneMetricsOpenMetricsMetadataLabelsJsonObject &conf) {
  DataplaneMetricsOpenMetricsMetadataBase::replaceLabels(name, conf);
}

void DataplaneMetricsOpenMetricsMetadata::delLabels(const std::string &name) {
  labels_.erase(
      std::remove_if(
          labels_.begin(), labels_.end(),
          [name](const std::shared_ptr<DataplaneMetricsOpenMetricsMetadataLabels>
                 &entry) { return entry->getName() == name; }),
      labels_.end());
}

void DataplaneMetricsOpenMetricsMetadata::delLabelsList() {
  labels_.clear();
}


