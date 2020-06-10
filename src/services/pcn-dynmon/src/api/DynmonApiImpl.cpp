#include "DynmonApiImpl.h"

namespace polycube {
  namespace service {
    namespace api {
      using namespace polycube::service::model;
      namespace DynmonApiImpl {
        namespace {
          std::unordered_map<std::string, std::shared_ptr<Dynmon>> cubes;
          std::mutex cubes_mutex;

          std::shared_ptr<Dynmon> get_cube(const std::string &name) {
            std::lock_guard<std::mutex> guard(cubes_mutex);
            auto iter = cubes.find(name);
            if (iter == cubes.end())
              throw std::runtime_error("Cube " + name + " does not exist");
            return iter->second;
          }
        }// namespace

        void create_dynmon_by_id(const std::string &name, const DynmonJsonObject &jsonObject) {
          {
            // check if name is valid before creating it
            std::lock_guard<std::mutex> guard(cubes_mutex);
            if (cubes.count(name) != 0) {
              throw std::runtime_error("There is already a cube with name " + name);
            }
          }
          auto ptr = std::make_shared<Dynmon>(name, jsonObject);
          std::unordered_map<std::string, std::shared_ptr<Dynmon>>::iterator iter;
          bool inserted;
          std::lock_guard<std::mutex> guard(cubes_mutex);
          std::tie(iter, inserted) = cubes.emplace(name, std::move(ptr));
          if (!inserted)
            throw std::runtime_error("There is already a cube with name " + name);
        }

        void replace_dynmon_by_id(const std::string &name, const DynmonJsonObject &bridge) {
          throw std::runtime_error("Method not supported!");
        }

        void delete_dynmon_by_id(const std::string &name) {
          std::lock_guard<std::mutex> guard(cubes_mutex);
          if (cubes.count(name) == 0)
            throw std::runtime_error("Cube " + name + " does not exist");
          cubes.erase(name);
        }

        std::vector<DynmonJsonObject> read_dynmon_list_by_id() {
          std::vector<DynmonJsonObject> jsonObject_vect;
          for (auto &i : cubes) {
            auto m = get_cube(i.first);
            jsonObject_vect.push_back(m->toJsonObject());
          }
          return jsonObject_vect;
        }

        std::vector<nlohmann::fifo_map<std::string, std::string>> read_dynmon_list_by_id_get_list() {
          std::vector<nlohmann::fifo_map<std::string, std::string>> r;
          for (auto &x : cubes) {
            nlohmann::fifo_map<std::string, std::string> m;
            m["name"] = x.first;
            r.push_back(std::move(m));
          }
          return r;
        }

        /**
         * @brief   Create dataplane-config by ID
         *
         * Create operation of resource: dataplane-config*
         *
         * @param[in] name ID of name
         * @param[in] value dataplane-configbody object
         *
         * Responses:
         *
         */
        void
        create_dynmon_dataplane_config_by_id(const std::string &name, const DataplaneConfigJsonObject &value) {
          auto dynmon = get_cube(name);
          return dynmon->setDataplaneConfig(value);
        }

        /**
         * @brief   Create egress by ID
         *
         * Create operation of resource: egress*
         *
         * @param[in] name ID of name
         * @param[in] value egressbody object
         *
         * Responses:
         *
         */
        void
        create_dynmon_dataplane_config_egress_path_by_id(const std::string &name,
                                                         const PathConfigJsonObject &value) {
          auto dynmon = get_cube(name);
          auto dataplaneConfig = dynmon->getDataplaneConfig();
          return dataplaneConfig->addEgressPathConfig(value);
        }

        /**
         * @brief   Create ingress by ID
         *
         * Create operation of resource: ingress*
         *
         * @param[in] name ID of name
         * @param[in] value ingressbody object
         *
         * Responses:
         *
         */
        void
        create_dynmon_dataplane_config_ingress_path_by_id(const std::string &name,
                                                          const PathConfigJsonObject &value) {
          auto dynmon = get_cube(name);
          auto dataplaneConfig = dynmon->getDataplaneConfig();
          return dataplaneConfig->addIngressPathConfig(value);
        }

        /**
         * @brief   Delete dataplane-config by ID
         *
         * Delete operation of resource: dataplane-config*
         *
         * @param[in] name ID of name
         *
         * Responses:
         *
         */
        void
        delete_dynmon_dataplane_config_by_id(const std::string &name) {
          auto dynmon = get_cube(name);
          return dynmon->resetDataplaneConfig();
        }

        /**
         * @brief   Delete egress by ID
         *
         * Delete operation of resource: egress*
         *
         * @param[in] name ID of name
         *
         * Responses:
         *
         */
        void
        delete_dynmon_dataplane_config_egress_path_by_id(const std::string &name) {
          auto dynmon = get_cube(name);
          return dynmon->resetEgressPathConfig();
        }

        /**
         * @brief   Delete ingress by ID
         *
         * Delete operation of resource: ingress*
         *
         * @param[in] name ID of name
         *
         * Responses:
         *
         */
        void
        delete_dynmon_dataplane_config_ingress_path_by_id(const std::string &name) {
          auto dynmon = get_cube(name);
          return dynmon->resetIngressPathConfig();
        }

        /**
         * @brief   Read dynmon by ID
         *
         * Read operation of resource: dynmon*
         *
         * @param[in] name ID of name
         *
         * Responses:
         * DynmonJsonObject
         */
        DynmonJsonObject
        read_dynmon_by_id(const std::string &name) {
          return get_cube(name)->toJsonObject();
        }

