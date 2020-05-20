#pragma once

#include "../models/DataplaneConfig.h"
#include "../models/Metrics.h"
#include "../serializer/DynmonJsonObject.h"
#include "polycube/services/fifo_map.hpp"
#include "polycube/services/transparent_cube.h"
#include "polycube/services/utils.h"
#include <spdlog/spdlog.h>

using namespace polycube::service::model;

class DynmonBase : public virtual polycube::service::TransparentCube {
 public:
  explicit DynmonBase(const std::string name);
  ~DynmonBase() override = default;
  virtual void update(const DynmonJsonObject &conf);
  virtual DynmonJsonObject toJsonObject();

  /**
   *  Dataplane configuration
   */
  virtual std::shared_ptr<DataplaneConfig> getDataplaneConfig() = 0;
  virtual void setDataplaneConfig(const DataplaneConfigJsonObject &value) = 0;
  virtual void resetDataplaneConfig() = 0;

  /**
   *  Collected metrics in JSON format
   */
  virtual std::shared_ptr<Metrics> getMetrics() = 0;

  /**
   *  Collected metrics in OpenMetrics Format of both ingress and egress paths
   */
  virtual std::string getOpenMetrics() = 0;

 protected:
  std::string m_name;
};
