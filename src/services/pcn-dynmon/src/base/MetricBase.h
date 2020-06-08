#pragma once

#include "../serializer/MetricJsonObject.h"
#include <spdlog/spdlog.h>

using namespace polycube::service::model;

class Dynmon;

class MetricBase {
 public:
  MetricBase(Dynmon &parent);
  ~MetricBase() = default;
  virtual void update(const MetricJsonObject &conf);
  virtual MetricJsonObject toJsonObject();

  /**
   *  Name of the metric (e.g, number of HTTP requests)
   */
  virtual std::string getName() = 0;

  /**
   *  Value of the metric
   */
  virtual nlohmann::json getValue() = 0;

  /**
   *  Timestamp
   */
  virtual int64_t getTimestamp() = 0;

  std::shared_ptr<spdlog::logger> logger();

 protected:
  Dynmon &parent_;
};
