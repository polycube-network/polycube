#include "Dyntmon.h"
#include "Dyntmon_dp.h"
#include "MapExtractor.h"
#include "Utils.h"

Dyntmon::Dyntmon(const std::string name, const DyntmonJsonObject &conf)
    : TransparentCube(conf.getBase(), {dyntmon_code}, {}), DyntmonBase(name) {
  logger()->info("Creating Dyntmon instance");
  addDataplane(conf.getDataplane());
  if (conf.dataplaneIsSet())
    dataplane_->reload();
}

Dyntmon::~Dyntmon() {
  logger()->info("Destroying Dyntmon instance");
}

void Dyntmon::packet_in(polycube::service::Direction direction,
                        polycube::service::PacketInMetadata &md,
                        const std::vector<uint8_t> &packet) {
  logger()->debug("Packet received");
}

std::shared_ptr<Dataplane> Dyntmon::getDataplane() {
  logger()->trace("[Dyntmon] getDataplane()");
  return dataplane_;
}

void Dyntmon::addDataplane(const DataplaneJsonObject &value) {
  logger()->trace("[Dyntmon] addDataplane()");
  if (dataplane_ == nullptr)
    dataplane_ = std::make_shared<Dataplane>(*this, value);
}

void Dyntmon::replaceDataplane(const DataplaneJsonObject &conf) {
  logger()->trace("[Dyntmon] replaceDataplane()");
  delDataplane();
  addDataplane(conf);
}

void Dyntmon::delDataplane() {
  dataplane_ = nullptr;
}

std::shared_ptr<Metrics> Dyntmon::getMetrics(const std::string &name) {
  logger()->trace("[Dyntmon] getMetrics()");
  auto metrics_info = dataplane_->getMetricsList();
  for (auto &it : metrics_info) {
    if (it->getName() == name) {
      auto value = MapExtractor::extractFromMap(*this, it->getMapName(), 0,
                                                ProgramType::INGRESS);
      Metrics metric =
          Metrics(*this, it->getName(), value.dump(), Utils::genTimestamp());
      return std::make_shared<Metrics>(metric);
    }
    throw std::runtime_error("Metric " + name + " does not exist.");
  }
}

std::vector<std::shared_ptr<Metrics>> Dyntmon::getMetricsList() {
  logger()->trace("[Dyntmon] getMetricsList()");
  std::vector<std::shared_ptr<Metrics>> metrics;
  auto metrics_info = dataplane_->getMetricsList();

  for (auto &it : metrics_info) {
    try {
      auto value = MapExtractor::extractFromMap(*this, it->getMapName(), 0,
                                                ProgramType::INGRESS);
      if (value.is_array() && value.size() == 1)
        value = value[0];
      Metrics metric =
          Metrics(*this, it->getName(), value.dump(), Utils::genTimestamp());
      metrics.push_back(std::make_shared<Metrics>(metric));
    } catch (...) {
      logger()->error("Unable to read {0} map", it->getMapName());
    }
  }
  return metrics;
}

std::string Dyntmon::getOpenMetrics() {
  logger()->trace("[Dyntmon] getOpenMetrics()");
  std::vector<std::string> metrics;
  auto metrics_info = dataplane_->getMetricsList();

  for (auto &it : metrics_info) {
    try {
      auto metadata = it->getOpenMetricsMetadata();
      auto labels = metadata->getLabelList();
      auto value = MapExtractor::extractFromMap(*this, it->getMapName(), 0,
                                                ProgramType::INGRESS);
      if (value.is_null())
        continue;

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
        strMetric.append(std::to_string(Utils::genTimestamp()));

        metrics.push_back("#HELP " + it->getName() + " " + metadata->getHelp());
        metrics.push_back(
            "#TYPE " + it->getName() + " " +
            DataplaneMetricsOpenMetricsMetadataJsonObject::
                DataplaneMetricsOpenMetricsMetadataTypeEnum_to_string(
                    metadata->getType()));
        metrics.push_back(strMetric);
      }

    } catch (const std::exception &ex) {
      logger()->debug("{0}", ex.what());
      logger()->error("Unable to read {0} map", it->getMapName());
    }
  }

  return Utils::join(metrics, "\n");
}

void Dyntmon::addMetrics(const std::string &name,
                         const MetricsJsonObject &conf) {}

void Dyntmon::addMetricsList(const std::vector<MetricsJsonObject> &conf) {}

void Dyntmon::replaceMetrics(const std::string &name,
                             const MetricsJsonObject &conf) {}

void Dyntmon::delMetrics(const std::string &name) {}

void Dyntmon::delMetricsList() {}
