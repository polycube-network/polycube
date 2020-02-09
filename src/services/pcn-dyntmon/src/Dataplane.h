#pragma once

#include "../base/DataplaneBase.h"
#include "DataplaneMetrics.h"
#include "Dyntmon_dp.h"

class Dyntmon;

using namespace polycube::service::model;

class Dataplane : public DataplaneBase {
 public:
  Dataplane(Dyntmon &parent, const DataplaneJsonObject &conf);
  virtual ~Dataplane();

  /// <summary>
  /// eBPF program name
  /// </summary>
  std::string getName() override;
  void setName(const std::string &value) override;

  /// <summary>
  /// eBPF source code
  /// </summary>
  std::string getCode() override;
  void setCode(const std::string &value) override;

  /// <summary>
  /// Exported Metric
  /// </summary>
  std::shared_ptr<DataplaneMetrics> getMetrics(const std::string &name) override;

  /// <summary>
  /// Exported Metrics
  /// </summary>
  std::vector<std::shared_ptr<DataplaneMetrics>> getMetricsList() override;
  void addMetrics(const std::string &name, const DataplaneMetricsJsonObject &conf) override;
  void addMetricsList(const std::vector<DataplaneMetricsJsonObject> &conf) override;
  void replaceMetrics(const std::string &name, const DataplaneMetricsJsonObject &conf) override;
  void delMetrics(const std::string &name) override;
  void delMetricsList() override;

  DataplaneReloadOutputJsonObject reload() override;

  private:
  std::string name_;
  std::string code_;
  std::vector<std::shared_ptr<DataplaneMetrics>> metrics_;
};
