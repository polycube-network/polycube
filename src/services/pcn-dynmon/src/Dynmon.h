#pragma once

#include "../base/DynmonBase.h"

#include "Dataplane.h"
#include "Metrics.h"

using namespace polycube::service::model;

class Dynmon : public DynmonBase {
 public:
  Dynmon(const std::string name, const DynmonJsonObject &conf);
  virtual ~Dynmon();

  void packet_in(polycube::service::Direction direction,
                 polycube::service::PacketInMetadata &md,
                 const std::vector<uint8_t> &packet) override;

  /**
   * @returns the current dataplane configuration
   */
  std::shared_ptr<Dataplane> getDataplane() override;

  /**
   * Stores the dataplane configuration
   * 
   * @param[conf] the dataplane configuration
   */
  void addDataplane(const DataplaneJsonObject &conf) override;

  /**
   * Replaces the current dataplane configuration
   * 
   * @param[conf] the new dataplane configuration
   */
  void replaceDataplane(const DataplaneJsonObject &conf) override;

  /**
   * Deletes the stored dataplane configuration
   */
  void delDataplane() override;

  /**
   * Return a metric with the specified name, in the JSON format.
   *
   * @param[name] the name of the metric
   *
   * @throw std::runtime_error Thrown if the `name` metric does not exist
   * 
   * @returns a metric in the JSON format
   */
  std::shared_ptr<Metrics> getMetrics(const std::string &name) override;

  /**
   * @returns a vector which contains all the collected metrics in the JSON format
   */
  std::vector<std::shared_ptr<Metrics>> getMetricsList() override;

  /**
   * @returns a string which contains all the collected metrics in the OpenMetrics format
   */
  std::string getOpenMetrics() override;

  /**
   * No implementation needed
   */
  void addMetrics(const std::string &name, const MetricsJsonObject &conf) override;
  void addMetricsList(const std::vector<MetricsJsonObject> &conf) override;
  void replaceMetrics(const std::string &name, const MetricsJsonObject &conf) override;
  void delMetrics(const std::string &name) override;
  void delMetricsList() override;

 private:
  // This member contains the current dataplane configuration
  std::shared_ptr<Dataplane> dataplane_;
  std::mutex dataplane_mutex_;
};