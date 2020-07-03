#pragma once

#include "../models/ExtractionOptions.h"
#include "../models/OpenMetricsMetadata.h"
#include "../serializer/MetricConfigJsonObject.h"
#include <spdlog/spdlog.h>

using namespace polycube::service::model;

class PathConfig;

class MetricConfigBase {
 public:
  MetricConfigBase(PathConfig &parent);
  ~MetricConfigBase() = default;
  virtual void update(const MetricConfigJsonObject &conf);
  virtual MetricConfigJsonObject toJsonObject();

  /**
   *  Name of the metric (e.g., number of HTTP requests)
   */
  virtual std::string getName() = 0;

  /**
   *  Corresponding eBPF map name
   */
  virtual std::string getMapName() = 0;

  /**
   *  Metric extraction options
   */
  virtual std::shared_ptr<ExtractionOptions> getExtractionOptions() = 0;
  virtual void addExtractionOptions(const ExtractionOptionsJsonObject &value) = 0;
  virtual void replaceExtractionOptions(const ExtractionOptionsJsonObject &conf);
  virtual void delExtractionOptions() = 0;

  /**
   *  Open-Metrics metadata
   */
  virtual std::shared_ptr<OpenMetricsMetadata> getOpenMetricsMetadata() = 0;
  virtual void addOpenMetricsMetadata(const OpenMetricsMetadataJsonObject &value) = 0;
  virtual void replaceOpenMetricsMetadata(const OpenMetricsMetadataJsonObject &conf);
  virtual void delOpenMetricsMetadata() = 0;

  std::shared_ptr<spdlog::logger> logger();

 protected:
  PathConfig &parent_;
};
