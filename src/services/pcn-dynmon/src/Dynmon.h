#pragma once

#include "../base/DynmonBase.h"
#include "Dynmon_dp.h"
#include "extractor/MapExtractor.h"
#include "extractor/options/SwapStateConfig.h"
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
   * Method to modify both original and swapped code accordingly to the mapName to be swapped.
   * Eg. if BPF_ARRAY(JOHN_DOE, u64, 1) needs to be swapped, then
   *    - in the original code will be inserted also BPF_ARRAY(JOHN_DOE_1, u64, 1)
   *    - in the swapped code all JOHN_DOE references will become JOHN_DOE_1
   *    - in the swapped code will be inserted also BPF_ARRAY(JOHN_DOE, u64, 1)
   * Those insertions are vital, since when reloading a program the new one must keep
   * the original/swapped map alive, otherwise if it would not declare it the map would
   * be destroyed, thus not able to be read.
   *
   * @param[original_code] the original code to be modified
   * @param[swapped_code] the swapped code to be modified
   * @param[mapName] the name of the map to be swapped
   */
  static void formatCodeToSwap(std::string &original_code, std::string &swapped_code,
                        std::string mapName);

  string toOpenMetrics(std::shared_ptr<MetricConfig> conf,
                       nlohmann::json value);

  SwapStateConfig ingressSwapState;
  SwapStateConfig egressSwapState;
  std::shared_ptr<DataplaneConfig> m_dpConfig;
  std::mutex m_ingressPathMutex;
  std::mutex m_egressPathMutex;
};
