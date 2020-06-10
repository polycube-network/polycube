#include <regex>

#include "Dynmon.h"

#include "Utils.h"

Dynmon::Dynmon(const std::string name, const DynmonJsonObject &conf)
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
        formatCodeToSwap(original_code, swapped_code, mc.getMapName());
        egressSwapState.enableSwap();
      }
    }
    if(egressSwapState.isSwapEnabled()) {
      logger()->debug("[Dynmon] Swap enabled for EGRESS program");
      egressSwapState.setSwapCode(swapped_code);
      egressSwapState.setOrigCode(original_code);
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
        formatCodeToSwap(original_code, swapped_code, mc.getMapName());
        ingressSwapState.enableSwap();
      }
    }
    if(ingressSwapState.isSwapEnabled()) {
      logger()->debug("[Dynmon] Swap enabled for INGRESS program");
      ingressSwapState.setSwapCode(swapped_code);
      ingressSwapState.setOrigCode(original_code);
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

  auto egressMetrics = getEgressMetrics();
  auto ingressMetrics = getIngressMetrics();

  Metrics metrics = Metrics(*this);

  for (auto &metric : egressMetrics)
    metrics.addEgressMetricUnsafe(metric);

  for (auto &metric : ingressMetrics)
    metrics.addIngressMetricUnsafe(metric);

  return std::make_shared<Metrics>(metrics);
}

std::shared_ptr<Metric> Dynmon::getEgressMetric(const std::string &name) {
  logger()->debug("[Dynmon] getEgressMetrics()");
  std::lock_guard<std::mutex> lock_eg(m_egressPathMutex);
  auto egressPathConfig = m_dpConfig->getEgressPathConfig();
  auto metricConfig = egressPathConfig->getMetricConfig(name);

  try {
    // Extracting the metric value from the corresponding eBPF map
    std::string mapName = metricConfig->getMapName();
    auto extractionOptions = metricConfig->getExtractionOptions();
    if(extractionOptions->getSwapOnRead() && !egressSwapState.hasSwapped()) {
      SwapStateConfig::formatMapName(mapName);
      logger()->debug("[Dynmon] EGRESS - Reading from Map: {} since code has swapped back", mapName);
    }
    auto value = MapExtractor::extractFromMap(*this, mapName,0, ProgramType::EGRESS, extractionOptions);
    return std::make_shared<Metric>(*this, metricConfig->getName(), value,
                                    Utils::genTimestampMicroSeconds());
  } catch (const std::exception &ex) {
    logger()->warn("{0}", ex.what());
    std::string msg = "Unable to read " + metricConfig->getMapName() + " map";
    logger()->warn(msg);
    throw std::runtime_error(msg);
  }
}

std::vector<std::shared_ptr<Metric>> Dynmon::getEgressMetrics() {
  logger()->debug("[Dynmon] getEgressMetrics()");
  std::lock_guard<std::mutex> lock_eg(m_egressPathMutex);

  std::vector<std::shared_ptr<Metric>> metrics;
  auto egressMetricConfigs =
      m_dpConfig->getEgressPathConfig()->getMetricConfigsList();

  // Extracting all metrics from the egress path
  for (auto &it : egressMetricConfigs) {
    try {
      // Extracting the metric value from the corresponding eBPF map
      std::string mapName = it->getMapName();
      auto extractionOptions = it->getExtractionOptions();
      if(extractionOptions->getSwapOnRead() && !egressSwapState.hasSwapped()) {
        SwapStateConfig::formatMapName(mapName);
        logger()->debug("[Dynmon] EGRESS - Reading from Map: {} since code has swapped back", mapName);
      }
      auto value = MapExtractor::extractFromMap(*this, mapName, 0,
                                                ProgramType::EGRESS,
                                                extractionOptions);
      metrics.push_back(std::make_shared<Metric>(
          *this, it->getName(), value, Utils::genTimestampMicroSeconds()));
    } catch (const std::exception &ex) {
      logger()->warn("{0}", ex.what());
      logger()->warn("Unable to read {0} map", it->getMapName());
    }
  }
  return metrics;
}