        /**
         * @brief   Read dataplane-config by ID
         *
         * Read operation of resource: dataplane-config*
         *
         * @param[in] name ID of name
         *
         * Responses:
         * DataplaneConfigJsonObject
         */
        DataplaneConfigJsonObject
        read_dynmon_dataplane_config_by_id(const std::string &name) {
          auto dynmon = get_cube(name);
          return dynmon->getDataplaneConfig()->toJsonObject();
        }

        /**
         * @brief   Read egress by ID
         *
         * Read operation of resource: egress*
         *
         * @param[in] name ID of name
         *
         * Responses:
         * PathConfigJsonObject
         */
        PathConfigJsonObject
        read_dynmon_dataplane_config_egress_path_by_id(const std::string &name) {
          auto dynmon = get_cube(name);
          auto dataplaneConfig = dynmon->getDataplaneConfig();
          auto egressPathConfig = dataplaneConfig->getEgressPathConfig();
          return dataplaneConfig->getEgressPathConfig()->toJsonObject();
        }

        /**
         * @brief   Read code by ID
         *
         * Read operation of resource: code*
         *
         * @param[in] name ID of name
         *
         * Responses:
         * std::string
         */
        std::string
        read_dynmon_dataplane_config_egress_path_code_by_id(const std::string &name) {
          auto dynmon = get_cube(name);
          auto dataplaneConfig = dynmon->getDataplaneConfig();
          auto egress = dataplaneConfig->getEgressPathConfig();
          return egress->getCode();
        }

        /**
         * @brief   Read metric-configs by ID
         *
         * Read operation of resource: metric-configs*
         *
         * @param[in] name ID of name
         * @param[in] metricConfigsName ID of metric-configs_name
         *
         * Responses:
         * MetricConfigJsonObject
         */
        MetricConfigJsonObject
        read_dynmon_dataplane_config_egress_path_metric_configs_by_id(const std::string &name,
                                                                      const std::string &metricConfigsName) {
          auto dynmon = get_cube(name);
          auto dataplaneConfig = dynmon->getDataplaneConfig();
          auto egress = dataplaneConfig->getEgressPathConfig();
          return egress->getMetricConfig(metricConfigsName)->toJsonObject();
        }

        /**
         * @brief   Read extraction-options by ID
         *
         * Read operation of resource: extraction-options*
         *
         * @param[in] name ID of name
         * @param[in] metricConfigsName ID of metric-configs_name
         *
         * Responses:
         * ExtractionOptionsJsonObject
         */
        ExtractionOptionsJsonObject
        read_dynmon_dataplane_config_egress_path_metric_configs_extraction_options_by_id(const std::string &name,
                                                                                         const std::string &metricConfigsName) {
          auto dynmon = get_cube(name);
          auto dataplaneConfig = dynmon->getDataplaneConfig();
          auto egress = dataplaneConfig->getEgressPathConfig();
          auto metricConfigs = egress->getMetricConfig(metricConfigsName);
          return metricConfigs->getExtractionOptions()->toJsonObject();
        }

        /**
         * @brief   Read empty-on-read by ID
         *
         * Read operation of resource: empty-on-read*
         *
         * @param[in] name ID of name
         * @param[in] metricConfigsName ID of metric-configs_name
         *
         * Responses:
         * bool
         */
        bool
        read_dynmon_dataplane_config_egress_path_metric_configs_extraction_options_empty_on_read_by_id(
            const std::string &name, const std::string &metricConfigsName) {
          auto dynmon = get_cube(name);
          auto dataplaneConfig = dynmon->getDataplaneConfig();
          auto egress = dataplaneConfig->getEgressPathConfig();
          auto metricConfigs = egress->getMetricConfig(metricConfigsName);
          auto extractionOptions = metricConfigs->getExtractionOptions();
          return extractionOptions->getEmptyOnRead();
        }

        /**
         * @brief   Read extraction-options-on-read by ID
         *
         * Read operation of resource: extraction-options-on-read*
         *
         * @param[in] name ID of name
         * @param[in] metricConfigsName ID of metric-configs_name
         *
         * Responses:
         * bool
         */
        bool
        read_dynmon_dataplane_config_egress_path_metric_configs_extraction_options_swap_on_read_by_id(
            const std::string &name, const std::string &metricConfigsName) {
          auto dynmon = get_cube(name);
          auto dataplaneConfig = dynmon->getDataplaneConfig();
          auto egress = dataplaneConfig->getEgressPathConfig();
          auto metricConfigs = egress->getMetricConfig(metricConfigsName);
          auto extractionOptions = metricConfigs->getExtractionOptions();
          return extractionOptions->getSwapOnRead();
        }

        /**
         * @brief   Read metric-configs by ID
         *
         * Read operation of resource: metric-configs*
         *
         * @param[in] name ID of name
         *
         * Responses:
         * std::vector<MetricConfigJsonObject>
         */
        std::vector<MetricConfigJsonObject>
        read_dynmon_dataplane_config_egress_path_metric_configs_list_by_id(const std::string &name) {
          auto dynmon = get_cube(name);
          auto dataplaneConfig = dynmon->getDataplaneConfig();
          auto egress = dataplaneConfig->getEgressPathConfig();
          auto &&metricConfigs = egress->getMetricConfigsList();
          std::vector<MetricConfigJsonObject> m;
          for (auto &i : metricConfigs)
            m.push_back(i->toJsonObject());
          return m;
        }

