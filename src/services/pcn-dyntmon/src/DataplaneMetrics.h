#pragma once

#include "../base/DataplaneMetricsBase.h"
#include "DataplaneMetricsOpenMetricsMetadata.h"

class Dataplane;

using namespace polycube::service::model;

class DataplaneMetrics : public DataplaneMetricsBase {
 public:
  DataplaneMetrics(Dataplane &parent, const DataplaneMetricsJsonObject &conf);
  virtual ~DataplaneMetrics();

  /// <summary>
  /// Name of the metric (e.g., number of HTTP requests)
  /// </summary>
  std::string getName() override;

  /// <summary>
  /// Corrisponding eBPF map name
  /// </summary>
  std::string getMapName() override;
  void setMapName(const std::string &value) override;

  /// <summary>
  /// Open-Metrics metadata
  /// </summary>
  std::shared_ptr<DataplaneMetricsOpenMetricsMetadata> getOpenMetricsMetadata() override;
  void addOpenMetricsMetadata(const DataplaneMetricsOpenMetricsMetadataJsonObject &value) override;
  void replaceOpenMetricsMetadata(const DataplaneMetricsOpenMetricsMetadataJsonObject &conf) override;
  void delOpenMetricsMetadata() override;

  private:
   std::string name_;
   std::string mapName_;
   std::shared_ptr<DataplaneMetricsOpenMetricsMetadata> metadata_;
};
