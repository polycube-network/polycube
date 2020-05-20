#pragma once

#include "../base/ExtractionOptionsBase.h"

class MetricConfig;

using namespace polycube::service::model;

class ExtractionOptions : public ExtractionOptionsBase {
 public:
  ExtractionOptions(MetricConfig &parent, const ExtractionOptionsJsonObject &conf);
  ~ExtractionOptions() = default;

  /**
   *  When true, map entries are deleted after being extracted
   */
  bool getEmptyOnRead() override;

  /**
   *  When true, the map is swapped with a new one before reading its content to provide thread safety and atomicity on read/write operations
   */
  bool getSwapOnRead() override;

 private:
  bool m_emptyOnRead;
  bool m_swapOnRead;
};