        /**
         * @brief   Read map-name by ID
         *
         * Read operation of resource: map-name*
         *
         * @param[in] name ID of name
         * @param[in] metricConfigsName ID of metric-configs_name
         *
         * Responses:
         * std::string
         */
        std::string
        read_dynmon_dataplane_config_egress_path_metric_configs_map_name_by_id(const std::string &name,
                                                                               const std::string &metricConfigsName) {
          auto dynmon = get_cube(name);
          auto dataplaneConfig = dynmon->getDataplaneConfig();
          auto egress = dataplaneConfig->getEgressPathConfig();
          auto metricConfigs = egress->getMetricConfig(metricConfigsName);
          return metricConfigs->getMapName();
        }

        /**
         * @brief   Read open-metrics-metadata by ID
         *
         * Read operation of resource: open-metrics-metadata*
         *
         * @param[in] name ID of name
         * @param[in] metricConfigsName ID of metric-configs_name
         *
         * Responses:
         * OpenMetricsMetadataJsonObject
         */
        OpenMetricsMetadataJsonObject
        read_dynmon_dataplane_config_egress_path_metric_configs_open_metrics_metadata_by_id(const std::string &name,
                                                                                            const std::string &metricConfigsName) {
          auto dynmon = get_cube(name);
          auto dataplaneConfig = dynmon->getDataplaneConfig();
          auto egress = dataplaneConfig->getEgressPathConfig();
          auto metricConfigs = egress->getMetricConfig(metricConfigsName);
          return metricConfigs->getOpenMetricsMetadata()->toJsonObject();
        }

        /**
         * @brief   Read help by ID
         *
         * Read operation of resource: help*
         *
         * @param[in] name ID of name
         * @param[in] metricConfigsName ID of metric-configs_name
         *
         * Responses:
         * std::string
         */
        std::string
        read_dynmon_dataplane_config_egress_path_metric_configs_open_metrics_metadata_help_by_id(
            const std::string &name, const std::string &metricConfigsName) {
          auto dynmon = get_cube(name);
          auto dataplaneConfig = dynmon->getDataplaneConfig();
          auto egress = dataplaneConfig->getEgressPathConfig();
          auto metricConfigs = egress->getMetricConfig(metricConfigsName);
          auto openMetricsMetadata = metricConfigs->getOpenMetricsMetadata();
          return openMetricsMetadata->getHelp();
        }

        /**
         * @brief   Read labels by ID
         *
         * Read operation of resource: labels*
         *
         * @param[in] name ID of name
         * @param[in] metricConfigsName ID of metric-configs_name
         * @param[in] labelsName ID of labels_name
         *
         * Responses:
         * LabelJsonObject
         */
        LabelJsonObject
        read_dynmon_dataplane_config_egress_path_metric_configs_open_metrics_metadata_labels_by_id(
            const std::string &name, const std::string &metricConfigsName, const std::string &labelsName) {
          auto dynmon = get_cube(name);
          auto dataplaneConfig = dynmon->getDataplaneConfig();
          auto egress = dataplaneConfig->getEgressPathConfig();
          auto metricConfigs = egress->getMetricConfig(metricConfigsName);
          auto openMetricsMetadata = metricConfigs->getOpenMetricsMetadata();
          return openMetricsMetadata->getLabel(labelsName)->toJsonObject();
        }

        /**
         * @brief   Read labels by ID
         *
         * Read operation of resource: labels*
         *
         * @param[in] name ID of name
         * @param[in] metricConfigsName ID of metric-configs_name
         *
         * Responses:
         * std::vector<LabelJsonObject>
         */
        std::vector<LabelJsonObject>
        read_dynmon_dataplane_config_egress_path_metric_configs_open_metrics_metadata_labels_list_by_id(
            const std::string &name, const std::string &metricConfigsName) {
          auto dynmon = get_cube(name);
          auto dataplaneConfig = dynmon->getDataplaneConfig();
          auto egress = dataplaneConfig->getEgressPathConfig();
          auto metricConfigs = egress->getMetricConfig(metricConfigsName);
          auto openMetricsMetadata = metricConfigs->getOpenMetricsMetadata();
          auto &&labels = openMetricsMetadata->getLabelsList();
          std::vector<LabelJsonObject> m;
          for (auto &i : labels)
            m.push_back(i->toJsonObject());
          return m;
        }

        /**
         * @brief   Read value by ID
         *
         * Read operation of resource: value*
         *
         * @param[in] name ID of name
         * @param[in] metricConfigsName ID of metric-configs_name
         * @param[in] labelsName ID of labels_name
         *
         * Responses:
         * std::string
         */
        std::string
        read_dynmon_dataplane_config_egress_path_metric_configs_open_metrics_metadata_labels_value_by_id(
            const std::string &name, const std::string &metricConfigsName, const std::string &labelsName) {
          auto dynmon = get_cube(name);
          auto dataplaneConfig = dynmon->getDataplaneConfig();
          auto egress = dataplaneConfig->getEgressPathConfig();
          auto metricConfigs = egress->getMetricConfig(metricConfigsName);
          auto openMetricsMetadata = metricConfigs->getOpenMetricsMetadata();
          auto labels = openMetricsMetadata->getLabel(labelsName);
          return labels->getValue();
        }

