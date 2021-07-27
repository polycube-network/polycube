#include "Label.h"
#include "../Dynmon.h"
#include <utility>

Label::Label(OpenMetricsMetadata &parent)
    : LabelBase(parent) {}

Label::Label(OpenMetricsMetadata &parent, const LabelJsonObject &conf)
    : LabelBase(parent) {
  if (conf.nameIsSet())
    m_name = conf.getName();
  if (conf.valueIsSet())
    m_value = conf.getValue();
}

Label::Label(OpenMetricsMetadata &parent, std::string name, std::string value)
    : LabelBase(parent), m_name(std::move(name)), m_value(std::move(value)) {}

std::string Label::getName() {
  return m_name;
}

void Label::setName(const std::string &name) {
  m_name = name;
}

std::string Label::getValue() {
  return m_value;
}

void Label::setValue(const std::string &value) {
  m_value = value;
}
