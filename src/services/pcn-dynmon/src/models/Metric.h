#pragma once

#include "../base/MetricBase.h"

class Metrics;

using namespace polycube::service::model;

class Metric : public MetricBase {
 public:
  explicit Metric(Dynmon &parent);
  Metric(Dynmon &parent, const MetricJsonObject &conf);
  Metric(Dynmon &parent, const std::string &name, const nlohmann::json &value, const int64_t &timestamp);
  ~Metric() = default;

  /**
   *  Name of the metric (e.g, number of HTTP requests)
   */
  std::string getName() override;
  void setName(const std::string &);

  /**
   *  Value of the metric
   */
  nlohmann::json getValue() override;
  void setValue(const nlohmann::json &);

  /**
   *  Timestamp
   */
  int64_t getTimestamp() override;
  void setTimestamp(const int64_t &);

 private:
  std::string m_name;
  nlohmann::json m_value;
  int64_t m_timestamp;
};