        /**
         * @brief   Read type by ID
         *
         * Read operation of resource: type*
         *
         * @param[in] name ID of name
         * @param[in] metricConfigsName ID of metric-configs_name
         *
         * Responses:
         * MetricTypeEnum
         */
        MetricTypeEnum
        read_dynmon_dataplane_config_egress_path_metric_configs_open_metrics_metadata_type_by_id(
            const std::string &name, const std::string &metricConfigsName) {
          auto dynmon = get_cube(name);
          auto dataplaneConfig = dynmon->getDataplaneConfig();
          auto egress = dataplaneConfig->getEgressPathConfig();
          auto metricConfigs = egress->getMetricConfig(metricConfigsName);
          auto openMetricsMetadata = metricConfigs->getOpenMetricsMetadata();
          return openMetricsMetadata->getType();
        }

        /**
         * @brief   Read name by ID
         *
         * Read operation of resource: name*
         *
         * @param[in] name ID of name
         *
         * Responses:
         * std::string
         */
        std::string
        read_dynmon_dataplane_config_egress_path_name_by_id(const std::string &name) {
          auto dynmon = get_cube(name);
          auto dataplaneConfig = dynmon->getDataplaneConfig();
          auto egressPath = dataplaneConfig->getEgressPathConfig();
          return egressPath->getName();
        }

        /**
         * @brief   Read ingress by ID
         *
         * Read operation of resource: ingress*
         *
         * @param[in] name ID of name
         *
         * Responses:
         * PathConfigJsonObject
         */
        PathConfigJsonObject
        read_dynmon_dataplane_config_ingress_path_by_id(const std::string &name) {
          auto dynmon = get_cube(name);
          auto dataplaneConfig = dynmon->getDataplaneConfig();
          return dataplaneConfig->getIngressPathConfig()->toJsonObject();
        }

        /**
         * @brief   Read code by ID
         *
         * Read operation of resource: code*
         *
         * @param[in] name ID of name
         *
         * Responses:
         * std::string
         */
        std::string
        read_dynmon_dataplane_config_ingress_path_code_by_id(const std::string &name) {
          auto dynmon = get_cube(name);
          auto dataplaneConfig = dynmon->getDataplaneConfig();
          auto ingress = dataplaneConfig->getIngressPathConfig();
          return ingress->getCode();
        }

        /**
         * @brief   Read metric-configs by ID
         *
         * Read operation of resource: metric-configs*
         *
         * @param[in] name ID of name
         * @param[in] metricConfigsName ID of metric-configs_name
         *
         * Responses:
         * PathConfigMetricConfigsJsonObject
         */
        MetricConfigJsonObject
        read_dynmon_dataplane_config_ingress_path_metric_configs_by_id(const std::string &name,
                                                                       const std::string &metricConfigsName) {
          auto dynmon = get_cube(name);
          auto dataplaneConfig = dynmon->getDataplaneConfig();
          auto ingress = dataplaneConfig->getIngressPathConfig();
          return ingress->getMetricConfig(metricConfigsName)->toJsonObject();
        }

        /**
         * @brief   Read extraction-options by ID
         *
         * Read operation of resource: extraction-options*
         *
         * @param[in] name ID of name
         * @param[in] metricConfigsName ID of metric-configs_name
         *
         * Responses:
         * PathConfigMetricConfigsExtractionOptionsJsonObject
         */
        ExtractionOptionsJsonObject
        read_dynmon_dataplane_config_ingress_path_metric_configs_extraction_options_by_id(const std::string &name,
                                                                                          const std::string &metricConfigsName) {
          auto dynmon = get_cube(name);
          auto dataplaneConfig = dynmon->getDataplaneConfig();
          auto ingress = dataplaneConfig->getIngressPathConfig();
          auto metricConfigs = ingress->getMetricConfig(metricConfigsName);
          return metricConfigs->getExtractionOptions()->toJsonObject();
        }

        /**
         * @brief   Read empty-on-read by ID
         *
         * Read operation of resource: empty-on-read*
         *
         * @param[in] name ID of name
         * @param[in] metricConfigsName ID of metric-configs_name
         *
         * Responses:
         * bool
         */
        bool
        read_dynmon_dataplane_config_ingress_path_metric_configs_extraction_options_empty_on_read_by_id(
            const std::string &name, const std::string &metricConfigsName) {
          auto dynmon = get_cube(name);
          auto dataplaneConfig = dynmon->getDataplaneConfig();
          auto ingress = dataplaneConfig->getIngressPathConfig();
          auto metricConfigs = ingress->getMetricConfig(metricConfigsName);
          auto extractionOptions = metricConfigs->getExtractionOptions();
          return extractionOptions->getEmptyOnRead();
        }

        /**
         * @brief   Read extraction-options-on-read by ID
         *
         * Read operation of resource: extraction-options-on-read*
         *
         * @param[in] name ID of name
         * @param[in] metricConfigsName ID of metric-configs_name
         *
         * Responses:
         * bool
         */
        bool
        read_dynmon_dataplane_config_ingress_path_metric_configs_extraction_options_swap_on_read_by_id(
            const std::string &name, const std::string &metricConfigsName) {
          auto dynmon = get_cube(name);
          auto dataplaneConfig = dynmon->getDataplaneConfig();
          auto ingress = dataplaneConfig->getIngressPathConfig();
          auto metricConfigs = ingress->getMetricConfig(metricConfigsName);
          auto extractionOptions = metricConfigs->getExtractionOptions();
          return extractionOptions->getSwapOnRead();
        }

