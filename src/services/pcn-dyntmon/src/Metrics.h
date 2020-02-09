#pragma once

#include "../base/MetricsBase.h"

class Dyntmon;

using namespace polycube::service::model;

class Metrics : public MetricsBase {
 public:
  Metrics(Dyntmon &parent, const MetricsJsonObject &conf);
  Metrics(Dyntmon &parent, std::string name, std::string value, const int64_t &timestamp);
  virtual ~Metrics();

  /// <summary>
  /// Name of the metric (e.g, number of HTTP requests)
  /// </summary>
  std::string getName() override;

  /// <summary>
  /// Value of the metric
  /// </summary>
  std::string getValue() override;

  /// <summary>
  /// Timestamp
  /// </summary>
  int64_t getTimestamp() override;

  private:
   std::string name_;
   std::string value_;
   int64_t timestamp_;
};
