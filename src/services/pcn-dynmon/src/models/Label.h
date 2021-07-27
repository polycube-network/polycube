#pragma once

#include "../base/LabelBase.h"

class OpenMetricsMetadata;

using namespace polycube::service::model;

class Label : public LabelBase {
 public:
  explicit Label(OpenMetricsMetadata &parent);
  Label(OpenMetricsMetadata &parent, const LabelJsonObject &conf);
  Label(OpenMetricsMetadata &parent, std::string name, std::string value);
  ~Label() = default;

  /**
   *  Name of the label (e.g., "method")
   */
  std::string getName() override;
  void setName(const std::string &);

  /**
   *  Label value (e.g., "POST")
   */
  std::string getValue() override;
  void setValue(const std::string &);

 private:
  std::string m_name;
  std::string m_value;
};