        /**
         * @brief   Read metric-configs by ID
         *
         * Read operation of resource: metric-configs*
         *
         * @param[in] name ID of name
         *
         * Responses:
         * std::vector<PathConfigMetricConfigsJsonObject>
         */
        std::vector<MetricConfigJsonObject>
        read_dynmon_dataplane_config_ingress_path_metric_configs_list_by_id(const std::string &name) {
          auto dynmon = get_cube(name);
          auto dataplaneConfig = dynmon->getDataplaneConfig();
          auto ingress = dataplaneConfig->getIngressPathConfig();
          auto &&metricConfigs = ingress->getMetricConfigsList();
          std::vector<MetricConfigJsonObject> m;
          for (auto &i : metricConfigs)
            m.push_back(i->toJsonObject());
          return m;
        }

        /**
         * @brief   Read map-name by ID
         *
         * Read operation of resource: map-name*
         *
         * @param[in] name ID of name
         * @param[in] metricConfigsName ID of metric-configs_name
         *
         * Responses:
         * std::string
         */
        std::string
        read_dynmon_dataplane_config_ingress_path_metric_configs_map_name_by_id(const std::string &name,
                                                                                const std::string &metricConfigsName) {
          auto dynmon = get_cube(name);
          auto dataplaneConfig = dynmon->getDataplaneConfig();
          auto ingress = dataplaneConfig->getIngressPathConfig();
          auto metricConfigs = ingress->getMetricConfig(metricConfigsName);
          return metricConfigs->getMapName();
        }

        /**
         * @brief   Read open-metrics-metadata by ID
         *
         * Read operation of resource: open-metrics-metadata*
         *
         * @param[in] name ID of name
         * @param[in] metricConfigsName ID of metric-configs_name
         *
         * Responses:
         * PathConfigMetricConfigsOpenMetricsMetadataJsonObject
         */
        OpenMetricsMetadataJsonObject
        read_dynmon_dataplane_config_ingress_path_metric_configs_open_metrics_metadata_by_id(const std::string &name,
                                                                                             const std::string &metricConfigsName) {
          auto dynmon = get_cube(name);
          auto dataplaneConfig = dynmon->getDataplaneConfig();
          auto ingress = dataplaneConfig->getIngressPathConfig();
          auto metricConfigs = ingress->getMetricConfig(metricConfigsName);
          auto metadata = metricConfigs->getOpenMetricsMetadata();
          if (metadata != nullptr)
            return metricConfigs->getOpenMetricsMetadata()->toJsonObject();
          else
            return nlohmann::json();
        }

        /**
         * @brief   Read help by ID
         *
         * Read operation of resource: help*
         *
         * @param[in] name ID of name
         * @param[in] metricConfigsName ID of metric-configs_name
         *
         * Responses:
         * std::string
         */
        std::string
        read_dynmon_dataplane_config_ingress_path_metric_configs_open_metrics_metadata_help_by_id(
            const std::string &name, const std::string &metricConfigsName) {
          auto dynmon = get_cube(name);
          auto dataplaneConfig = dynmon->getDataplaneConfig();
          auto ingress = dataplaneConfig->getIngressPathConfig();
          auto metricConfigs = ingress->getMetricConfig(metricConfigsName);
          auto openMetricsMetadata = metricConfigs->getOpenMetricsMetadata();
          return openMetricsMetadata->getHelp();
        }

        /**
         * @brief   Read labels by ID
         *
         * Read operation of resource: labels*
         *
         * @param[in] name ID of name
         * @param[in] metricConfigsName ID of metric-configs_name
         * @param[in] labelsName ID of labels_name
         *
         * Responses:
         * PathConfigMetricConfigsOpenMetricsMetadataLabelsJsonObject
         */
        LabelJsonObject
        read_dynmon_dataplane_config_ingress_path_metric_configs_open_metrics_metadata_labels_by_id(
            const std::string &name, const std::string &metricConfigsName, const std::string &labelsName) {
          auto dynmon = get_cube(name);
          auto dataplaneConfig = dynmon->getDataplaneConfig();
          auto ingress = dataplaneConfig->getIngressPathConfig();
          auto metricConfigs = ingress->getMetricConfig(metricConfigsName);
          auto openMetricsMetadata = metricConfigs->getOpenMetricsMetadata();
          return openMetricsMetadata->getLabel(labelsName)->toJsonObject();
        }

        /**
         * @brief   Read labels by ID
         *
         * Read operation of resource: labels*
         *
         * @param[in] name ID of name
         * @param[in] metricConfigsName ID of metric-configs_name
         *
         * Responses:
         * std::vector<PathConfigMetricConfigsOpenMetricsMetadataLabelsJsonObject>
         */
        std::vector<LabelJsonObject>
        read_dynmon_dataplane_config_ingress_path_metric_configs_open_metrics_metadata_labels_list_by_id(
            const std::string &name, const std::string &metricConfigsName) {
          auto dynmon = get_cube(name);
          auto dataplaneConfig = dynmon->getDataplaneConfig();
          auto ingress = dataplaneConfig->getIngressPathConfig();
          auto metricConfigs = ingress->getMetricConfig(metricConfigsName);
          auto openMetricsMetadata = metricConfigs->getOpenMetricsMetadata();
          auto &&labels = openMetricsMetadata->getLabelsList();
          std::vector<LabelJsonObject> m;
          for (auto &i : labels)
            m.push_back(i->toJsonObject());
          return m;
        }

