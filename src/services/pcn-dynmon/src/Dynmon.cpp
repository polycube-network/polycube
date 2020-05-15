#include "Dynmon.h"
#include "Dynmon_dp.h"
#include "MapExtractor.h"
#include "Utils.h"

Dynmon::Dynmon(const std::string name, const DynmonJsonObject &conf)
  : TransparentCube(conf.getBase(), { dynmon_code }, {}),
    DynmonBase(name) {
  logger()->info("Creating Dynmon instance");
  // Loading the default dataplane configuration
  addDataplane(DataplaneJsonObject());
  // try to inject the received dataplane configuration
  try {
    replaceDataplane(conf.getDataplane());
  } catch (...) {}
}

Dynmon::~Dynmon() {
  logger()->info("Destroying Dynmon instance");
}

void Dynmon::packet_in(polycube::service::Direction direction,
    polycube::service::PacketInMetadata &md,
    const std::vector<uint8_t> &packet) {
    logger()->debug("Packet received");
}

std::shared_ptr<Dataplane> Dynmon::getDataplane() {
  std::lock_guard<std::mutex> guard(dataplane_mutex_);
  return dataplane_;
}

void Dynmon::addDataplane(const DataplaneJsonObject &conf) {
  std::lock_guard<std::mutex> guard(dataplane_mutex_);
  if (dataplane_ == nullptr)
    dataplane_ = std::make_shared<Dataplane>(*this, conf);
}

void Dynmon::replaceDataplane(const DataplaneJsonObject &conf) {
  std::lock_guard<std::mutex> guard(dataplane_mutex_);

  std::shared_ptr<Dataplane> new_dp = std::make_shared<Dataplane>(*this, conf);

  // INJECTING THE eBPF CODE
  try {
    // The reload method of the cube injects the eBPF code in Kernel.
    // If the eBPF code is rejected by the eBPF verifier, an exception is thrown
    reload(new_dp->getCode());
    dataplane_ = new_dp;
    logger()->debug("[DATAPLANE] " + new_dp->getName() + " loaded");
  } catch (...) {
    // The eBPF code present in the provided configuration is invalid;
    // the previous configuration is loaded.
    reload(dataplane_->getCode());
    std::string error =
        "[DATAPLANE] Invalid eBPF code; previous dataplane loaded";
    // Logging the verifier excpetion
    logger()->debug(error);
    // Throwing an exception containing the error
    throw std::runtime_error(error);
  }
}

void Dynmon::delDataplane() {
  std::lock_guard<std::mutex> guard(dataplane_mutex_);
  dataplane_ = nullptr;
}

std::shared_ptr<Metrics> Dynmon::getMetrics(const std::string &name) {
  std::lock_guard<std::mutex> guard(dataplane_mutex_);

  auto metrics_info = dataplane_->getMetricsList();

  // Looking for the metric info stored in the current dataplane
  for (auto &it : metrics_info) {
    if (it->getName() == name) {
      // Extracting the metric value from the corresponding eBPF map
      auto value = MapExtractor::extractFromMap(*this, it->getMapName(), 0,
                                                ProgramType::INGRESS);
      Metrics metric =
          Metrics(*this, it->getName(), value.dump(), Utils::genTimestampMicroSeconds());
      return std::make_shared<Metrics>(metric);
    }
  }

  // Throwing a runtime error if the metric does not exist
  throw std::runtime_error("Metric " + name + " does not exist.");
}

std::vector<std::shared_ptr<Metrics>> Dynmon::getMetricsList() {
  std::lock_guard<std::mutex> guard(dataplane_mutex_);
  
  std::vector<std::shared_ptr<Metrics>> metrics;
  auto metrics_info = dataplane_->getMetricsList();

  // Getting all the metrics 
  for (auto &it : metrics_info) {
    try {
      // Extracting the metric value from the corresponding eBPF map
      auto value = MapExtractor::extractFromMap(*this, it->getMapName(), 0,
                                                ProgramType::INGRESS);
      if (value.is_array() && value.size() == 1)
        value = value[0];
      Metrics metric =
          Metrics(*this, it->getName(), value.dump(), Utils::genTimestampMicroSeconds());
      metrics.push_back(std::make_shared<Metrics>(metric));
    } catch (const std::exception &ex) {
      logger()->warn("{0}",ex.what());
      logger()->warn("Unable to read {0} map", it->getMapName());
    }
  }
  return metrics;
}

std::string Dynmon::getOpenMetrics() {
  std::lock_guard<std::mutex> guard(dataplane_mutex_);
  
  std::vector<std::string> metrics;
  auto metrics_info = dataplane_->getMetricsList();

  // Getting all the metrics 
  for (auto &it : metrics_info) {
    try {
      auto metadata = it->getOpenMetricsMetadata();
      auto labels = metadata->getLabelsList();

      // Extracting the metric value from the corresponding eBPF map
      auto value = MapExtractor::extractFromMap(*this, it->getMapName(), 0,
                                                ProgramType::INGRESS);
      if (value.is_null() || value[0].is_null() || !value[0].is_number()){
        logger()->warn("Unable to convert metric {0} to OpenMetrics format.",it->getName());
        continue;
      }

      // Transforming the metric value in the OpenMetric format
      for (auto i = 0; i < value.size(); i++) {
        std::string strMetric;
        // Appending metric name
        strMetric.append(it->getName() +
                         (value.size() == 1 ? "" : "_" + std::to_string(i)));

        // Appending metric labels
        strMetric.append("{");
        for (auto label = labels.begin(); label != labels.end(); label++)
          strMetric.append(label->get()->getName() + "=\"" +
                           label->get()->getValue() + "\"" +
                           ((std::next(label) != labels.end()) ? ", " : ""));
        strMetric.append("} ");

        // Appending metric value
        strMetric.append(std::to_string(value[i].get<uint64_t>()));
        strMetric.append(" ");
        strMetric.append(std::to_string(Utils::genTimestampSeconds()));

        // Adding the HELP header
        metrics.push_back("#HELP " + it->getName() + " " + metadata->getHelp());
        
        // Adding the TYPE header
        metrics.push_back(
            "#TYPE " + it->getName() + " " +
            DataplaneMetricsOpenMetricsMetadataJsonObject::
                DataplaneMetricsOpenMetricsMetadataTypeEnum_to_string(
                    metadata->getType()));

        // Adding the metric
        metrics.push_back(strMetric);
      }

    } catch (const std::exception &ex) {
        logger()->warn("{0}",ex.what());
      logger()->warn("Unable to read {0} map", it->getMapName());
    }
  }

  // Joining the metrics in a unique string before returning them
  return Utils::join(metrics, "\n");
}

void Dynmon::addMetrics(const std::string &name, const MetricsJsonObject &conf) {}

void Dynmon::addMetricsList(const std::vector<MetricsJsonObject> &conf) {}

void Dynmon::replaceMetrics(const std::string &name, const MetricsJsonObject &conf) {}

void Dynmon::delMetrics(const std::string &name) {}

void Dynmon::delMetricsList() {}