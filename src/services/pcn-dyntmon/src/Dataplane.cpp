#include "Dataplane.h"
#include "Dyntmon.h"

Dataplane::Dataplane(Dyntmon &parent, const DataplaneJsonObject &conf)
    : DataplaneBase(parent) {
    logger()->trace("[Dataplane] Dataplane()");
  if (conf.nameIsSet())
    setName(conf.getName());
  else
    setName("Default");
  if (conf.codeIsSet())
    setCode(conf.getCode());
  else
    setCode(dyntmon_code);
  addMetricsList(conf.getMetrics());
}

Dataplane::~Dataplane() {}

std::string Dataplane::getName() {
  logger()->trace("[Dataplane] getName()");
  return name_;
}

void Dataplane::setName(const std::string &value) {
  logger()->trace("[Dataplane] setName()");
  name_ = value;
}

std::string Dataplane::getCode() {
  logger()->trace("[Dataplane] getCode()");
  return code_;
}

void Dataplane::setCode(const std::string &value) {
  logger()->trace("[Dataplane] setCode()");
  code_ = value;
}

std::shared_ptr<DataplaneMetrics> Dataplane::getMetrics(
    const std::string &name) {
  logger()->trace("[Dataplane] getMetrics()");
  for (auto &it : metrics_)
    if (it->getName() == name)
      return it;
  throw std::runtime_error("The metric does not exist");
}

std::vector<std::shared_ptr<DataplaneMetrics>> Dataplane::getMetricsList() {
  logger()->trace("[Dataplane] getMetricsList()");
  return metrics_;
}

void Dataplane::addMetrics(const std::string &name,
                           const DataplaneMetricsJsonObject &conf) {
  logger()->trace("[Dataplane] addMetrics()");
  auto entry = std::make_shared<DataplaneMetrics>(*this, conf);
  if (entry == nullptr)
    throw std::runtime_error("This should not happen");
  for (auto &it : metrics_)
    if (it->getName() == entry->getName())
      throw std::runtime_error("Cannot insert duplicate metrics entry");
  metrics_.push_back(entry);
}

void Dataplane::addMetricsList(
    const std::vector<DataplaneMetricsJsonObject> &conf) {
  logger()->trace("[Dataplane] addMetricsList()");
  for (auto &it : conf)
    addMetrics(it.getName(), it);
}

void Dataplane::replaceMetrics(const std::string &name,
                               const DataplaneMetricsJsonObject &conf) {
  logger()->trace("[Dataplane] replaceMetrics()");
  delMetrics(name);
  addMetrics(conf.getName(), conf);
}

void Dataplane::delMetrics(const std::string &name) {
  logger()->trace("[Dataplane] delMetrics()");
  metrics_.erase(
      std::remove_if(metrics_.begin(), metrics_.end(),
                     [name](const std::shared_ptr<DataplaneMetrics> &entry) {
                       return entry->getName() == name;
                     }),
      metrics_.end());
}

void Dataplane::delMetricsList() {
  logger()->trace("[Dataplane] delMetricsList()");
  metrics_.clear();
}

DataplaneReloadOutputJsonObject Dataplane::reload() {
  logger()->trace("[Dataplane] reload()");
  DataplaneReloadOutputJsonObject output;
  try {
    if (!code_.empty()) {
      parent_.reload(code_);
      logger()->trace(name_ + " dataplane loaded");
      output.setMessage(name_ + " dataplane loaded");
    } else
      throw std::runtime_error("Invalid eBPF code; Default dataplane loaded");
  } catch (std::exception ex) {
    code_ = dyntmon_code;
    parent_.reload(code_);
    logger()->trace("Invalid eBPF code; Default dataplane loaded");
    throw std::runtime_error("Invalid eBPF code; Default dataplane loaded");
  }
  return output;
}