        /**
         * @brief   Read value by ID
         *
         * Read operation of resource: value*
         *
         * @param[in] name ID of name
         * @param[in] metricConfigsName ID of metric-configs_name
         * @param[in] labelsName ID of labels_name
         *
         * Responses:
         * std::string
         */
        std::string
        read_dynmon_dataplane_config_ingress_path_metric_configs_open_metrics_metadata_labels_value_by_id(
            const std::string &name, const std::string &metricConfigsName, const std::string &labelsName) {
          auto dynmon = get_cube(name);
          auto dataplaneConfig = dynmon->getDataplaneConfig();
          auto ingress = dataplaneConfig->getIngressPathConfig();
          auto metricConfigs = ingress->getMetricConfig(metricConfigsName);
          auto openMetricsMetadata = metricConfigs->getOpenMetricsMetadata();
          auto labels = openMetricsMetadata->getLabel(labelsName);
          return labels->getValue();
        }

        /**
         * @brief   Read type by ID
         *
         * Read operation of resource: type*
         *
         * @param[in] name ID of name
         * @param[in] metricConfigsName ID of metric-configs_name
         *
         * Responses:
         * PathConfigMetricConfigsOpenMetricsMetadataTypeEnum
         */
        MetricTypeEnum
        read_dynmon_dataplane_config_ingress_path_metric_configs_open_metrics_metadata_type_by_id(
            const std::string &name, const std::string &metricConfigsName) {
          auto dynmon = get_cube(name);
          auto dataplaneConfig = dynmon->getDataplaneConfig();
          auto ingress = dataplaneConfig->getIngressPathConfig();
          auto metricConfigs = ingress->getMetricConfig(metricConfigsName);
          auto openMetricsMetadata = metricConfigs->getOpenMetricsMetadata();
          return openMetricsMetadata->getType();
        }

        /**
         * @brief   Read name by ID
         *
         * Read operation of resource: name*
         *
         * @param[in] name ID of name
         *
         * Responses:
         * std::string
         */
        std::string
        read_dynmon_dataplane_config_ingress_path_name_by_id(const std::string &name) {
          auto dynmon = get_cube(name);
          auto dataplaneConfig = dynmon->getDataplaneConfig();
          auto ingressPath = dataplaneConfig->getIngressPathConfig();
          return ingressPath->getName();
        }

        /**
         * @brief   Read metrics by ID
         *
         * Read operation of resource: metrics*
         *
         * @param[in] name ID of name
         *
         * Responses:
         * MetricsJsonObject
         */
        MetricsJsonObject
        read_dynmon_metrics_by_id(const std::string &name) {
          auto dynmon = get_cube(name);
          dynmon->triggerReadIngress();
          dynmon->triggerReadEgress();
          return dynmon->getMetrics()->toJsonObject();
        }

        /**
         * @brief   Read egress by ID
         *
         * Read operation of resource: egress*
         *
         * @param[in] name ID of name
         * @param[in] metricName ID of egress_name
         *
         * Responses:
         * MetricJsonObject
         */
        MetricJsonObject
        read_dynmon_metrics_egress_metrics_by_id(const std::string &name, const std::string &metricName) {
          auto dynmon = get_cube(name);
          dynmon->triggerReadEgress();
          auto metric = dynmon->getEgressMetric(metricName);
          return metric->toJsonObject();
        }

        /**
         * @brief   Read egress by ID
         *
         * Read operation of resource: egress*
         *
         * @param[in] name ID of name
         *
         * Responses:
         * std::vector<MetricJsonObject>
         */
        std::vector<MetricJsonObject>
        read_dynmon_metrics_egress_metrics_list_by_id(const std::string &name) {
          auto dynmon = get_cube(name);
          dynmon->triggerReadEgress();
          auto metrics = dynmon->getEgressMetrics();
          std::vector<MetricJsonObject> m;
          for (auto &i : metrics)
            m.push_back(i->toJsonObject());
          return m;
        }

        /**
         * @brief   Read timestamp by ID
         *
         * Read operation of resource: timestamp*
         *
         * @param[in] name ID of name
         * @param[in] metricName ID of egress_name
         *
         * Responses:
         * int64_t
         */
        int64_t
        read_dynmon_metrics_egress_metrics_timestamp_by_id(const std::string &name, const std::string &metricName) {
          auto dynmon = get_cube(name);
          dynmon->triggerReadEgress();
          auto metric = dynmon->getEgressMetric(metricName);
          return metric->getTimestamp();
        }

        /**
         * @brief   Read value by ID
         *
         * Read operation of resource: value*
         *
         * @param[in] name ID of name
         * @param[in] egressName ID of egress_name
         *
         * Responses:
         * nlohmann::json
         */
        nlohmann::json
        read_dynmon_metrics_egress_metrics_value_by_id(const std::string &name, const std::string &egressName) {
          auto dynmon = get_cube(name);
          dynmon->triggerReadEgress();
          auto metric = dynmon->getEgressMetric(egressName);
          return metric->getValue();
        }

        /**
         * @brief   Read ingress by ID
         *
         * Read operation of resource: ingress*
         *
         * @param[in] name ID of name
         * @param[in] metricName ID of ingress_name
         *
         * Responses:
         * MetricJsonObject
         */
        MetricJsonObject
        read_dynmon_metrics_ingress_metrics_by_id(const std::string &name, const std::string &metricName) {
          auto dynmon = get_cube(name);
          dynmon->triggerReadIngress();
          auto metric = dynmon->getIngressMetric(metricName);
          return metric->toJsonObject();
        }