std::shared_ptr<Metric> Dynmon::getIngressMetric(const std::string &name) {
  logger()->debug("[Dynmon] getIngressMetric()");
  std::lock_guard<std::mutex> lock_in(m_ingressPathMutex);
  auto ingressPathConfig = m_dpConfig->getIngressPathConfig();
  auto metricConfig = ingressPathConfig->getMetricConfig(name);

  try {
    // Extracting the metric value from the corresponding eBPF map
    std::string mapName = metricConfig->getMapName();
    auto extractionOptions = metricConfig->getExtractionOptions();
    if(extractionOptions->getSwapOnRead() && !ingressSwapState.hasSwapped()) {
      SwapStateConfig::formatMapName(mapName);
      logger()->debug("[Dynmon] INGRESS - Reading from Map: {} since code has swapped back", mapName);
    }
    auto value = MapExtractor::extractFromMap(*this, mapName,
                                              0, ProgramType::INGRESS,
                                              extractionOptions);
    return std::make_shared<Metric>(*this, metricConfig->getName(), value,
                                    Utils::genTimestampMicroSeconds());
  } catch (const std::exception &ex) {
    logger()->warn("{0}", ex.what());
    std::string msg = "Unable to read " + metricConfig->getMapName() + " map";
    logger()->warn(msg);
    throw std::runtime_error(msg);
  }
}

std::vector<std::shared_ptr<Metric>> Dynmon::getIngressMetrics() {
  logger()->debug("[Dynmon] getEgressMetrics()");
  std::lock_guard<std::mutex> lock_in(m_ingressPathMutex);

  std::vector<std::shared_ptr<Metric>> metrics;
  auto ingressMetricConfigs =
      m_dpConfig->getIngressPathConfig()->getMetricConfigsList();

  // Extracting all metrics from the egress path
  for (auto &it : ingressMetricConfigs) {
    try {
      // Extracting the metric value from the corresponding eBPF map
      std::string mapName = it->getMapName();
      auto extractionOptions = it->getExtractionOptions();
      if(extractionOptions->getSwapOnRead() && !ingressSwapState.hasSwapped()) {
        SwapStateConfig::formatMapName(mapName);
        logger()->debug("[Dynmon] INGRESS - Reading from Map: {} since code has swapped back", mapName);
      }
      auto value = MapExtractor::extractFromMap(*this, mapName, 0,
                                                ProgramType::INGRESS,
                                                extractionOptions);
      metrics.push_back(std::make_shared<Metric>(
          *this, it->getName(), value, Utils::genTimestampMicroSeconds()));
    } catch (const std::exception &ex) {
      logger()->warn("{0}", ex.what());
      logger()->warn("Unable to read {0} map", it->getMapName());
    }
  }
  return metrics;
}

std::string Dynmon::getOpenMetrics() {
  logger()->debug("[Dynmon] getOpenMetrics()");
  auto eg_metrics = getEgressOpenMetrics();
  auto in_metrics = getIngressOpenMetrics();
  if (eg_metrics.empty())
    return in_metrics;
  if (in_metrics.empty())
    return eg_metrics;
  return eg_metrics + "\n" + in_metrics;
}

std::string Dynmon::getEgressOpenMetrics() {
  logger()->debug("[Dynmon] getEgressOpenMetrics()");
  std::lock_guard<std::mutex> lock_eg(m_egressPathMutex);

  std::vector<std::string> metrics;
  auto egressMetricConfigs =
      m_dpConfig->getEgressPathConfig()->getMetricConfigsList();

  // Extracting all metrics from the egress path
  for (auto &it : egressMetricConfigs) {
    try {
      auto metadata = it->getOpenMetricsMetadata();
      if (metadata == nullptr)
        continue;

      std::string mapName = it->getMapName();
      auto extractionOptions = it->getExtractionOptions();
      if(extractionOptions->getSwapOnRead() && !egressSwapState.hasSwapped()) {
        SwapStateConfig::formatMapName(mapName);
        logger()->debug("[Dynmon] EGRESS - Reading from Map: {} since code has swapped back", mapName);
      }
      // Extracting the metric value from the corresponding eBPF map
      auto value = MapExtractor::extractFromMap(*this, mapName, 0,
                                                ProgramType::EGRESS,
                                                extractionOptions);
      auto metric = toOpenMetrics(it, value);
      metrics.push_back(metric);
    } catch (const std::exception &ex) {
      logger()->warn("{0}", ex.what());
      logger()->warn("Unable to read {0} map", it->getMapName());
    }
  }
  return Utils::join(metrics, "\n");
}

