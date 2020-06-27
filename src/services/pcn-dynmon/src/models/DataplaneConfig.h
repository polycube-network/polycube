#pragma once

#include "../base/DataplaneConfigBase.h"
#include "PathConfig.h"

class Dynmon;

using namespace polycube::service::model;

class DataplaneConfig : public DataplaneConfigBase {
 public:
  explicit DataplaneConfig(Dynmon &parent);
  DataplaneConfig(Dynmon &parent, const DataplaneConfigJsonObject &conf);
  ~DataplaneConfig() = default;

  /**
   *  Dataplane configuration for the INGRESS path
   */
  std::shared_ptr<PathConfig> getIngressPathConfig() override;
  void addIngressPathConfig(const PathConfigJsonObject &config) override;
  void replaceIngressPathConfig(const PathConfigJsonObject &conf) override;
  void delIngressPathConfig() override;

  /**
   *  Dataplane configuration for the EGRESS path
   */
  std::shared_ptr<PathConfig> getEgressPathConfig() override;
  void addEgressPathConfig(const PathConfigJsonObject &config) override;
  void replaceEgressPathConfig(const PathConfigJsonObject &conf) override;
  void delEgressPathConfig() override;

 private:
  std::shared_ptr<PathConfig> m_ingressPathConfig;
  std::shared_ptr<PathConfig> m_egressPathConfig;
};