        /**
         * @brief   Read ingress by ID
         *
         * Read operation of resource: ingress*
         *
         * @param[in] name ID of name
         *
         * Responses:
         * std::vector<MetricJsonObject>
         */
        std::vector<MetricJsonObject>
        read_dynmon_metrics_ingress_metrics_list_by_id(const std::string &name) {
          auto dynmon = get_cube(name);
          dynmon->triggerReadIngress();
          auto metrics = dynmon->getIngressMetrics();
          std::vector<MetricJsonObject> m{};
          for (auto &i : metrics)
            m.push_back(i->toJsonObject());
          return m;
        }

        /**
         * @brief   Read timestamp by ID
         *
         * Read operation of resource: timestamp*
         *
         * @param[in] name ID of name
         * @param[in] ingressName ID of ingress_name
         *
         * Responses:
         * int64_t
         */
        int64_t
        read_dynmon_metrics_ingress_metrics_timestamp_by_id(const std::string &name, const std::string &ingressName) {
          auto dynmon = get_cube(name);
          dynmon->triggerReadIngress();
          auto metric = dynmon->getIngressMetric(ingressName);
          return metric->getTimestamp();
        }

        /**
         * @brief   Read value by ID
         *
         * Read operation of resource: value*
         *
         * @param[in] name ID of name
         * @param[in] ingressName ID of ingress_name
         *
         * Responses:
         * nlohmann::json
         */
        nlohmann::json
        read_dynmon_metrics_ingress_metrics_value_by_id(const std::string &name, const std::string &ingressName) {
          auto dynmon = get_cube(name);
          dynmon->triggerReadIngress();
          auto metric = dynmon->getIngressMetric(ingressName);
          return metric->getValue();
        }

        /**
         * @brief   Read open-metrics by ID
         *
         * Read operation of resource: open-metrics*
         *
         * @param[in] name ID of name
         *
         * Responses:
         * std::string
         */
        std::string
        read_dynmon_open_metrics_by_id(const std::string &name) {
          auto dynmon = get_cube(name);
          return dynmon->getOpenMetrics();
        }

        /**
         * @brief   Replace dataplane-config by ID
         *
         * Replace operation of resource: dataplane-config*
         *
         * @param[in] name ID of name
         * @param[in] value dataplane-configbody object
         *
         * Responses:
         *
         */
        void
        replace_dynmon_dataplane_config_by_id(const std::string &name, const DataplaneConfigJsonObject &value) {
          auto dynmon = get_cube(name);
          return dynmon->setDataplaneConfig(value);
        }

        /**
         * @brief   Replace egress by ID
         *
         * Replace operation of resource: egress*
         *
         * @param[in] name ID of name
         * @param[in] value egressbody object
         *
         * Responses:
         *
         */
        void
        replace_dynmon_dataplane_config_egress_path_by_id(const std::string &name,
                                                          const PathConfigJsonObject &value) {
          auto dynmon = get_cube(name);
          return dynmon->setEgressPathConfig(value);
        }

        /**
         * @brief   Replace ingress by ID
         *
         * Replace operation of resource: ingress*
         *
         * @param[in] name ID of name
         * @param[in] value ingressbody object
         *
         * Responses:
         *
         */
        void
        replace_dynmon_dataplane_config_ingress_path_by_id(const std::string &name,
                                                           const PathConfigJsonObject &value) {
          auto dynmon = get_cube(name);
          return dynmon->setIngressPathConfig(value);
        }

        /**
         * @brief   Update dynmon by ID
         *
         * Update operation of resource: dynmon*
         *
         * @param[in] name ID of name
         * @param[in] value dynmonbody object
         *
         * Responses:
         *
         */
        void
        update_dynmon_by_id(const std::string &name, const DynmonJsonObject &value) {
          auto dynmon = get_cube(name);
          return dynmon->update(value);
        }

        /**
         * @brief   Update dataplane-config by ID
         *
         * Update operation of resource: dataplane-config*
         *
         * @param[in] name ID of name
         * @param[in] value dataplane-configbody object
         *
         * Responses:
         *
         */
        void
        update_dynmon_dataplane_config_by_id(const std::string &name, const DataplaneConfigJsonObject &value) {
          auto dynmon = get_cube(name);
          return dynmon->setDataplaneConfig(value);
        }

        /**
         * @brief   Update egress by ID
         *
         * Update operation of resource: egress*
         *
         * @param[in] name ID of name
         * @param[in] value egressbody object
         *
         * Responses:
         *
         */
        void
        update_dynmon_dataplane_config_egress_path_by_id(const std::string &name,
                                                         const PathConfigJsonObject &value) {
          auto dynmon = get_cube(name);
          return dynmon->setEgressPathConfig(value);
        }

        /**
         * @brief   Update name by ID
         *
         * Update operation of resource: name*
         *
         * @param[in] name ID of name
         * @param[in] value Configuration name
         *
         * Responses:
         *
         */
        void
        update_dynmon_dataplane_config_egress_path_name_by_id(const std::string &name, const std::string &value) {
          throw std::runtime_error("Method not allowed");
        }

        /**
         * @brief   Update ingress by ID
         *
         * Update operation of resource: ingress*
         *
         * @param[in] name ID of name
         * @param[in] value ingressbody object
         *
         * Responses:
         *
         */
        void
        update_dynmon_dataplane_config_ingress_path_by_id(const std::string &name,
                                                          const PathConfigJsonObject &value) {
          auto dynmon = get_cube(name);
          return dynmon->setIngressPathConfig(value);
        }

