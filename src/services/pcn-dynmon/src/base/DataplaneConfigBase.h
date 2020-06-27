#pragma once

#include "../models/PathConfig.h"
#include "../serializer/DataplaneConfigJsonObject.h"
#include <spdlog/spdlog.h>

using namespace polycube::service::model;

class Dynmon;

class DataplaneConfigBase {
 public:
  DataplaneConfigBase(Dynmon &parent);
  ~DataplaneConfigBase() = default;
  virtual void update(const DataplaneConfigJsonObject &conf);
  virtual DataplaneConfigJsonObject toJsonObject();

  /**
   *  Dataplane configuration for the INGRESS path
   */
  virtual std::shared_ptr<PathConfig> getIngressPathConfig() = 0;
  virtual void addIngressPathConfig(const PathConfigJsonObject &value) = 0;
  virtual void replaceIngressPathConfig(const PathConfigJsonObject &conf);
  virtual void delIngressPathConfig() = 0;

  /**
   *  Dataplane configuration for the EGRESS path
   */
  virtual std::shared_ptr<PathConfig> getEgressPathConfig() = 0;
  virtual void addEgressPathConfig(const PathConfigJsonObject &value) = 0;
  virtual void replaceEgressPathConfig(const PathConfigJsonObject &conf);
  virtual void delEgressPathConfig() = 0;
  std::shared_ptr<spdlog::logger> logger();

 protected:
  Dynmon &parent_;
};
