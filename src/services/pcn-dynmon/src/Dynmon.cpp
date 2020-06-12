#include "Dynmon.h"

#include "Utils.h"

Dynmon::Dynmon(const std::string& name, const DynmonJsonObject &conf)
    : TransparentCube(conf.getBase(), {dynmon_code}, {dynmon_code}),
      DynmonBase(name) {
  logger()->info("Creating Dynmon instance");
  m_dpConfig = std::make_shared<DataplaneConfig>(*this);
  if (conf.dataplaneConfigIsSet())
    setDataplaneConfig(conf.getDataplaneConfig());
}

Dynmon::~Dynmon() {
  logger()->info("Destroying Dynmon instance");
}

void Dynmon::packet_in(polycube::service::Direction direction,
                       polycube::service::PacketInMetadata &md,
                       const std::vector<uint8_t> &packet) {
  logger()->debug("Packet received");
}

std::shared_ptr<DataplaneConfig> Dynmon::getDataplaneConfig() {
  logger()->debug("[Dynmon] getDataplaneConfig()");
  return m_dpConfig;
}

void Dynmon::setDataplaneConfig(const DataplaneConfigJsonObject &config) {
  logger()->debug("[Dynmon] setDataplaneConfig(config)");
  if (config.egressPathIsSet())
    setEgressPathConfig(config.getEgressPath());

  if (config.ingressPathIsSet())
    setIngressPathConfig(config.getIngressPath());
}

void Dynmon::resetDataplaneConfig() {
  logger()->debug("[Dynmon] resetDataplaneConfig()");
  resetEgressPathConfig();
  resetIngressPathConfig();
}

void Dynmon::setEgressPathConfig(const PathConfigJsonObject &config) {
  logger()->debug("[Dynmon] setEgressPathConfig(config)");
  std::lock_guard<std::mutex> lock_eg(m_egressPathMutex);
  try {
    std::string original_code = config.getCode();
    std::string swapped_code = original_code;
    for(auto &mc : config.getMetricConfigs()) {
      if(mc.getExtractionOptions().getSwapOnRead()) {
        SwapConfig::formatCodeToSwap(original_code, swapped_code, mc.getMapName());
        egressSwapState.enableSwap();
      }
    }
    if(egressSwapState.isSwapEnabled()) {
      logger()->debug("[Dynmon] Swap enabled for EGRESS program");
      egressSwapState.setCodes(original_code, swapped_code);
      //Load the original tuned code
      reload(original_code, 0, ProgramType::EGRESS);
    } else {
      //Load the real config code since no swap
      reload(config.getCode(), 0, ProgramType::EGRESS);
    }
    m_dpConfig->replaceEgressPathConfig(config);
  } catch (std::exception &ex) {
    logger()->error("ERROR injecting EGRESS path code: {0}", ex.what());
    logger()->info("Restoring default EGRESS path configuration");
    m_dpConfig->replaceEgressPathConfig(PathConfig(*m_dpConfig).toJsonObject());
    reload(m_dpConfig->getEgressPathConfig()->getCode(), 0,
           ProgramType::EGRESS);
  }
}

void Dynmon::setIngressPathConfig(const PathConfigJsonObject &config) {
  logger()->debug("[Dynmon] setIngressPathConfig(config)");
  std::lock_guard<std::mutex> lock_in(m_ingressPathMutex);
  try {
    std::string original_code = std::string(config.getCode());
    std::string swapped_code;
    for(auto &mc : config.getMetricConfigs()) {
      //Check if the map needs to be swapped
      if(mc.getExtractionOptions().getSwapOnRead()) {
        SwapConfig::formatCodeToSwap(original_code, swapped_code, mc.getMapName());
        ingressSwapState.enableSwap();
      }
    }
    if(ingressSwapState.isSwapEnabled()) {
      logger()->debug("[Dynmon] Swap enabled for INGRESS program");
      ingressSwapState.setCodes(original_code, swapped_code);
      //Load the original tuned code
      reload(original_code, 0, ProgramType::INGRESS);
    } else {
      //Load the real config code since no swap
      reload(config.getCode(), 0, ProgramType::INGRESS);
    }
    m_dpConfig->replaceIngressPathConfig(config);
  } catch (std::exception &ex) {
    logger()->error("ERROR injecting INGRESS path code: {0}", ex.what());
    logger()->info("Restoring default INGRESS path configuration");
    m_dpConfig->replaceIngressPathConfig(
        PathConfig(*m_dpConfig).toJsonObject());
    reload(m_dpConfig->getIngressPathConfig()->getCode(), 0,
           ProgramType::INGRESS);
  }
}

