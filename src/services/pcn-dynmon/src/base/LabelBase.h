#pragma once

#include "../serializer/LabelJsonObject.h"
#include <spdlog/spdlog.h>

using namespace polycube::service::model;

class OpenMetricsMetadata;

class LabelBase {
 public:
  LabelBase(OpenMetricsMetadata &parent);
  ~LabelBase() = default;
  virtual void update(const LabelJsonObject &conf);
  virtual LabelJsonObject toJsonObject();

  /**
   *  Name of the label (e.g., &#39;method&#39;)
   */
  virtual std::string getName() = 0;

  /**
   *  Label value (e.g., &#39;POST&#39;)
   */
  virtual std::string getValue() = 0;

  std::shared_ptr<spdlog::logger> logger();

 protected:
  OpenMetricsMetadata &parent_;
};
