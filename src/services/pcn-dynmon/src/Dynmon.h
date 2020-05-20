#pragma once

#include "../base/DynmonBase.h"
#include "Dynmon_dp.h"
#include "extractor/MapExtractor.h"
#include "models/DataplaneConfig.h"
#include "models/Metric.h"
#include "models/Metrics.h"

using namespace polycube::service::model;
using polycube::service::ProgramType;

class Dynmon : public DynmonBase {
 public:
  Dynmon(const std::string name, const DynmonJsonObject &conf);
  ~Dynmon() override;
  void packet_in(polycube::service::Direction direction,
                 polycube::service::PacketInMetadata &md,
                 const std::vector<uint8_t> &packet) override;

  /**
   *  Dataplane configuration
   */
  std::shared_ptr<DataplaneConfig> getDataplaneConfig() override;
  void setDataplaneConfig(const DataplaneConfigJsonObject &config) override;
  void resetDataplaneConfig() override;

  void setEgressPathConfig(const PathConfigJsonObject &config);
  void resetEgressPathConfig();
  void setIngressPathConfig(const PathConfigJsonObject &config);
  void resetIngressPathConfig();

  /**
   *  Collected metrics in JSON format of both ingress and egress paths
   */
  std::shared_ptr<Metrics> getMetrics() override;

  /**
   *  Collected metric in JSON format from the egress path
   */
  std::shared_ptr<Metric> getEgressMetric(const std::string &name);

  /**
   *  Collected metrics in JSON format from the egress path
   */
  std::vector<std::shared_ptr<Metric>> getEgressMetrics();

  /**
   *  Collected metric in JSON format from the ingress path
   */
  std::shared_ptr<Metric> getIngressMetric(const std::string &name);

  /**
   *  Collected metrics in JSON format from the ingress path
   */
  std::vector<std::shared_ptr<Metric>> getIngressMetrics();

  /**
   *  Collected metrics in OpenMetrics format of both ingress and egress paths
   */
  std::string getOpenMetrics() override;

  /**
   *  Collected metrics in OpenMetrics format in the Egress Path
   */
  std::string getEgressOpenMetrics();

  /**
   *  Collected metrics in OpenMetrics format in the Ingress Path
   */
  std::string getIngressOpenMetrics();

 private:
  string toOpenMetrics(std::shared_ptr<MetricConfig> conf,
                       nlohmann::json value);

  std::shared_ptr<DataplaneConfig> m_dpConfig;
  std::mutex m_ingressPathMutex;
  std::mutex m_egressPathMutex;
};
