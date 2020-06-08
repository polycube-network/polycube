#pragma once

#include "../Dynmon.h"
#include <map>
#include <memory>
#include <mutex>

#include "DataplaneConfigJsonObject.h"
#include "DynmonJsonObject.h"
#include "ExtractionOptionsJsonObject.h"
#include "LabelJsonObject.h"
#include "MetricConfigJsonObject.h"
#include "MetricJsonObject.h"
#include "MetricsJsonObject.h"
#include "OpenMetricsMetadataJsonObject.h"
#include "PathConfigJsonObject.h"
#include <vector>

namespace polycube {
  namespace service {
    namespace api {
      using namespace polycube::service::model;
      namespace DynmonApiImpl {
        void create_dynmon_by_id(const std::string &name, const DynmonJsonObject &value);
        void create_dynmon_dataplane_config_by_id(const std::string &name, const DataplaneConfigJsonObject &value);
        void create_dynmon_dataplane_config_egress_path_by_id(const std::string &name, const PathConfigJsonObject &value);
        void create_dynmon_dataplane_config_ingress_path_by_id(const std::string &name, const PathConfigJsonObject &value);
        void delete_dynmon_by_id(const std::string &name);
        void delete_dynmon_dataplane_config_by_id(const std::string &name);
        void delete_dynmon_dataplane_config_egress_path_by_id(const std::string &name);
        void delete_dynmon_dataplane_config_ingress_path_by_id(const std::string &name);
        DynmonJsonObject read_dynmon_by_id(const std::string &name);
        DataplaneConfigJsonObject read_dynmon_dataplane_config_by_id(const std::string &name);
        PathConfigJsonObject read_dynmon_dataplane_config_egress_path_by_id(const std::string &name);
        std::string read_dynmon_dataplane_config_egress_path_code_by_id(const std::string &name);
        MetricConfigJsonObject read_dynmon_dataplane_config_egress_path_metric_configs_by_id(const std::string &name, const std::string &metricConfigsName);
        ExtractionOptionsJsonObject read_dynmon_dataplane_config_egress_path_metric_configs_extraction_options_by_id(const std::string &name, const std::string &metricConfigsName);
        bool read_dynmon_dataplane_config_egress_path_metric_configs_extraction_options_empty_on_read_by_id(const std::string &name, const std::string &metricConfigsName);
        bool read_dynmon_dataplane_config_egress_path_metric_configs_extraction_options_swap_on_read_by_id(const std::string &name, const std::string &metricConfigsName);
        std::vector<MetricConfigJsonObject> read_dynmon_dataplane_config_egress_path_metric_configs_list_by_id(const std::string &name);
        std::string read_dynmon_dataplane_config_egress_path_metric_configs_map_name_by_id(const std::string &name, const std::string &metricConfigsName);
        OpenMetricsMetadataJsonObject read_dynmon_dataplane_config_egress_path_metric_configs_open_metrics_metadata_by_id(const std::string &name, const std::string &metricConfigsName);
        std::string read_dynmon_dataplane_config_egress_path_metric_configs_open_metrics_metadata_help_by_id(const std::string &name, const std::string &metricConfigsName);
        LabelJsonObject read_dynmon_dataplane_config_egress_path_metric_configs_open_metrics_metadata_labels_by_id(const std::string &name, const std::string &metricConfigsName, const std::string &labelsName);
        std::vector<LabelJsonObject> read_dynmon_dataplane_config_egress_path_metric_configs_open_metrics_metadata_labels_list_by_id(const std::string &name, const std::string &metricConfigsName);
        std::string read_dynmon_dataplane_config_egress_path_metric_configs_open_metrics_metadata_labels_value_by_id(const std::string &name, const std::string &metricConfigsName, const std::string &labelsName);
        MetricTypeEnum read_dynmon_dataplane_config_egress_path_metric_configs_open_metrics_metadata_type_by_id(const std::string &name, const std::string &metricConfigsName);
        std::string read_dynmon_dataplane_config_egress_path_name_by_id(const std::string &name);
        PathConfigJsonObject read_dynmon_dataplane_config_ingress_path_by_id(const std::string &name);
        std::string read_dynmon_dataplane_config_ingress_path_code_by_id(const std::string &name);
        MetricConfigJsonObject read_dynmon_dataplane_config_ingress_path_metric_configs_by_id(const std::string &name, const std::string &metricConfigsName);
        ExtractionOptionsJsonObject read_dynmon_dataplane_config_ingress_path_metric_configs_extraction_options_by_id(const std::string &name, const std::string &metricConfigsName);
        bool read_dynmon_dataplane_config_ingress_path_metric_configs_extraction_options_empty_on_read_by_id(const std::string &name, const std::string &metricConfigsName);
        bool read_dynmon_dataplane_config_ingress_path_metric_configs_extraction_options_swap_on_read_by_id(const std::string &name, const std::string &metricConfigsName);
        std::vector<MetricConfigJsonObject> read_dynmon_dataplane_config_ingress_path_metric_configs_list_by_id(const std::string &name);
        std::string read_dynmon_dataplane_config_ingress_path_metric_configs_map_name_by_id(const std::string &name, const std::string &metricConfigsName);
        OpenMetricsMetadataJsonObject read_dynmon_dataplane_config_ingress_path_metric_configs_open_metrics_metadata_by_id(const std::string &name, const std::string &metricConfigsName);
        std::string read_dynmon_dataplane_config_ingress_path_metric_configs_open_metrics_metadata_help_by_id(const std::string &name, const std::string &metricConfigsName);
        LabelJsonObject read_dynmon_dataplane_config_ingress_path_metric_configs_open_metrics_metadata_labels_by_id(const std::string &name, const std::string &metricConfigsName, const std::string &labelsName);
        std::vector<LabelJsonObject> read_dynmon_dataplane_config_ingress_path_metric_configs_open_metrics_metadata_labels_list_by_id(const std::string &name, const std::string &metricConfigsName);
        std::string read_dynmon_dataplane_config_ingress_path_metric_configs_open_metrics_metadata_labels_value_by_id(const std::string &name, const std::string &metricConfigsName, const std::string &labelsName);
        MetricTypeEnum read_dynmon_dataplane_config_ingress_path_metric_configs_open_metrics_metadata_type_by_id(const std::string &name, const std::string &metricConfigsName);
        std::string read_dynmon_dataplane_config_ingress_path_name_by_id(const std::string &name);
        std::vector<DynmonJsonObject> read_dynmon_list_by_id();
        MetricsJsonObject read_dynmon_metrics_by_id(const std::string &name);
        MetricJsonObject read_dynmon_metrics_egress_metrics_by_id(const std::string &name, const std::string &metricName);
        std::vector<MetricJsonObject> read_dynmon_metrics_egress_metrics_list_by_id(const std::string &name);
        int64_t read_dynmon_metrics_egress_metrics_timestamp_by_id(const std::string &name, const std::string &metricName);
        nlohmann::json read_dynmon_metrics_egress_metrics_value_by_id(const std::string &name, const std::string &egressName);
        MetricJsonObject read_dynmon_metrics_ingress_metrics_by_id(const std::string &name, const std::string &metricName);
        std::vector<MetricJsonObject> read_dynmon_metrics_ingress_metrics_list_by_id(const std::string &name);
        int64_t read_dynmon_metrics_ingress_metrics_timestamp_by_id(const std::string &name, const std::string &ingressName);
        nlohmann::json read_dynmon_metrics_ingress_metrics_value_by_id(const std::string &name, const std::string &ingressName);
        std::string read_dynmon_open_metrics_by_id(const std::string &name);
        void replace_dynmon_by_id(const std::string &name, const DynmonJsonObject &value);
        void replace_dynmon_dataplane_config_by_id(const std::string &name, const DataplaneConfigJsonObject &value);
        void replace_dynmon_dataplane_config_egress_path_by_id(const std::string &name, const PathConfigJsonObject &value);
        void replace_dynmon_dataplane_config_ingress_path_by_id(const std::string &name, const PathConfigJsonObject &value);
        void update_dynmon_by_id(const std::string &name, const DynmonJsonObject &value);
        void update_dynmon_dataplane_config_by_id(const std::string &name, const DataplaneConfigJsonObject &value);
        void update_dynmon_dataplane_config_egress_path_by_id(const std::string &name, const PathConfigJsonObject &value);
        void update_dynmon_dataplane_config_egress_path_name_by_id(const std::string &name, const std::string &value);
        void update_dynmon_dataplane_config_ingress_path_by_id(const std::string &name, const PathConfigJsonObject &value);
        void update_dynmon_dataplane_config_ingress_path_name_by_id(const std::string &name, const std::string &value);
        void update_dynmon_list_by_id(const std::vector<DynmonJsonObject> &value);

