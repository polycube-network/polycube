#pragma once

#include "../base/DataplaneBase.h"

#include "DataplaneMetrics.h"
#include "Dynmon_dp.h"

#define DEFAULT_NAME "Default dataplane"
#define DEFAULT_CODE dynmon_code;

class Dynmon;

using namespace polycube::service::model;

/**
 * This class represents the configuration of the service's dataplane.
 * The class stores the name of the dataplane, its eBPF code and a vector
 * metric configurations used to configure the exported metrics.
 */
class Dataplane : public DataplaneBase {
 public:
  Dataplane(Dynmon &parent, const DataplaneJsonObject &conf);
  virtual ~Dataplane();

  /**
   * @returns the eBPF program name
   */
  std::string getName() override;

  /**
   * @returns eBPF source code
   */
  std::string getCode() override;

  /**
   * Returns a metric's configuration.
   *
   * @param[name] the name of the metric
   *
   * @throw std::runtime_error Thrown if the `name` metric does not exist
   * @returns the metric's configuration
   */
  std::shared_ptr<DataplaneMetrics> getMetrics(const std::string &name) override;

  /**
   * @returns all the metric configurations
   */
  std::vector<std::shared_ptr<DataplaneMetrics>> getMetricsList() override;

  /**
   * Adds a metric configuration
   *
   * @param[name] the name of the metric
   * @param[conf] the metric configuration
   */
  void addMetrics(const std::string &name,
                  const DataplaneMetricsJsonObject &conf) override;

  /**
   * Adds a set of metric configurations
   *
   * @param[name] the name of the metric
   * @param[conf] the metric configurations
   */
  void addMetricsList(const std::vector<DataplaneMetricsJsonObject> &conf) override;

  /**
   * Replaces the current set of metric configurations
   *
   * @param[name] the name of the metric
   * @param[conf] the metric configuration
   */
  void replaceMetrics(const std::string &name, const DataplaneMetricsJsonObject &conf) override;

  /**
   * Deletes a metric configuration
   */
  void delMetrics(const std::string &name) override;

  /**
   * Deletes al the metric configurations
   */
  void delMetricsList() override;

 private:
  std::string name_;
  std::string code_;
  std::vector<std::shared_ptr<DataplaneMetrics>> metrics_;
};
