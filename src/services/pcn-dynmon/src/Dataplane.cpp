#include "Dataplane.h"
#include "Dynmon.h"

Dataplane::Dataplane(Dynmon &parent, const DataplaneJsonObject &conf)
    : DataplaneBase(parent) {
  // Setting the dataplane name
  if (conf.nameIsSet())
    name_ = conf.getName();
  else
    name_ = DEFAULT_NAME;

  // Setting the dataplane code
  if (conf.codeIsSet())
    code_ = conf.getCode();
  else
    code_ = DEFAULT_CODE;

  // Setting the dataplane metrics configuration
  addMetricsList(conf.getMetrics());
}

Dataplane::~Dataplane() {}

std::string Dataplane::getName() {
  return name_;
}

std::string Dataplane::getCode() {
  return code_;
}

std::shared_ptr<DataplaneMetrics> Dataplane::getMetrics(const std::string &name) {
  for (auto &it : metrics_)
    if (it->getName() == name)
      return it;
  throw std::runtime_error("The metric does not exist");
}

std::vector<std::shared_ptr<DataplaneMetrics>> Dataplane::getMetricsList() {
  return metrics_;
}

void Dataplane::addMetrics(const std::string &name, const DataplaneMetricsJsonObject &conf) {
  auto entry = std::make_shared<DataplaneMetrics>(*this, conf);
  if (entry == nullptr)
    throw std::runtime_error("This should not happen");
  for (auto &it : metrics_)
    if (it->getName() == entry->getName())
      throw std::runtime_error("Cannot insert duplicate metrics entry");
  metrics_.push_back(entry);
}

void Dataplane::addMetricsList(const std::vector<DataplaneMetricsJsonObject> &conf) {
  DataplaneBase::addMetricsList(conf);
}

void Dataplane::replaceMetrics(const std::string &name, const DataplaneMetricsJsonObject &conf) {
  DataplaneBase::replaceMetrics(name, conf);
}

void Dataplane::delMetrics(const std::string &name) {
  metrics_.erase(
      std::remove_if(metrics_.begin(), metrics_.end(),
                     [name](const std::shared_ptr<DataplaneMetrics> &entry) {
                       return entry->getName() == name;
                     }),
      metrics_.end());
}

void Dataplane::delMetricsList() {
  metrics_.clear();
}