void Dynmon::resetEgressPathConfig() {
  logger()->debug("[Dynmon] resetEgressPathConfig()");
  std::lock_guard<std::mutex> lock_eg(m_egressPathMutex);

  PathConfigJsonObject conf;
  conf.setName(DEFAULT_PATH_NAME);
  conf.setCode(DEFAULT_PATH_CODE);
  egressSwapState = {};

  m_dpConfig->replaceEgressPathConfig(conf);
  reload(DEFAULT_PATH_CODE, 0, ProgramType::EGRESS);
}

void Dynmon::resetIngressPathConfig() {
  logger()->debug("[Dynmon] resetIngressPathConfig()");
  std::lock_guard<std::mutex> lock_eg(m_ingressPathMutex);

  PathConfigJsonObject conf;
  conf.setName(DEFAULT_PATH_NAME);
  conf.setCode(DEFAULT_PATH_CODE);
  ingressSwapState = {};

  m_dpConfig->replaceIngressPathConfig(conf);
  reload(DEFAULT_PATH_CODE, 0, ProgramType::INGRESS);
}

std::shared_ptr<Metrics> Dynmon::getMetrics() {
  logger()->debug("[Dynmon] getMetrics()");

  std::vector<shared_ptr<Metric>> egressMetrics, ingressMetrics;
  {
    std::lock_guard<std::mutex> lock_eg(m_egressPathMutex);
    triggerReadEgress();
    auto egressMetricConfigs =
        m_dpConfig->getEgressPathConfig()->getMetricConfigsList();

    // Extracting all metrics from the egress path
    for (auto &it : egressMetricConfigs) {
      try {
        auto metric = do_get_metric(it->getName(), it->getMapName(),
                                    ProgramType::EGRESS, it->getExtractionOptions());
        egressMetrics.push_back(metric);
      } catch (const std::exception &ex) {
        logger()->warn("{0}", ex.what());
        logger()->warn("Unable to read {0} map", it->getMapName());
      }
    }
  }

  {
    std::lock_guard<std::mutex> lock_in(m_ingressPathMutex);
    triggerReadIngress();
    auto ingressMetricConfigs =
        m_dpConfig->getIngressPathConfig()->getMetricConfigsList();

    // Extracting all metrics from the egress path
    for (auto &it : ingressMetricConfigs) {
      try {
        auto metric = do_get_metric(it->getName(), it->getMapName(),
                                    ProgramType::INGRESS, it->getExtractionOptions());
        ingressMetrics.push_back(metric);
      } catch (const std::exception &ex) {
        logger()->warn("{0}", ex.what());
        logger()->warn("Unable to read {0} map", it->getMapName());
      }
    }
  }

  Metrics metrics = Metrics(*this);

  for (auto &metric : egressMetrics)
    metrics.addEgressMetricUnsafe(metric);

  for (auto &metric : ingressMetrics)
    metrics.addIngressMetricUnsafe(metric);

  return std::make_shared<Metrics>(metrics);
}

