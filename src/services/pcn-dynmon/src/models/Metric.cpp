#include "Metric.h"
#include "../Dynmon.h"

Metric::Metric(Dynmon &parent) : MetricBase(parent) {}

Metric::Metric(Dynmon &parent, const MetricJsonObject &conf) : MetricBase(parent) {
  if (conf.nameIsSet())
    m_name = conf.getName();
  if (conf.valueIsSet())
    m_value = conf.getValue();
  if (conf.timestampIsSet())
    m_timestamp = conf.getTimestamp();
}

Metric::Metric(Dynmon &parent, const std::string &name, const nlohmann::json &value, const int64_t &timestamp) : MetricBase(parent) {
  m_name = name;
  m_value = value;
  m_timestamp = timestamp;
}

std::string Metric::getName() {
  return m_name;
}

void Metric::setName(const std::string &name) {
  m_name = name;
}

nlohmann::json Metric::getValue() {
  return m_value;
}

void Metric::setValue(const nlohmann::json &value) {
  m_value = value;
}

int64_t Metric::getTimestamp() {
  return m_timestamp;
}

void Metric::setTimestamp(const int64_t &timestamp) {
  m_timestamp = timestamp;
}