        /**
         * @brief   Update name by ID
         *
         * Update operation of resource: name*
         *
         * @param[in] name ID of name
         * @param[in] value Configuration name
         *
         * Responses:
         *
         */
        void
        update_dynmon_dataplane_config_ingress_path_name_by_id(const std::string &name, const std::string &value) {
          throw std::runtime_error("Method not allowed");
        }

        /**
         * @brief   Update dynmon by ID
         *
         * Update operation of resource: dynmon*
         *
         * @param[in] value dynmonbody object
         *
         * Responses:
         *
         */
        void
        update_dynmon_list_by_id(const std::vector<DynmonJsonObject> &value) {
          throw std::runtime_error("Method not supported");
        }

        /**
         * help related
         */
        std::vector<nlohmann::fifo_map<std::string, std::string>>
        read_dynmon_dataplane_config_egress_path_metric_configs_list_by_id_get_list(const std::string &name) {
          std::vector<nlohmann::fifo_map<std::string, std::string>> r;
          auto &&dynmon = get_cube(name);
          auto &&dataplaneConfig = dynmon->getDataplaneConfig();
          auto &&egress = dataplaneConfig->getEgressPathConfig();
          auto &&metricConfigs = egress->getMetricConfigsList();
          for (auto &i : metricConfigs) {
            nlohmann::fifo_map<std::string, std::string> keys;
            keys["name"] = i->getName();
            r.push_back(keys);
          }
          return r;
        }

        std::vector<nlohmann::fifo_map<std::string, std::string>>
        read_dynmon_dataplane_config_egress_path_metric_configs_open_metrics_metadata_labels_list_by_id_get_list(
            const std::string &name, const std::string &metricConfigsName) {
          std::vector<nlohmann::fifo_map<std::string, std::string>> r;
          auto &&dynmon = get_cube(name);
          auto &&dataplaneConfig = dynmon->getDataplaneConfig();
          auto &&egress = dataplaneConfig->getEgressPathConfig();
          auto &&metricConfigs = egress->getMetricConfig(metricConfigsName);
          auto &&openMetricsMetadata = metricConfigs->getOpenMetricsMetadata();
          auto &&labels = openMetricsMetadata->getLabelsList();
          for (auto &i : labels) {
            nlohmann::fifo_map<std::string, std::string> keys;
            keys["name"] = i->getName();
            r.push_back(keys);
          }
          return r;
        }

        std::vector<nlohmann::fifo_map<std::string, std::string>>
        read_dynmon_dataplane_config_ingress_path_metric_configs_list_by_id_get_list(const std::string &name) {
          std::vector<nlohmann::fifo_map<std::string, std::string>> r;
          auto &&dynmon = get_cube(name);
          auto &&dataplaneConfig = dynmon->getDataplaneConfig();
          auto &&ingress = dataplaneConfig->getIngressPathConfig();
          auto &&metricConfigs = ingress->getMetricConfigsList();
          for (auto &i : metricConfigs) {
            nlohmann::fifo_map<std::string, std::string> keys;
            keys["name"] = i->getName();
            r.push_back(keys);
          }
          return r;
        }

        std::vector<nlohmann::fifo_map<std::string, std::string>>
        read_dynmon_dataplane_config_ingress_path_metric_configs_open_metrics_metadata_labels_list_by_id_get_list(
            const std::string &name, const std::string &metricConfigsName) {
          std::vector<nlohmann::fifo_map<std::string, std::string>> r;
          auto &&dynmon = get_cube(name);
          auto &&dataplaneConfig = dynmon->getDataplaneConfig();
          auto &&ingress = dataplaneConfig->getIngressPathConfig();
          auto &&metricConfigs = ingress->getMetricConfig(metricConfigsName);
          auto &&openMetricsMetadata = metricConfigs->getOpenMetricsMetadata();
          auto &&labels = openMetricsMetadata->getLabelsList();
          for (auto &i : labels) {
            nlohmann::fifo_map<std::string, std::string> keys;
            keys["name"] = i->getName();
            r.push_back(keys);
          }
          return r;
        }

        std::vector<nlohmann::fifo_map<std::string, std::string>>
        read_dynmon_metrics_egress_metrics_list_by_id_get_list(const std::string &name) {
          std::vector<nlohmann::fifo_map<std::string, std::string>> r;
          auto &&dynmon = get_cube(name);
          auto config = dynmon->getDataplaneConfig();
          if (config == nullptr)
            return {};
          auto eg_config = config->getEgressPathConfig();
          if (eg_config == nullptr)
            return {};
          auto metrics = eg_config->getMetricConfigsList();
          for (auto &i : metrics) {
            nlohmann::fifo_map<std::string, std::string> keys;
            keys["name"] = i->getName();
            r.push_back(keys);
          }
          return r;
        }

        std::vector<nlohmann::fifo_map<std::string, std::string>>
        read_dynmon_metrics_ingress_metrics_list_by_id_get_list(const std::string &name) {
          std::vector<nlohmann::fifo_map<std::string, std::string>> r;
          auto &&dynmon = get_cube(name);
          auto config = dynmon->getDataplaneConfig();
          if (config == nullptr)
            return {};
          auto in_config = config->getIngressPathConfig();
          if (in_config == nullptr)
            return {};
          auto metrics = in_config->getMetricConfigsList();
          for (auto &i : metrics) {
            nlohmann::fifo_map<std::string, std::string> keys;
            keys["name"] = i->getName();
            r.push_back(keys);
          }
          return r;
        }
      }// namespace DynmonApiImpl
    }// namespace api
  }// namespace service
}// namespace polycube