std::shared_ptr<Metric> Dynmon::getEgressMetric(const std::string &name) {
  logger()->debug("[Dynmon] getEgressMetrics()");
  auto egressPathConfig = m_dpConfig->getEgressPathConfig();
  auto metricConfig = egressPathConfig->getMetricConfig(name);

  std::lock_guard<std::mutex> lock_eg(m_egressPathMutex);
  triggerReadEgress();

  try {
    return do_get_metric(name, metricConfig->getMapName(),
                         ProgramType::EGRESS, metricConfig->getExtractionOptions());
  } catch (const std::exception &ex) {
    logger()->warn("{0}", ex.what());
    std::string msg = "Unable to read " + metricConfig->getMapName() + " map";
    logger()->warn(msg);
    throw std::runtime_error(msg);
  }
}

std::vector<std::shared_ptr<Metric>> Dynmon::getEgressMetrics() {
  logger()->debug("[Dynmon] getEgressMetrics()");
  std::vector<std::shared_ptr<Metric>> metrics;
  auto egressMetricConfigs =
      m_dpConfig->getEgressPathConfig()->getMetricConfigsList();

  std::lock_guard<std::mutex> lock_eg(m_egressPathMutex);
  triggerReadEgress();

  // Extracting all metrics from the egress path
  for (auto &it : egressMetricConfigs) {
    try {
      // Extracting the metric value from the corresponding eBPF map
      auto value = do_get_metric(it->getMapName(), it->getMapName(),
                                 ProgramType::EGRESS, it->getExtractionOptions());
      metrics.push_back(value);
    } catch (const std::exception &ex) {
      logger()->warn("{0}", ex.what());
      logger()->warn("Unable to read {0} map", it->getMapName());
    }
  }
  return metrics;
}

std::shared_ptr<Metric> Dynmon::getIngressMetric(const std::string &name) {
  logger()->debug("[Dynmon] getIngressMetric()");
  auto ingressPathConfig = m_dpConfig->getIngressPathConfig();
  auto metricConfig = ingressPathConfig->getMetricConfig(name);

  std::lock_guard<std::mutex> lock_in(m_ingressPathMutex);
  triggerReadIngress();

  try {
    // Extracting the metric value from the corresponding eBPF map
    return do_get_metric(name, metricConfig->getMapName(),
                         ProgramType::INGRESS, metricConfig->getExtractionOptions());
  } catch (const std::exception &ex) {
    logger()->warn("{0}", ex.what());
    std::string msg = "Unable to read " + metricConfig->getMapName() + " map";
    logger()->warn(msg);
    throw std::runtime_error(msg);
  }
}

std::vector<std::shared_ptr<Metric>> Dynmon::getIngressMetrics() {
  logger()->debug("[Dynmon] getEgressMetrics()");
  std::vector<std::shared_ptr<Metric>> metrics;
  auto ingressMetricConfigs =
      m_dpConfig->getIngressPathConfig()->getMetricConfigsList();

  std::lock_guard<std::mutex> lock_in(m_ingressPathMutex);
  triggerReadIngress();

  // Extracting all metrics from the egress path
  for (auto &it : ingressMetricConfigs) {
    try {
      // Extracting the metric value from the corresponding eBPF map
      auto value = do_get_metric(it->getName(), it->getMapName(),
                                 ProgramType::INGRESS, it->getExtractionOptions());
      metrics.push_back(value);
    } catch (const std::exception &ex) {
      logger()->warn("{0}", ex.what());
      logger()->warn("Unable to read {0} map", it->getMapName());
    }
  }
  return metrics;
}

