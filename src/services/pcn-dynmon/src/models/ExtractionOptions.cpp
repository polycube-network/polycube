#include "ExtractionOptions.h"
#include "../Dynmon.h"

ExtractionOptions::ExtractionOptions(MetricConfig &parent, const ExtractionOptionsJsonObject &conf)
    : ExtractionOptionsBase(parent) {
  if (conf.emptyOnReadIsSet())
    m_emptyOnRead = conf.getEmptyOnRead();
  if (conf.swapOnReadIsSet())
    m_swapOnRead = conf.getSwapOnRead();
}

bool ExtractionOptions::getEmptyOnRead() {
  return m_emptyOnRead;
}

bool ExtractionOptions::getSwapOnRead() {
  return m_swapOnRead;
}
