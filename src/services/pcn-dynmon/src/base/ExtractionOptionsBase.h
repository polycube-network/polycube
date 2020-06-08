#pragma once

#include "../serializer/ExtractionOptionsJsonObject.h"
#include <spdlog/spdlog.h>

using namespace polycube::service::model;

class MetricConfig;

class ExtractionOptionsBase {
 public:
  explicit ExtractionOptionsBase(MetricConfig &parent);
  ~ExtractionOptionsBase() = default;
  virtual void update(const ExtractionOptionsJsonObject &conf);
  virtual ExtractionOptionsJsonObject toJsonObject();

  /**
   *  When true, map entries keys are extracted along witht entries values
   */
  virtual bool getSwapOnRead() = 0;

  /**
   *  When true, map entries are deleted after being extracted
   */
  virtual bool getEmptyOnRead() = 0;

  std::shared_ptr<spdlog::logger> logger();

 protected:
  MetricConfig &parent_;
};
