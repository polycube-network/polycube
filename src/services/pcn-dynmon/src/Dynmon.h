#pragma once

#include "../base/DynmonBase.h"
#include "Dynmon_dp.h"
#include "extractor/MapExtractor.h"
#include "models/DataplaneConfig.h"
#include "models/Metric.h"
#include "models/Metrics.h"
#include "swap/SwapCompiler.h"

using namespace polycube::service::model;
using polycube::service::ProgramType;

/**
 * Dynmon implementation class.
 *
 * Every Dynmon class has 2 mutexes, one for the egress path and one for the ingress one.
 * These two mutexes are public, since some operation needs to be protected already
 * in the DynmonApiImpl (like the trigger of a read), because there could be scenarios
 * in which even though the required operations may conceptually trigger the same events
 * (eg. both getIngressMetric(name) and getIngressMetrics() trigger the IngressRead event)
 * they need to be protected a priori, since they could possibly be related and call each other
 * (eg. getIngressMetrics() calls n-times getIngressMetric(name))
 */
class Dynmon : public DynmonBase {
 public:
  Dynmon(const std::string& name, const DynmonJsonObject &conf);
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
   *  Collected metrics in OpenMetrics format in the Egress Path.
   *
   *  UNUSED: left here for future endpoints development
   */
  std::string getEgressOpenMetrics();

  /**
   *  Collected metrics in OpenMetrics format in the Ingress Path
   *
   *  UNUSED: left here for future endpoints development
   */
  std::string getIngressOpenMetrics();

  /**
   *  Method to notify dynmon that a user wants to read one or more egress metrics.
   *  This way, if needed, the code is swapped before reading
   */
  void triggerReadEgress();

  /**
   *  Method to notify dynmon that a user wants to read one or more ingress metrics.
   *  This way, if needed, the code is swapped before reading
   */
  void triggerReadIngress();

 private:
  /**
   * Function to parse a metric into OpenMetric format
   *
   * @param conf  the metric configuration
   * @param value the metric extracted value
   * @return      the metric in OpenMetric format
   */
  string toOpenMetrics(const std::shared_ptr<MetricConfig>& conf,
                       nlohmann::json value);

  /**
   * Private function to retrieve a metric
   *
   * @param name                the metric name
   * @param mapName             the map-name specified in the metric
   * @param type                the program type
   * @param extractionOptions   the extraction options of that metric
   * @return                    the metric extracted
   */
  std::shared_ptr<Metric> do_get_metric(const std::string& name, std::string mapName, ProgramType type,
                                        const std::shared_ptr<ExtractionOptions>& extractionOptions);

  /**
   * Private function to retrieve a metric in OpenMetric format
   *
   * @param config  the entire metric configuration
   * @param type    the program type
   * @return        the metric extracted
   */
  std::string do_get_open_metric(const shared_ptr<MetricConfig>& config, ProgramType type);

  /* The Swap state configuration for ingress path*/
  SwapStateConfig ingressSwapState;
  /* The Swap state configuration for egress path*/
  SwapStateConfig egressSwapState;
  std::shared_ptr<DataplaneConfig> m_dpConfig;
  std::mutex m_ingressPathMutex;
  std::mutex m_egressPathMutex;
};
