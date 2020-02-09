#pragma once

#include "../base/DyntmonBase.h"
#include "Dataplane.h"
#include "Metrics.h"

using namespace polycube::service::model;

class Dyntmon : public DyntmonBase {
 public:
  Dyntmon(const std::string name, const DyntmonJsonObject &conf);
  virtual ~Dyntmon();

  void packet_in(polycube::service::Direction direction,
                 polycube::service::PacketInMetadata &md,
                 const std::vector<uint8_t> &packet) override;

  /// <summary>
  /// Running probe
  /// </summary>
  std::shared_ptr<Dataplane> getDataplane() override;
  void addDataplane(const DataplaneJsonObject &value) override;
  void replaceDataplane(const DataplaneJsonObject &conf) override;
  void delDataplane() override;

  /// <summary>
  /// Collected metrics
  /// </summary>
  std::shared_ptr<Metrics> getMetrics(const std::string &name) override;
  std::vector<std::shared_ptr<Metrics>> getMetricsList() override;
  void addMetrics(const std::string &name, const MetricsJsonObject &conf) override;
  void addMetricsList(const std::vector<MetricsJsonObject> &conf) override;
  void replaceMetrics(const std::string &name, const MetricsJsonObject &conf) override;
  void delMetrics(const std::string &name) override;
  void delMetricsList() override;

  /// <summary>
  /// Collected metrics in OpenMetrics Format
  /// </summary>
  std::string getOpenMetrics() override;

  private:
   std::shared_ptr<Dataplane> dataplane_;
};