std::string Dynmon::getOpenMetrics() {
  logger()->debug("[Dynmon] getOpenMetrics()");

  std::string eg_metrics, in_metrics;
  {
    std::vector<std::string> metrics;
    auto egressMetricConfigs =
        m_dpConfig->getEgressPathConfig()->getMetricConfigsList();

    std::lock_guard<std::mutex> lock_eg(m_egressPathMutex);
    triggerReadEgress();

    // Extracting all metrics from the egress path
    for (auto &it : egressMetricConfigs) {
      try {
        auto metadata = it->getOpenMetricsMetadata();
        if (metadata == nullptr)
          continue;

        auto metric = do_get_open_metric(it, ProgramType::EGRESS);
        metrics.push_back(metric);
      } catch (const std::exception &ex) {
        logger()->warn("{0}", ex.what());
        logger()->warn("Unable to read {0} map", it->getMapName());
      }
    }
    eg_metrics = Utils::join(metrics, "\n");
  }
  {
    std::vector<std::string> metrics;
    auto ingressMetricConfigs =
        m_dpConfig->getIngressPathConfig()->getMetricConfigsList();

    std::lock_guard<std::mutex> lock_in(m_ingressPathMutex);
    triggerReadIngress();

    // Extracting all metrics from the egress path
    for (auto &it : ingressMetricConfigs) {
      try {
        auto metadata = it->getOpenMetricsMetadata();
        if (metadata == nullptr)
          continue;

        auto metric = do_get_open_metric(it, ProgramType::INGRESS);
        metrics.push_back(metric);
      } catch (const std::exception &ex) {
        logger()->warn("{0}", ex.what());
        logger()->warn("Unable to read {0} map", it->getMapName());
      }
    }
    in_metrics = Utils::join(metrics, "\n");
  }

  if (eg_metrics.empty())
    return in_metrics;
  if (in_metrics.empty())
    return eg_metrics;
  return eg_metrics + "\n" + in_metrics;
}

/*UNUSED: left here for future endpoints development*/
std::string Dynmon::getEgressOpenMetrics() {
  logger()->debug("[Dynmon] getEgressOpenMetrics()");
  std::vector<std::string> metrics;
  auto egressMetricConfigs =
      m_dpConfig->getEgressPathConfig()->getMetricConfigsList();

  std::lock_guard<std::mutex> lock_eg(m_egressPathMutex);
  triggerReadEgress();

  // Extracting all metrics from the egress path
  for (auto &it : egressMetricConfigs) {
    try {
      auto metadata = it->getOpenMetricsMetadata();
      if (metadata == nullptr)
        continue;

      auto metric = do_get_open_metric(it, ProgramType::EGRESS);
      metrics.push_back(metric);
    } catch (const std::exception &ex) {
      logger()->warn("{0}", ex.what());
      logger()->warn("Unable to read {0} map", it->getMapName());
    }
  }
  return Utils::join(metrics, "\n");
}

/*UNUSED: left here for future endpoints development*/
std::string Dynmon::getIngressOpenMetrics() {
  logger()->debug("[Dynmon] getEgressMetrics()");
  std::vector<std::string> metrics;
  auto ingressMetricConfigs =
      m_dpConfig->getIngressPathConfig()->getMetricConfigsList();

  std::lock_guard<std::mutex> lock_in(m_ingressPathMutex);
  triggerReadIngress();

  // Extracting all metrics from the egress path
  for (auto &it : ingressMetricConfigs) {
    try {
      auto metadata = it->getOpenMetricsMetadata();
      if (metadata == nullptr)
        continue;

      // Extracting the metric value from the corresponding eBPF map
      auto metric = do_get_open_metric(it, ProgramType::INGRESS);
      metrics.push_back(metric);
    } catch (const std::exception &ex) {
      logger()->warn("{0}", ex.what());
      logger()->warn("Unable to read {0} map", it->getMapName());
    }
  }
  return Utils::join(metrics, "\n");
}