        /* help related */
        std::vector<nlohmann::fifo_map<std::string, std::string>> read_dynmon_dataplane_config_egress_path_metric_configs_list_by_id_get_list(const std::string &name);
        std::vector<nlohmann::fifo_map<std::string, std::string>> read_dynmon_dataplane_config_egress_path_metric_configs_open_metrics_metadata_labels_list_by_id_get_list(const std::string &name, const std::string &metricConfigsName);
        std::vector<nlohmann::fifo_map<std::string, std::string>> read_dynmon_dataplane_config_ingress_path_metric_configs_list_by_id_get_list(const std::string &name);
        std::vector<nlohmann::fifo_map<std::string, std::string>> read_dynmon_dataplane_config_ingress_path_metric_configs_open_metrics_metadata_labels_list_by_id_get_list(const std::string &name, const std::string &metricConfigsName);
        std::vector<nlohmann::fifo_map<std::string, std::string>> read_dynmon_list_by_id_get_list();
        std::vector<nlohmann::fifo_map<std::string, std::string>> read_dynmon_metrics_egress_metrics_list_by_id_get_list(const std::string &name);
        std::vector<nlohmann::fifo_map<std::string, std::string>> read_dynmon_metrics_ingress_metrics_list_by_id_get_list(const std::string &name);
      }// namespace DynmonApiImpl
    }// namespace api
  }// namespace service
}// namespace polycube
