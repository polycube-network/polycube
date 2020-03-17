#pragma once

#include "../base/MetricsBase.h"

class Dynmon;

using namespace polycube::service::model;

/**
 * This class represents a metric in the JSON format.
 */
class Metrics : public MetricsBase {
 public:
  Metrics(Dynmon &parent, const MetricsJsonObject &conf);
  Metrics(Dynmon &parent, std::string name, std::string value, const int64_t &timestamp);
  virtual ~Metrics();

  /**
   * @returns the name of the metric
   */
  std::string getName() override;

  /**
   * @returns the value of the metric
   */
  std::string getValue() override;

  /**
   * @returns the timestamp of the metric
   */
  int64_t getTimestamp() override;

private:
  std::string name_;
  std::string value_;
  int64_t timestamp_;
};