std::string Dynmon::toOpenMetrics(const std::shared_ptr<MetricConfig>& conf,
                                  nlohmann::json value) {
  std::vector<std::string> sub_metrics;
  try {
    auto metadata = conf->getOpenMetricsMetadata();

    if (value.is_null() || value[0].is_null() || !value[0].is_number()) {
      logger()->warn("Unable to convert metric {0} to OpenMetrics format.",
                     conf->getName());
      return "";
    }

    // Transforming the metric value in the OpenMetric format
    for (auto i = 0; i < value.size(); i++) {
      std::string strMetric;
      // Appending metric name
      strMetric.append(conf->getName() +
                       (value.size() == 1 ? "" : "_" + std::to_string(i)));

      // Appending metric labels
      strMetric.append("{");
      auto labels = metadata->getLabelsList();
      for (auto label = labels.begin(); label != labels.end(); label++)
        strMetric.append(label->get()->getName() + "=\"" +
                         label->get()->getValue() + "\"" +
                         ((std::next(label) != labels.end()) ? ", " : ""));
      strMetric.append("} ");

      // Appending metric value
      strMetric.append(std::to_string(value[i].get<uint64_t>()));

      // Adding the HELP header
      sub_metrics.push_back("#HELP " + conf->getName() + " " +
                            metadata->getHelp());

      // Adding the TYPE header
      sub_metrics.push_back(
          "#TYPE " + conf->getName() + " " +
          OpenMetricsMetadataJsonObject::MetricTypeEnum_to_string(
              metadata->getType()));
      // Adding the metric
      sub_metrics.push_back(strMetric);
    }

  } catch (const std::exception &ex) {
    logger()->warn("{0}", ex.what());
    logger()->warn("Unable to read {0} map", conf->getMapName());
  }
  // Joining the sub_metrics in a unique string before returning them
  return Utils::join(sub_metrics, "\n");
}

void Dynmon::triggerReadEgress() {
  if(egressSwapState.isSwapEnabled()) {
    logger()->debug("[Dynmon] Triggered read EGRESS! Loading the {} code", egressSwapState.hasSwapped()? "original" : "swap");
    reload(egressSwapState.getCodeToLoad(), 0, ProgramType::EGRESS);
  }
}

void Dynmon::triggerReadIngress() {
  if(ingressSwapState.isSwapEnabled()) {
    logger()->debug("[Dynmon] Triggered read INGRESS! Loading the {} code", ingressSwapState.hasSwapped()? "original" : "swap");
    reload(ingressSwapState.getCodeToLoad(), 0, ProgramType::INGRESS);
  }
}

std::shared_ptr<Metric> Dynmon::do_get_metric(const std::string& name, std::string mapName,
                                              ProgramType type,
                                              const std::shared_ptr<ExtractionOptions>& extractionOptions) {
  if(extractionOptions->getSwapOnRead() &&
      ( (type == ProgramType::INGRESS && !ingressSwapState.hasSwapped() ) ||
       (type == ProgramType::EGRESS && !egressSwapState.hasSwapped()) )) {
      SwapConfig::formatMapName(mapName);
      logger()->debug("[Dynmon] {} - Reading from Map: {} since code has swapped back",
                      type == ProgramType::INGRESS ? "INGRESS" : "EGRESS",mapName);
  }
  auto value = MapExtractor::extractFromMap(*this, mapName,
                                            0, type,
                                            extractionOptions);
  return std::make_shared<Metric>(*this, name, value,
                                  Utils::genTimestampMicroSeconds());
}

std::string Dynmon::do_get_open_metric(const shared_ptr<MetricConfig>& config, ProgramType type) {

  auto extractionOptions = config->getExtractionOptions();
  auto mapName = config->getMapName();

  if(extractionOptions->getSwapOnRead() &&
     ( (type == ProgramType::INGRESS && !ingressSwapState.hasSwapped() ) ||
       (type == ProgramType::EGRESS && !egressSwapState.hasSwapped()) )) {
    SwapConfig::formatMapName(mapName);
    logger()->debug("[Dynmon] {} - Reading from Map: {} since code has swapped back",
                    type == ProgramType::INGRESS ? "INGRESS" : "EGRESS",mapName);
  }

  auto value = MapExtractor::extractFromMap(*this, mapName,
                                            0, type,
                                            extractionOptions);
  return toOpenMetrics(config, value);
}