std::string Dynmon::getIngressOpenMetrics() {
  logger()->debug("[Dynmon] getEgressMetrics()");
  std::lock_guard<std::mutex> lock_in(m_ingressPathMutex);

  std::vector<std::string> metrics;
  auto ingressMetricConfigs =
      m_dpConfig->getIngressPathConfig()->getMetricConfigsList();

  // Extracting all metrics from the egress path
  for (auto &it : ingressMetricConfigs) {
    try {
      auto metadata = it->getOpenMetricsMetadata();
      if (metadata == nullptr)
        continue;

      std::string mapName = it->getMapName();
      auto extractionOptions = it->getExtractionOptions();
      if(extractionOptions->getSwapOnRead() && !egressSwapState.hasSwapped()) {
        SwapStateConfig::formatMapName(mapName);
        logger()->debug(
            "[Dynmon] EGRESS - Reading from Map: {} since code has swapped back",
            mapName);
      }
      // Extracting the metric value from the corresponding eBPF map
      auto value = MapExtractor::extractFromMap(*this, it->getMapName(), 0,
                                                ProgramType::INGRESS,
                                                extractionOptions);
      auto metric = toOpenMetrics(it, value);
      metrics.push_back(metric);
    } catch (const std::exception &ex) {
      logger()->warn("{0}", ex.what());
      logger()->warn("Unable to read {0} map", it->getMapName());
    }
  }
  return Utils::join(metrics, "\n");
}

std::string Dynmon::toOpenMetrics(std::shared_ptr<MetricConfig> conf,
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
    egressSwapState.triggerHasSwapped();
    if(egressSwapState.hasSwapped()) {
      logger()->debug("[Dynmon] Triggered read EGRESS! Loading Swap code and read from original");
      reload(egressSwapState.getSwapCode(), 0, ProgramType::EGRESS);
    } else {
      logger()->debug("[Dynmon] Triggered read EGRESS! Loading original code and read from Swap");
      reload(egressSwapState.getOrigCode(), 0, ProgramType::EGRESS);
    }
  }
}

void Dynmon::triggerReadIngress() {
  if(ingressSwapState.isSwapEnabled()) {
    ingressSwapState.triggerHasSwapped();
    if(ingressSwapState.hasSwapped()) {
      logger()->debug("[Dynmon] Triggered read INGRESS! Loading Swap code and read from original");
      reload(ingressSwapState.getSwapCode(), 0, ProgramType::INGRESS);
    } else {
      logger()->debug("[Dynmon] Triggered read INGRESS! Loading original code and read from Swap");
      reload(ingressSwapState.getOrigCode(), 0, ProgramType::INGRESS);
    }
  }
}

void Dynmon::formatCodeToSwap(std::string &original_code, std::string &swapped_code,
                              std::string mapName) {
  std::regex name_sub ("(" + mapName + ")");   // to change from name to name_1
  std::regex map_decl ("(BPF_.+" + mapName +".+)"); // to declare the map the same, otherwise not accessible
  std::smatch match1, match2;
  swapped_code = std::regex_replace(original_code, name_sub, mapName + SWAP_MAP_NAME_FORMAT);
  if(std::regex_search(swapped_code, match1, map_decl) && std::regex_search(original_code, match2, map_decl)) {
    std::string orig_decl = match2[0].str();
    std::string swap_decl = match1[0].str();
    std::size_t orig_index = original_code.find(orig_decl);   //index must exists since regex has matched
    std::size_t swap_index = swapped_code.find(swap_decl);    //index must exists since regex has matched
    original_code.insert(orig_index, swap_decl + "\n");
    swapped_code.insert(swap_index, orig_decl + "\n");
  } else
    throw std::runtime_error("Fix regex search method - formatCodeToSwap()");
}