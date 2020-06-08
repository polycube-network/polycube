#pragma once

#include "../base/MetricConfigBase.h"
#include "ExtractionOptions.h"
#include "OpenMetricsMetadata.h"

class PathConfig;

using namespace polycube::service::model;

class MetricConfig : public MetricConfigBase {
 public:
  MetricConfig(PathConfig &parent, const MetricConfigJsonObject &conf);
  ~MetricConfig() = default;

  /**
   *  Name of the metric (e.g., number of HTTP requests)
   */
  std::string getName() override;

  /**
   *  Corresponding eBPF map name
   */
  std::string getMapName() override;

  /**
   *  Metric extraction options
   */
  std::shared_ptr<ExtractionOptions> getExtractionOptions() override;
  void addExtractionOptions(const ExtractionOptionsJsonObject &options) override;
  void replaceExtractionOptions(const ExtractionOptionsJsonObject &conf) override;
  void delExtractionOptions() override;

  /**
   *  Open-Metrics metadata
   */
  std::shared_ptr<OpenMetricsMetadata> getOpenMetricsMetadata() override;
  void addOpenMetricsMetadata(const OpenMetricsMetadataJsonObject &metadata) override;
  void replaceOpenMetricsMetadata(const OpenMetricsMetadataJsonObject &conf) override;
  void delOpenMetricsMetadata() override;

 private:
  std::string m_name;
  std::string m_mapName;
  std::shared_ptr<ExtractionOptions> m_extractionOptions;
  std::shared_ptr<OpenMetricsMetadata> m_openMetricsMetadata;
};
