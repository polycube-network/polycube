#include "DataplaneMetricsOpenMetricsMetadata.h"
#include "Dyntmon.h"

DataplaneMetricsOpenMetricsMetadata::DataplaneMetricsOpenMetricsMetadata(
    DataplaneMetrics &parent,
    const DataplaneMetricsOpenMetricsMetadataJsonObject &conf)
    : DataplaneMetricsOpenMetricsMetadataBase(parent) {
  logger()->trace("[DataplaneMetricsOpenMetricsMetadata] DataplaneMetricsOpenMetricsMetadata()");
  if (conf.helpIsSet())
    help_ = conf.getHelp();
  if (conf.typeIsSet())
    type_ = conf.getType();
  if (conf.labelIsSet())
    addLabelList(conf.getLabel());
}

DataplaneMetricsOpenMetricsMetadata::~DataplaneMetricsOpenMetricsMetadata() {}

std::string DataplaneMetricsOpenMetricsMetadata::getHelp() {
  logger()->trace("[DataplaneMetricsOpenMetricsMetadata] getHelp()");
  return help_;
}

void DataplaneMetricsOpenMetricsMetadata::setHelp(const std::string &value) {
  logger()->trace("[DataplaneMetricsOpenMetricsMetadata] setHelp()");
  help_ = value;
}

DataplaneMetricsOpenMetricsMetadataTypeEnum
DataplaneMetricsOpenMetricsMetadata::getType() {
  logger()->trace("[DataplaneMetricsOpenMetricsMetadata] getType()");
  return type_;
}

void DataplaneMetricsOpenMetricsMetadata::setType(
    const DataplaneMetricsOpenMetricsMetadataTypeEnum &value) {
  logger()->trace("[DataplaneMetricsOpenMetricsMetadata] setType()");
  type_ = value;
}

std::shared_ptr<DataplaneMetricsOpenMetricsMetadataLabel>
DataplaneMetricsOpenMetricsMetadata::getLabel(const std::string &name) {
  logger()->trace("[DataplaneMetricsOpenMetricsMetadata] getLabel()");
  for (auto &it : labels_)
    if (it->getName() == name)
      return it;
  throw std::runtime_error("The label does not exist");
}

std::vector<std::shared_ptr<DataplaneMetricsOpenMetricsMetadataLabel>>
DataplaneMetricsOpenMetricsMetadata::getLabelList() {
  logger()->trace("[DataplaneMetricsOpenMetricsMetadata] getLabelList()");
  return labels_;
}

void DataplaneMetricsOpenMetricsMetadata::addLabel(
    const std::string &name,
    const DataplaneMetricsOpenMetricsMetadataLabelJsonObject &conf) {
  logger()->trace("[DataplaneMetricsOpenMetricsMetadata] addLabel()");
  auto label =
      std::make_shared<DataplaneMetricsOpenMetricsMetadataLabel>(*this, conf);
  if (label == nullptr)
    throw std::runtime_error("This should not happen");
  for (auto &it : labels_)
    if (it->getName() == label->getName())
      throw std::runtime_error("Cannot insert duplicate label entry");
  labels_.push_back(label);
}

void DataplaneMetricsOpenMetricsMetadata::addLabelList(
    const std::vector<DataplaneMetricsOpenMetricsMetadataLabelJsonObject>
        &conf) {
  logger()->trace("[DataplaneMetricsOpenMetricsMetadata] addLabelList()");
  for (auto &it : conf)
    addLabel(it.getName(), it);
}

void DataplaneMetricsOpenMetricsMetadata::replaceLabel(
    const std::string &name,
    const DataplaneMetricsOpenMetricsMetadataLabelJsonObject &conf) {
  logger()->trace("[DataplaneMetricsOpenMetricsMetadata] replaceLabel()");
  delLabel(name);
  addLabel(conf.getName(), conf);
}

void DataplaneMetricsOpenMetricsMetadata::delLabel(const std::string &name) {
  logger()->trace("[DataplaneMetricsOpenMetricsMetadata] delLabel()");
  labels_.erase(
      std::remove_if(
          labels_.begin(), labels_.end(),
          [name](const std::shared_ptr<DataplaneMetricsOpenMetricsMetadataLabel>
                     &entry) { return entry->getName() == name; }),
      labels_.end());
}

void DataplaneMetricsOpenMetricsMetadata::delLabelList() {
  logger()->trace("[DataplaneMetricsOpenMetricsMetadata] delLabelList()");
  labels_.clear();
}
