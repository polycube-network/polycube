#pragma once

#include "../base/DataplaneMetricsBase.h"

#include "DataplaneMetricsOpenMetricsMetadata.h"

class Dataplane;

using namespace polycube::service::model;

class DataplaneMetrics : public DataplaneMetricsBase {
 public:
  DataplaneMetrics(Dataplane &parent, const DataplaneMetricsJsonObject &conf);
  virtual ~DataplaneMetrics();

  /**
   * Returns the name of the metric (e.g., number of HTTP requests)
   */
  std::string getName() override;

  /**
   * Returns the eBPF map name
   */
  std::string getMapName() override;

  /**
   * Returns the OpenMetrics metadata(s) of the metric
   */
  std::shared_ptr<DataplaneMetricsOpenMetricsMetadata> getOpenMetricsMetadata() override;
  void addOpenMetricsMetadata(const DataplaneMetricsOpenMetricsMetadataJsonObject &value) override;
  void replaceOpenMetricsMetadata(const DataplaneMetricsOpenMetricsMetadataJsonObject &conf) override;
  void delOpenMetricsMetadata() override;

private:
  std::string name_;
  std::string mapName_;
  std::shared_ptr<DataplaneMetricsOpenMetricsMetadata> metadata_;
};
