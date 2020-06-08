#pragma once
#define POLYCUBE_SERVICE_NAME "dynmon"

#include "polycube/services/response.h"
#include "polycube/services/shared_lib_elements.h"

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

#ifdef __cplusplus
extern "C" {
#endif

Response create_dynmon_by_id_handler(const char *name, const Key *keys, size_t num_keys, const char *value);
Response create_dynmon_dataplane_config_by_id_handler(const char *name, const Key *keys, size_t num_keys, const char *value);
Response create_dynmon_dataplane_config_egress_path_by_id_handler(const char *name, const Key *keys, size_t num_keys, const char *value);
Response create_dynmon_dataplane_config_ingress_path_by_id_handler(const char *name, const Key *keys, size_t num_keys, const char *value);

Response delete_dynmon_by_id_handler(const char *name, const Key *keys, size_t num_keys);
Response delete_dynmon_dataplane_config_by_id_handler(const char *name, const Key *keys, size_t num_keys);
Response delete_dynmon_dataplane_config_egress_path_by_id_handler(const char *name, const Key *keys, size_t num_keys);
Response delete_dynmon_dataplane_config_ingress_path_by_id_handler(const char *name, const Key *keys, size_t num_keys);

Response read_dynmon_by_id_handler(const char *name, const Key *keys, size_t num_keys);
Response read_dynmon_dataplane_config_by_id_handler(const char *name, const Key *keys, size_t num_keys);
Response read_dynmon_dataplane_config_egress_path_by_id_handler(const char *name, const Key *keys, size_t num_keys);
Response read_dynmon_dataplane_config_egress_path_code_by_id_handler(const char *name, const Key *keys, size_t num_keys);
Response read_dynmon_dataplane_config_egress_path_metric_configs_by_id_handler(const char *name, const Key *keys, size_t num_keys);
Response read_dynmon_dataplane_config_egress_path_metric_configs_extraction_options_by_id_handler(const char *name, const Key *keys, size_t num_keys);
Response read_dynmon_dataplane_config_egress_path_metric_configs_extraction_options_empty_on_read_by_id_handler(const char *name, const Key *keys, size_t num_keys);
Response read_dynmon_dataplane_config_egress_path_metric_configs_extraction_options_swap_on_read_by_id_handler(const char *name, const Key *keys, size_t num_keys);
Response read_dynmon_dataplane_config_egress_path_metric_configs_list_by_id_handler(const char *name, const Key *keys, size_t num_keys);
Response read_dynmon_dataplane_config_egress_path_metric_configs_map_name_by_id_handler(const char *name, const Key *keys, size_t num_keys);
Response read_dynmon_dataplane_config_egress_path_metric_configs_open_metrics_metadata_by_id_handler(const char *name, const Key *keys, size_t num_keys);
Response read_dynmon_dataplane_config_egress_path_metric_configs_open_metrics_metadata_help_by_id_handler(const char *name, const Key *keys, size_t num_keys);
Response read_dynmon_dataplane_config_egress_path_metric_configs_open_metrics_metadata_labels_by_id_handler(const char *name, const Key *keys, size_t num_keys);
Response read_dynmon_dataplane_config_egress_path_metric_configs_open_metrics_metadata_labels_list_by_id_handler(const char *name, const Key *keys, size_t num_keys);
Response read_dynmon_dataplane_config_egress_path_metric_configs_open_metrics_metadata_labels_value_by_id_handler(const char *name, const Key *keys, size_t num_keys);
Response read_dynmon_dataplane_config_egress_path_metric_configs_open_metrics_metadata_type_by_id_handler(const char *name, const Key *keys, size_t num_keys);
Response read_dynmon_dataplane_config_egress_path_name_by_id_handler(const char *name, const Key *keys, size_t num_keys);
Response read_dynmon_dataplane_config_ingress_path_by_id_handler(const char *name, const Key *keys, size_t num_keys);
Response read_dynmon_dataplane_config_ingress_path_code_by_id_handler(const char *name, const Key *keys, size_t num_keys);
Response read_dynmon_dataplane_config_ingress_path_metric_configs_by_id_handler(const char *name, const Key *keys, size_t num_keys);
Response read_dynmon_dataplane_config_ingress_path_metric_configs_extraction_options_by_id_handler(const char *name, const Key *keys, size_t num_keys);
Response read_dynmon_dataplane_config_ingress_path_metric_configs_extraction_options_empty_on_read_by_id_handler(const char *name, const Key *keys, size_t num_keys);
Response read_dynmon_dataplane_config_ingress_path_metric_configs_extraction_options_swap_on_read_by_id_handler(const char *name, const Key *keys, size_t num_keys);
Response read_dynmon_dataplane_config_ingress_path_metric_configs_list_by_id_handler(const char *name, const Key *keys, size_t num_keys);
Response read_dynmon_dataplane_config_ingress_path_metric_configs_map_name_by_id_handler(const char *name, const Key *keys, size_t num_keys);
Response read_dynmon_dataplane_config_ingress_path_metric_configs_open_metrics_metadata_by_id_handler(const char *name, const Key *keys, size_t num_keys);
Response read_dynmon_dataplane_config_ingress_path_metric_configs_open_metrics_metadata_help_by_id_handler(const char *name, const Key *keys, size_t num_keys);
Response read_dynmon_dataplane_config_ingress_path_metric_configs_open_metrics_metadata_labels_by_id_handler(const char *name, const Key *keys, size_t num_keys);
Response read_dynmon_dataplane_config_ingress_path_metric_configs_open_metrics_metadata_labels_list_by_id_handler(const char *name, const Key *keys, size_t num_keys);
Response read_dynmon_dataplane_config_ingress_path_metric_configs_open_metrics_metadata_labels_value_by_id_handler(const char *name, const Key *keys, size_t num_keys);
Response read_dynmon_dataplane_config_ingress_path_metric_configs_open_metrics_metadata_type_by_id_handler(const char *name, const Key *keys, size_t num_keys);
Response read_dynmon_dataplane_config_ingress_path_name_by_id_handler(const char *name, const Key *keys, size_t num_keys);
Response read_dynmon_list_by_id_handler(const char *name, const Key *keys, size_t num_keys);
Response read_dynmon_metrics_by_id_handler(const char *name, const Key *keys, size_t num_keys);
Response read_dynmon_metrics_egress_metrics_by_id_handler(const char *name, const Key *keys, size_t num_keys);
Response read_dynmon_metrics_egress_metrics_list_by_id_handler(const char *name, const Key *keys, size_t num_keys);
Response read_dynmon_metrics_egress_metrics_timestamp_by_id_handler(const char *name, const Key *keys, size_t num_keys);
Response read_dynmon_metrics_egress_metrics_value_by_id_handler(const char *name, const Key *keys, size_t num_keys);
Response read_dynmon_metrics_ingress_metrics_by_id_handler(const char *name, const Key *keys, size_t num_keys);
Response read_dynmon_metrics_ingress_metrics_list_by_id_handler(const char *name, const Key *keys, size_t num_keys);
Response read_dynmon_metrics_ingress_metrics_timestamp_by_id_handler(const char *name, const Key *keys, size_t num_keys);
Response read_dynmon_metrics_ingress_metrics_value_by_id_handler(const char *name, const Key *keys, size_t num_keys);
Response read_dynmon_open_metrics_by_id_handler(const char *name, const Key *keys, size_t num_keys);

Response replace_dynmon_by_id_handler(const char *name, const Key *keys, size_t num_keys, const char *value);
Response replace_dynmon_dataplane_config_by_id_handler(const char *name, const Key *keys, size_t num_keys, const char *value);
Response replace_dynmon_dataplane_config_egress_path_by_id_handler(const char *name, const Key *keys, size_t num_keys, const char *value);
Response replace_dynmon_dataplane_config_ingress_path_by_id_handler(const char *name, const Key *keys, size_t num_keys, const char *value);

Response update_dynmon_by_id_handler(const char *name, const Key *keys, size_t num_keys, const char *value);
Response update_dynmon_dataplane_config_by_id_handler(const char *name, const Key *keys, size_t num_keys, const char *value);
Response update_dynmon_dataplane_config_egress_path_by_id_handler(const char *name, const Key *keys, size_t num_keys, const char *value);
Response update_dynmon_dataplane_config_egress_path_name_by_id_handler(const char *name, const Key *keys, size_t num_keys, const char *value);
Response update_dynmon_dataplane_config_ingress_path_by_id_handler(const char *name, const Key *keys, size_t num_keys, const char *value);
Response update_dynmon_dataplane_config_ingress_path_name_by_id_handler(const char *name, const Key *keys, size_t num_keys, const char *value);
Response update_dynmon_list_by_id_handler(const char *name, const Key *keys, size_t num_keys, const char *value);

Response dynmon_dataplane_config_egress_path_metric_configs_list_by_id_help(const char *name, const Key *keys, size_t num_keys);
Response dynmon_dataplane_config_egress_path_metric_configs_open_metrics_metadata_labels_list_by_id_help(const char *name, const Key *keys, size_t num_keys);
Response dynmon_dataplane_config_ingress_path_metric_configs_list_by_id_help(const char *name, const Key *keys, size_t num_keys);
Response dynmon_dataplane_config_ingress_path_metric_configs_open_metrics_metadata_labels_list_by_id_help(const char *name, const Key *keys, size_t num_keys);
Response dynmon_list_by_id_help(const char *name, const Key *keys, size_t num_keys);
Response dynmon_metrics_egress_metrics_list_by_id_help(const char *name, const Key *keys, size_t num_keys);
Response dynmon_metrics_ingress_metrics_list_by_id_help(const char *name, const Key *keys, size_t num_keys);

#ifdef __cplusplus
}
#endif
