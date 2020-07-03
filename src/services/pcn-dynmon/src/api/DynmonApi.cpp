#include "DynmonApi.h"
#include "DynmonApiImpl.h"

using namespace polycube::service::model;
using namespace polycube::service::api::DynmonApiImpl;

#ifdef __cplusplus
extern "C" {
#endif

Response create_dynmon_by_id_handler(
    const char *name, const Key *keys,
    size_t num_keys,
    const char *value) {
  // Getting the path params
  std::string unique_name{name};
  try {
    auto request_body = nlohmann::json::parse(std::string{value});
    // Getting the body param
    DynmonJsonObject unique_value{request_body};
    unique_value.setName(unique_name);
    create_dynmon_by_id(unique_name, unique_value);
    return {kCreated, nullptr};
  } catch (const std::exception &e) {
    return {kGenericError, ::strdup(e.what())};
  }
}

Response create_dynmon_dataplane_config_by_id_handler(
    const char *name, const Key *keys,
    size_t num_keys,
    const char *value) {
  // Getting the path params
  std::string unique_name{name};
  try {
    auto request_body = nlohmann::json::parse(std::string{value});
    // Getting the body param
    DataplaneConfigJsonObject unique_value{request_body};
    create_dynmon_dataplane_config_by_id(unique_name, unique_value);
    return {kCreated, nullptr};
  } catch (const std::exception &e) {
    return {kGenericError, ::strdup(e.what())};
  }
}

Response create_dynmon_dataplane_config_egress_path_by_id_handler(
    const char *name, const Key *keys,
    size_t num_keys,
    const char *value) {
  // Getting the path params
  std::string unique_name{name};
  try {
    auto request_body = nlohmann::json::parse(std::string{value});
    // Getting the body param
    PathConfigJsonObject unique_value{request_body};
    create_dynmon_dataplane_config_egress_path_by_id(unique_name, unique_value);
    return {kCreated, nullptr};
  } catch (const std::exception &e) {
    return {kGenericError, ::strdup(e.what())};
  }
}

Response create_dynmon_dataplane_config_ingress_path_by_id_handler(
    const char *name, const Key *keys,
    size_t num_keys,
    const char *value) {
  // Getting the path params
  std::string unique_name{name};
  try {
    auto request_body = nlohmann::json::parse(std::string{value});
    // Getting the body param
    PathConfigJsonObject unique_value{request_body};
    create_dynmon_dataplane_config_ingress_path_by_id(unique_name, unique_value);
    return {kCreated, nullptr};
  } catch (const std::exception &e) {
    return {kGenericError, ::strdup(e.what())};
  }
}

Response delete_dynmon_by_id_handler(
    const char *name, const Key *keys,
    size_t num_keys) {
  // Getting the path params
  std::string unique_name{name};
  try {
    delete_dynmon_by_id(unique_name);
    return {kOk, nullptr};
  } catch (const std::exception &e) {
    return {kGenericError, ::strdup(e.what())};
  }
}

Response delete_dynmon_dataplane_config_by_id_handler(
    const char *name, const Key *keys,
    size_t num_keys) {
  // Getting the path params
  std::string unique_name{name};
  try {
    delete_dynmon_dataplane_config_by_id(unique_name);
    return {kOk, nullptr};
  } catch (const std::exception &e) {
    return {kGenericError, ::strdup(e.what())};
  }
}

Response delete_dynmon_dataplane_config_egress_path_by_id_handler(
    const char *name, const Key *keys,
    size_t num_keys) {
  // Getting the path params
  std::string unique_name{name};
  try {
    delete_dynmon_dataplane_config_egress_path_by_id(unique_name);
    return {kOk, nullptr};
  } catch (const std::exception &e) {
    return {kGenericError, ::strdup(e.what())};
  }
}

Response delete_dynmon_dataplane_config_ingress_path_by_id_handler(
    const char *name, const Key *keys,
    size_t num_keys) {
  // Getting the path params
  std::string unique_name{name};
  try {
    delete_dynmon_dataplane_config_ingress_path_by_id(unique_name);
    return {kOk, nullptr};
  } catch (const std::exception &e) {
    return {kGenericError, ::strdup(e.what())};
  }
}

Response read_dynmon_by_id_handler(
    const char *name, const Key *keys,
    size_t num_keys) {
  // Getting the path params
  std::string unique_name{name};
  try {
    auto x = read_dynmon_by_id(unique_name);
    nlohmann::json response_body;
    response_body = x.toJson();
    return {kOk, ::strdup(response_body.dump().c_str())};
  } catch (const std::exception &e) {
    return {kGenericError, ::strdup(e.what())};
  }
}

Response read_dynmon_dataplane_config_by_id_handler(
    const char *name, const Key *keys,
    size_t num_keys) {
  // Getting the path params
  std::string unique_name{name};
  try {
    auto x = read_dynmon_dataplane_config_by_id(unique_name);
    nlohmann::json response_body;
    response_body = x.toJson();
    return {kOk, ::strdup(response_body.dump().c_str())};
  } catch (const std::exception &e) {
    return {kGenericError, ::strdup(e.what())};
  }
}

Response read_dynmon_dataplane_config_egress_path_by_id_handler(
    const char *name, const Key *keys,
    size_t num_keys) {
  // Getting the path params
  std::string unique_name{name};
  try {
    auto x = read_dynmon_dataplane_config_egress_path_by_id(unique_name);
    nlohmann::json response_body;
    response_body = x.toJson();
    return {kOk, ::strdup(response_body.dump().c_str())};
  } catch (const std::exception &e) {
    return {kGenericError, ::strdup(e.what())};
  }
}

Response read_dynmon_dataplane_config_egress_path_code_by_id_handler(
    const char *name, const Key *keys,
    size_t num_keys) {
  // Getting the path params
  std::string unique_name{name};
  try {
    auto x = read_dynmon_dataplane_config_egress_path_code_by_id(unique_name);
    nlohmann::json response_body;
    response_body = x;
    return {kOk, ::strdup(response_body.dump().c_str())};
  } catch (const std::exception &e) {
    return {kGenericError, ::strdup(e.what())};
  }
}

Response read_dynmon_dataplane_config_egress_path_metric_configs_by_id_handler(
    const char *name, const Key *keys,
    size_t num_keys) {
  // Getting the path params
  std::string unique_name{name};
  std::string unique_metricConfigsName;
  for (size_t i = 0; i < num_keys; ++i)
    if (!strcmp(keys[i].name, "metric-configs_name")) {
      unique_metricConfigsName = std::string{keys[i].value.string};
      break;
    }
  try {
    auto x = read_dynmon_dataplane_config_egress_path_metric_configs_by_id(unique_name, unique_metricConfigsName);
    nlohmann::json response_body;
    response_body = x.toJson();
    return {kOk, ::strdup(response_body.dump().c_str())};
  } catch (const std::exception &e) {
    return {kGenericError, ::strdup(e.what())};
  }
}

Response read_dynmon_dataplane_config_egress_path_metric_configs_extraction_options_by_id_handler(
    const char *name, const Key *keys,
    size_t num_keys) {
  // Getting the path params
  std::string unique_name{name};
  std::string unique_metricConfigsName;
  for (size_t i = 0; i < num_keys; ++i)
    if (!strcmp(keys[i].name, "metric-configs_name")) {
      unique_metricConfigsName = std::string{keys[i].value.string};
      break;
    }
  try {
    auto x = read_dynmon_dataplane_config_egress_path_metric_configs_extraction_options_by_id(unique_name, unique_metricConfigsName);
    nlohmann::json response_body;
    response_body = x.toJson();
    return {kOk, ::strdup(response_body.dump().c_str())};
  } catch (const std::exception &e) {
    return {kGenericError, ::strdup(e.what())};
  }
}

Response read_dynmon_dataplane_config_egress_path_metric_configs_extraction_options_empty_on_read_by_id_handler(
    const char *name, const Key *keys,
    size_t num_keys) {
  // Getting the path params
  std::string unique_name{name};
  std::string unique_metricConfigsName;
  for (size_t i = 0; i < num_keys; ++i)
    if (!strcmp(keys[i].name, "metric-configs_name")) {
      unique_metricConfigsName = std::string{keys[i].value.string};
      break;
    }
  try {
    auto x = read_dynmon_dataplane_config_egress_path_metric_configs_extraction_options_empty_on_read_by_id(unique_name, unique_metricConfigsName);
    nlohmann::json response_body;
    response_body = x;
    return {kOk, ::strdup(response_body.dump().c_str())};
  } catch (const std::exception &e) {
    return {kGenericError, ::strdup(e.what())};
  }
}

Response read_dynmon_dataplane_config_egress_path_metric_configs_extraction_options_swap_on_read_by_id_handler(
    const char *name, const Key *keys,
    size_t num_keys) {
  // Getting the path params
  std::string unique_name{name};
  std::string unique_metricConfigsName;
  for (size_t i = 0; i < num_keys; ++i)
    if (!strcmp(keys[i].name, "metric-configs_name")) {
      unique_metricConfigsName = std::string{keys[i].value.string};
      break;
    }
  try {
    auto x = read_dynmon_dataplane_config_egress_path_metric_configs_extraction_options_swap_on_read_by_id(unique_name, unique_metricConfigsName);
    nlohmann::json response_body;
    response_body = x;
    return {kOk, ::strdup(response_body.dump().c_str())};
  } catch (const std::exception &e) {
    return {kGenericError, ::strdup(e.what())};
  }
}

Response read_dynmon_dataplane_config_egress_path_metric_configs_list_by_id_handler(
    const char *name, const Key *keys,
    size_t num_keys) {
  // Getting the path params
  std::string unique_name{name};
  try {
    auto x = read_dynmon_dataplane_config_egress_path_metric_configs_list_by_id(unique_name);
    nlohmann::json response_body;
    for (auto &i : x)
      response_body += i.toJson();
    return {kOk, ::strdup(response_body.dump().c_str())};
  } catch (const std::exception &e) {
    return {kGenericError, ::strdup(e.what())};
  }
}

Response read_dynmon_dataplane_config_egress_path_metric_configs_map_name_by_id_handler(
    const char *name, const Key *keys,
    size_t num_keys) {
  // Getting the path params
  std::string unique_name{name};
  std::string unique_metricConfigsName;
  for (size_t i = 0; i < num_keys; ++i)
    if (!strcmp(keys[i].name, "metric-configs_name")) {
      unique_metricConfigsName = std::string{keys[i].value.string};
      break;
    }
  try {
    auto x = read_dynmon_dataplane_config_egress_path_metric_configs_map_name_by_id(unique_name, unique_metricConfigsName);
    nlohmann::json response_body;
    response_body = x;
    return {kOk, ::strdup(response_body.dump().c_str())};
  } catch (const std::exception &e) {
    return {kGenericError, ::strdup(e.what())};
  }
}

Response read_dynmon_dataplane_config_egress_path_metric_configs_open_metrics_metadata_by_id_handler(
    const char *name, const Key *keys,
    size_t num_keys) {
  // Getting the path params
  std::string unique_name{name};
  std::string unique_metricConfigsName;
  for (size_t i = 0; i < num_keys; ++i)
    if (!strcmp(keys[i].name, "metric-configs_name")) {
      unique_metricConfigsName = std::string{keys[i].value.string};
      break;
    }
  try {
    auto x = read_dynmon_dataplane_config_egress_path_metric_configs_open_metrics_metadata_by_id(unique_name, unique_metricConfigsName);
    nlohmann::json response_body;
    response_body = x.toJson();
    return {kOk, ::strdup(response_body.dump().c_str())};
  } catch (const std::exception &e) {
    return {kGenericError, ::strdup(e.what())};
  }
}

Response read_dynmon_dataplane_config_egress_path_metric_configs_open_metrics_metadata_help_by_id_handler(
    const char *name, const Key *keys,
    size_t num_keys) {
  // Getting the path params
  std::string unique_name{name};
  std::string unique_metricConfigsName;
  for (size_t i = 0; i < num_keys; ++i)
    if (!strcmp(keys[i].name, "metric-configs_name")) {
      unique_metricConfigsName = std::string{keys[i].value.string};
      break;
    }
  try {
    auto x = read_dynmon_dataplane_config_egress_path_metric_configs_open_metrics_metadata_help_by_id(unique_name, unique_metricConfigsName);
    nlohmann::json response_body;
    response_body = x;
    return {kOk, ::strdup(response_body.dump().c_str())};
  } catch (const std::exception &e) {
    return {kGenericError, ::strdup(e.what())};
  }
}

Response read_dynmon_dataplane_config_egress_path_metric_configs_open_metrics_metadata_labels_by_id_handler(
    const char *name, const Key *keys,
    size_t num_keys) {
  // Getting the path params
  std::string unique_name{name};
  std::string unique_metricConfigsName;
  for (size_t i = 0; i < num_keys; ++i)
    if (!strcmp(keys[i].name, "metric-configs_name")) {
      unique_metricConfigsName = std::string{keys[i].value.string};
      break;
    }
  std::string unique_labelsName;
  for (size_t i = 0; i < num_keys; ++i)
    if (!strcmp(keys[i].name, "labels_name")) {
      unique_labelsName = std::string{keys[i].value.string};
      break;
    }
  try {
    auto x = read_dynmon_dataplane_config_egress_path_metric_configs_open_metrics_metadata_labels_by_id(unique_name, unique_metricConfigsName, unique_labelsName);
    nlohmann::json response_body;
    response_body = x.toJson();
    return {kOk, ::strdup(response_body.dump().c_str())};
  } catch (const std::exception &e) {
    return {kGenericError, ::strdup(e.what())};
  }
}

Response read_dynmon_dataplane_config_egress_path_metric_configs_open_metrics_metadata_labels_list_by_id_handler(
    const char *name, const Key *keys,
    size_t num_keys) {
  // Getting the path params
  std::string unique_name{name};
  std::string unique_metricConfigsName;
  for (size_t i = 0; i < num_keys; ++i)
    if (!strcmp(keys[i].name, "metric-configs_name")) {
      unique_metricConfigsName = std::string{keys[i].value.string};
      break;
    }
  try {
    auto x = read_dynmon_dataplane_config_egress_path_metric_configs_open_metrics_metadata_labels_list_by_id(unique_name, unique_metricConfigsName);
    nlohmann::json response_body;
    for (auto &i : x)
      response_body += i.toJson();
    return {kOk, ::strdup(response_body.dump().c_str())};
  } catch (const std::exception &e) {
    return {kGenericError, ::strdup(e.what())};
  }
}

Response read_dynmon_dataplane_config_egress_path_metric_configs_open_metrics_metadata_labels_value_by_id_handler(
    const char *name, const Key *keys,
    size_t num_keys) {
  // Getting the path params
  std::string unique_name{name};
  std::string unique_metricConfigsName;
  for (size_t i = 0; i < num_keys; ++i)
    if (!strcmp(keys[i].name, "metric-configs_name")) {
      unique_metricConfigsName = std::string{keys[i].value.string};
      break;
    }
  std::string unique_labelsName;
  for (size_t i = 0; i < num_keys; ++i)
    if (!strcmp(keys[i].name, "labels_name")) {
      unique_labelsName = std::string{keys[i].value.string};
      break;
    }
  try {
    auto x = read_dynmon_dataplane_config_egress_path_metric_configs_open_metrics_metadata_labels_value_by_id(unique_name, unique_metricConfigsName, unique_labelsName);
    nlohmann::json response_body;
    response_body = x;
    return {kOk, ::strdup(response_body.dump().c_str())};
  } catch (const std::exception &e) {
    return {kGenericError, ::strdup(e.what())};
  }
}

Response read_dynmon_dataplane_config_egress_path_metric_configs_open_metrics_metadata_type_by_id_handler(
    const char *name, const Key *keys,
    size_t num_keys) {
  // Getting the path params
  std::string unique_name{name};
  std::string unique_metricConfigsName;
  for (size_t i = 0; i < num_keys; ++i)
    if (!strcmp(keys[i].name, "metric-configs_name")) {
      unique_metricConfigsName = std::string{keys[i].value.string};
      break;
    }
  try {
    auto x = read_dynmon_dataplane_config_egress_path_metric_configs_open_metrics_metadata_type_by_id(unique_name, unique_metricConfigsName);
    nlohmann::json response_body;
    response_body = OpenMetricsMetadataJsonObject::MetricTypeEnum_to_string(x);
    return {kOk, ::strdup(response_body.dump().c_str())};
  } catch (const std::exception &e) {
    return {kGenericError, ::strdup(e.what())};
  }
}

Response read_dynmon_dataplane_config_egress_path_name_by_id_handler(const char *name, const Key *keys, size_t num_keys) {
  // Getting the path params
  std::string unique_name{name};
  try {
    auto x = read_dynmon_dataplane_config_egress_path_name_by_id(unique_name);
    nlohmann::json response_body;
    response_body = x;
    return {kOk, ::strdup(response_body.dump().c_str())};
  } catch (const std::exception &e) {
    return {kGenericError, ::strdup(e.what())};
  }
}

Response read_dynmon_dataplane_config_ingress_path_by_id_handler(const char *name, const Key *keys, size_t num_keys) {
  // Getting the path params
  std::string unique_name{name};
  try {
    auto x = read_dynmon_dataplane_config_ingress_path_by_id(unique_name);
    nlohmann::json response_body;
    response_body = x.toJson();
    return {kOk, ::strdup(response_body.dump().c_str())};
  } catch (const std::exception &e) {
    return {kGenericError, ::strdup(e.what())};
  }
}

Response read_dynmon_dataplane_config_ingress_path_code_by_id_handler(const char *name, const Key *keys, size_t num_keys) {
  // Getting the path params
  std::string unique_name{name};
  try {
    auto x = read_dynmon_dataplane_config_ingress_path_code_by_id(unique_name);
    nlohmann::json response_body;
    response_body = x;
    return {kOk, ::strdup(response_body.dump().c_str())};
  } catch (const std::exception &e) {
    return {kGenericError, ::strdup(e.what())};
  }
}

Response read_dynmon_dataplane_config_ingress_path_metric_configs_by_id_handler(const char *name, const Key *keys, size_t num_keys) {
  // Getting the path params
  std::string unique_name{name};
  std::string unique_metricConfigsName;
  for (size_t i = 0; i < num_keys; ++i)
    if (!strcmp(keys[i].name, "metric-configs_name")) {
      unique_metricConfigsName = std::string{keys[i].value.string};
      break;
    }
  try {
    auto x = read_dynmon_dataplane_config_ingress_path_metric_configs_by_id(unique_name, unique_metricConfigsName);
    nlohmann::json response_body;
    response_body = x.toJson();
    return {kOk, ::strdup(response_body.dump().c_str())};
  } catch (const std::exception &e) {
    return {kGenericError, ::strdup(e.what())};
  }
}

Response read_dynmon_dataplane_config_ingress_path_metric_configs_extraction_options_by_id_handler(const char *name, const Key *keys, size_t num_keys) {
  // Getting the path params
  std::string unique_name{name};
  std::string unique_metricConfigsName;
  for (size_t i = 0; i < num_keys; ++i)
    if (!strcmp(keys[i].name, "metric-configs_name")) {
      unique_metricConfigsName = std::string{keys[i].value.string};
      break;
    }
  try {
    auto x = read_dynmon_dataplane_config_ingress_path_metric_configs_extraction_options_by_id(unique_name, unique_metricConfigsName);
    nlohmann::json response_body;
    response_body = x.toJson();
    return {kOk, ::strdup(response_body.dump().c_str())};
  } catch (const std::exception &e) {
    return {kGenericError, ::strdup(e.what())};
  }
}

Response read_dynmon_dataplane_config_ingress_path_metric_configs_extraction_options_empty_on_read_by_id_handler(const char *name, const Key *keys, size_t num_keys) {
  // Getting the path params
  std::string unique_name{name};
  std::string unique_metricConfigsName;
  for (size_t i = 0; i < num_keys; ++i)
    if (!strcmp(keys[i].name, "metric-configs_name")) {
      unique_metricConfigsName = std::string{keys[i].value.string};
      break;
    }
  try {
    auto x = read_dynmon_dataplane_config_ingress_path_metric_configs_extraction_options_empty_on_read_by_id(unique_name, unique_metricConfigsName);
    nlohmann::json response_body;
    response_body = x;
    return {kOk, ::strdup(response_body.dump().c_str())};
  } catch (const std::exception &e) {
    return {kGenericError, ::strdup(e.what())};
  }
}

Response read_dynmon_dataplane_config_ingress_path_metric_configs_extraction_options_swap_on_read_by_id_handler(
    const char *name, const Key *keys,
    size_t num_keys) {
  // Getting the path params
  std::string unique_name{name};
  std::string unique_metricConfigsName;
  for (size_t i = 0; i < num_keys; ++i)
    if (!strcmp(keys[i].name, "metric-configs_name")) {
      unique_metricConfigsName = std::string{keys[i].value.string};
      break;
    }
  try {
    auto x = read_dynmon_dataplane_config_ingress_path_metric_configs_extraction_options_swap_on_read_by_id(unique_name, unique_metricConfigsName);
    nlohmann::json response_body;
    response_body = x;
    return {kOk, ::strdup(response_body.dump().c_str())};
  } catch (const std::exception &e) {
    return {kGenericError, ::strdup(e.what())};
  }
}

Response read_dynmon_dataplane_config_ingress_path_metric_configs_list_by_id_handler(
    const char *name, const Key *keys,
    size_t num_keys) {
  // Getting the path params
  std::string unique_name{name};
  try {
    auto x = read_dynmon_dataplane_config_ingress_path_metric_configs_list_by_id(unique_name);
    nlohmann::json response_body;
    for (auto &i : x)
      response_body += i.toJson();
    return {kOk, ::strdup(response_body.dump().c_str())};
  } catch (const std::exception &e) {
    return {kGenericError, ::strdup(e.what())};
  }
}

Response read_dynmon_dataplane_config_ingress_path_metric_configs_map_name_by_id_handler(
    const char *name, const Key *keys,
    size_t num_keys) {
  // Getting the path params
  std::string unique_name{name};
  std::string unique_metricConfigsName;
  for (size_t i = 0; i < num_keys; ++i)
    if (!strcmp(keys[i].name, "metric-configs_name")) {
      unique_metricConfigsName = std::string{keys[i].value.string};
      break;
    }
  try {
    auto x = read_dynmon_dataplane_config_ingress_path_metric_configs_map_name_by_id(unique_name, unique_metricConfigsName);
    nlohmann::json response_body;
    response_body = x;
    return {kOk, ::strdup(response_body.dump().c_str())};
  } catch (const std::exception &e) {
    return {kGenericError, ::strdup(e.what())};
  }
}

Response read_dynmon_dataplane_config_ingress_path_metric_configs_open_metrics_metadata_by_id_handler(
    const char *name, const Key *keys,
    size_t num_keys) {
  // Getting the path params
  std::string unique_name{name};
  std::string unique_metricConfigsName;
  for (size_t i = 0; i < num_keys; ++i)
    if (!strcmp(keys[i].name, "metric-configs_name")) {
      unique_metricConfigsName = std::string{keys[i].value.string};
      break;
    }
  try {
    auto x = read_dynmon_dataplane_config_ingress_path_metric_configs_open_metrics_metadata_by_id(unique_name, unique_metricConfigsName);
    nlohmann::json response_body;
    response_body = x.toJson();
    return {kOk, ::strdup(response_body.dump().c_str())};
  } catch (const std::exception &e) {
    return {kGenericError, ::strdup(e.what())};
  }
}

Response read_dynmon_dataplane_config_ingress_path_metric_configs_open_metrics_metadata_help_by_id_handler(
    const char *name, const Key *keys,
    size_t num_keys) {
  // Getting the path params
  std::string unique_name{name};
  std::string unique_metricConfigsName;
  for (size_t i = 0; i < num_keys; ++i)
    if (!strcmp(keys[i].name, "metric-configs_name")) {
      unique_metricConfigsName = std::string{keys[i].value.string};
      break;
    }
  try {
    auto x = read_dynmon_dataplane_config_ingress_path_metric_configs_open_metrics_metadata_help_by_id(unique_name, unique_metricConfigsName);
    nlohmann::json response_body;
    response_body = x;
    return {kOk, ::strdup(response_body.dump().c_str())};
  } catch (const std::exception &e) {
    return {kGenericError, ::strdup(e.what())};
  }
}

Response read_dynmon_dataplane_config_ingress_path_metric_configs_open_metrics_metadata_labels_by_id_handler(
    const char *name, const Key *keys,
    size_t num_keys) {
  // Getting the path params
  std::string unique_name{name};
  std::string unique_metricConfigsName;
  for (size_t i = 0; i < num_keys; ++i)
    if (!strcmp(keys[i].name, "metric-configs_name")) {
      unique_metricConfigsName = std::string{keys[i].value.string};
      break;
    }
  std::string unique_labelsName;
  for (size_t i = 0; i < num_keys; ++i) {
    if (!strcmp(keys[i].name, "labels_name")) {
      unique_labelsName = std::string{keys[i].value.string};
      break;
    }
  }
  try {
    auto x = read_dynmon_dataplane_config_ingress_path_metric_configs_open_metrics_metadata_labels_by_id(unique_name, unique_metricConfigsName, unique_labelsName);
    nlohmann::json response_body;
    response_body = x.toJson();
    return {kOk, ::strdup(response_body.dump().c_str())};
  } catch (const std::exception &e) {
    return {kGenericError, ::strdup(e.what())};
  }
}

Response read_dynmon_dataplane_config_ingress_path_metric_configs_open_metrics_metadata_labels_list_by_id_handler(
    const char *name, const Key *keys,
    size_t num_keys) {
  // Getting the path params
  std::string unique_name{name};
  std::string unique_metricConfigsName;
  for (size_t i = 0; i < num_keys; ++i)
    if (!strcmp(keys[i].name, "metric-configs_name")) {
      unique_metricConfigsName = std::string{keys[i].value.string};
      break;
    }
  try {
    auto x = read_dynmon_dataplane_config_ingress_path_metric_configs_open_metrics_metadata_labels_list_by_id(unique_name, unique_metricConfigsName);
    nlohmann::json response_body;
    for (auto &i : x)
      response_body += i.toJson();
    return {kOk, ::strdup(response_body.dump().c_str())};
  } catch (const std::exception &e) {
    return {kGenericError, ::strdup(e.what())};
  }
}

Response read_dynmon_dataplane_config_ingress_path_metric_configs_open_metrics_metadata_labels_value_by_id_handler(
    const char *name, const Key *keys,
    size_t num_keys) {
  // Getting the path params
  std::string unique_name{name};
  std::string unique_metricConfigsName;
  for (size_t i = 0; i < num_keys; ++i)
    if (!strcmp(keys[i].name, "metric-configs_name")) {
      unique_metricConfigsName = std::string{keys[i].value.string};
      break;
    }
  std::string unique_labelsName;
  for (size_t i = 0; i < num_keys; ++i)
    if (!strcmp(keys[i].name, "labels_name")) {
      unique_labelsName = std::string{keys[i].value.string};
      break;
    }
  try {
    auto x = read_dynmon_dataplane_config_ingress_path_metric_configs_open_metrics_metadata_labels_value_by_id(unique_name, unique_metricConfigsName, unique_labelsName);
    nlohmann::json response_body;
    response_body = x;
    return {kOk, ::strdup(response_body.dump().c_str())};
  } catch (const std::exception &e) {
    return {kGenericError, ::strdup(e.what())};
  }
}

Response read_dynmon_dataplane_config_ingress_path_metric_configs_open_metrics_metadata_type_by_id_handler(
    const char *name, const Key *keys,
    size_t num_keys) {
  // Getting the path params
  std::string unique_name{name};
  std::string unique_metricConfigsName;
  for (size_t i = 0; i < num_keys; ++i)
    if (!strcmp(keys[i].name, "metric-configs_name")) {
      unique_metricConfigsName = std::string{keys[i].value.string};
      break;
    }
  try {
    auto x = read_dynmon_dataplane_config_ingress_path_metric_configs_open_metrics_metadata_type_by_id(unique_name, unique_metricConfigsName);
    nlohmann::json response_body;
    response_body = OpenMetricsMetadataJsonObject::MetricTypeEnum_to_string(x);
    return {kOk, ::strdup(response_body.dump().c_str())};
  } catch (const std::exception &e) {
    return {kGenericError, ::strdup(e.what())};
  }
}

Response read_dynmon_dataplane_config_ingress_path_name_by_id_handler(
    const char *name, const Key *keys,
    size_t num_keys) {
  // Getting the path params
  std::string unique_name{name};

  try {

    auto x = read_dynmon_dataplane_config_ingress_path_name_by_id(unique_name);
    nlohmann::json response_body;
    response_body = x;
    return {kOk, ::strdup(response_body.dump().c_str())};
  } catch (const std::exception &e) {
    return {kGenericError, ::strdup(e.what())};
  }
}

Response read_dynmon_list_by_id_handler(
    const char *name, const Key *keys,
    size_t num_keys) {
  try {
    auto x = read_dynmon_list_by_id();
    nlohmann::json response_body;
    for (auto &i : x)
      response_body += i.toJson();
    return {kOk, ::strdup(response_body.dump().c_str())};
  } catch (const std::exception &e) {
    return {kGenericError, ::strdup(e.what())};
  }
}

Response read_dynmon_metrics_by_id_handler(
    const char *name, const Key *keys,
    size_t num_keys) {
  // Getting the path params
  std::string unique_name{name};
  try {
    auto x = read_dynmon_metrics_by_id(unique_name);
    nlohmann::json response_body;
    response_body = x.toJson();
    return {kOk, ::strdup(response_body.dump().c_str())};
  } catch (const std::exception &e) {
    return {kGenericError, ::strdup(e.what())};
  }
}

Response read_dynmon_metrics_egress_metrics_by_id_handler(
    const char *name, const Key *keys,
    size_t num_keys) {
  // Getting the path params
  std::string unique_name{name};
  std::string unique_egressName;
  for (size_t i = 0; i < num_keys; ++i)
    if (!strcmp(keys[i].name, "egress-metrics_name")) {
      unique_egressName = std::string{keys[i].value.string};
      break;
    }
  try {
    auto x = read_dynmon_metrics_egress_metrics_by_id(unique_name, unique_egressName);
    nlohmann::json response_body;
    response_body = x.toJson();
    return {kOk, ::strdup(response_body.dump().c_str())};
  } catch (const std::exception &e) {
    return {kGenericError, ::strdup(e.what())};
  }
}

Response read_dynmon_metrics_egress_metrics_list_by_id_handler(
    const char *name, const Key *keys,
    size_t num_keys) {
  // Getting the path params
  std::string unique_name{name};
  try {
    auto x = read_dynmon_metrics_egress_metrics_list_by_id(unique_name);
    nlohmann::json response_body = nlohmann::json::array();
    for (auto &i : x)
      response_body.push_back(i.toJson());
    return {kOk, ::strdup(response_body.dump().c_str())};
  } catch (const std::exception &e) {
    return {kGenericError, ::strdup(e.what())};
  }
}

Response read_dynmon_metrics_egress_metrics_timestamp_by_id_handler(
    const char *name, const Key *keys,
    size_t num_keys) {
  // Getting the path params
  std::string unique_name{name};
  std::string unique_egressName;
  for (size_t i = 0; i < num_keys; ++i)
    if (!strcmp(keys[i].name, "egress-metrics_name")) {
      unique_egressName = std::string{keys[i].value.string};
      break;
    }
  try {
    auto x = read_dynmon_metrics_egress_metrics_timestamp_by_id(unique_name, unique_egressName);
    nlohmann::json response_body;
    response_body = x;
    return {kOk, ::strdup(response_body.dump().c_str())};
  } catch (const std::exception &e) {
    return {kGenericError, ::strdup(e.what())};
  }
}

Response read_dynmon_metrics_egress_metrics_value_by_id_handler(
    const char *name, const Key *keys,
    size_t num_keys) {
  // Getting the path params
  std::string unique_name{name};
  std::string unique_egressName;
  for (size_t i = 0; i < num_keys; ++i)
    if (!strcmp(keys[i].name, "egress-metrics_name")) {
      unique_egressName = std::string{keys[i].value.string};
      break;
    }
  try {
    auto x = read_dynmon_metrics_egress_metrics_value_by_id(unique_name, unique_egressName);
    nlohmann::json response_body;
    response_body = x;
    return {kOk, ::strdup(response_body.dump().c_str())};
  } catch (const std::exception &e) {
    return {kGenericError, ::strdup(e.what())};
  }
}

Response read_dynmon_metrics_ingress_metrics_by_id_handler(
    const char *name, const Key *keys,
    size_t num_keys) {
  // Getting the path params
  std::string unique_name{name};
  std::string unique_ingressName;
  for (size_t i = 0; i < num_keys; ++i)
    if (!strcmp(keys[i].name, "ingress-metrics_name")) {
      unique_ingressName = std::string{keys[i].value.string};
      break;
    }
  try {
    auto x = read_dynmon_metrics_ingress_metrics_by_id(unique_name, unique_ingressName);
    nlohmann::json response_body;
    response_body = x.toJson();
    return {kOk, ::strdup(response_body.dump().c_str())};
  } catch (const std::exception &e) {
    return {kGenericError, ::strdup(e.what())};
  }
}

Response read_dynmon_metrics_ingress_metrics_list_by_id_handler(
    const char *name, const Key *keys,
    size_t num_keys) {
  // Getting the path params
  std::string unique_name{name};
  try {
    auto x = read_dynmon_metrics_ingress_metrics_list_by_id(unique_name);
    nlohmann::json response_body = nlohmann::json::array();
    for (auto &i : x)
      response_body.push_back(i.toJson());
    return {kOk, ::strdup(response_body.dump().c_str())};
  } catch (const std::exception &e) {
    return {kGenericError, ::strdup(e.what())};
  }
}

Response read_dynmon_metrics_ingress_metrics_timestamp_by_id_handler(
    const char *name, const Key *keys,
    size_t num_keys) {
  // Getting the path params
  std::string unique_name{name};
  std::string unique_ingressName;
  for (size_t i = 0; i < num_keys; ++i)
    if (!strcmp(keys[i].name, "ingress-metrics_name")) {
      unique_ingressName = std::string{keys[i].value.string};
      break;
    }
  try {
    auto x = read_dynmon_metrics_ingress_metrics_timestamp_by_id(unique_name, unique_ingressName);
    nlohmann::json response_body;
    response_body = x;
    return {kOk, ::strdup(response_body.dump().c_str())};
  } catch (const std::exception &e) {
    return {kGenericError, ::strdup(e.what())};
  }
}

Response read_dynmon_metrics_ingress_metrics_value_by_id_handler(
    const char *name, const Key *keys,
    size_t num_keys) {
  // Getting the path params
  std::string unique_name{name};
  std::string unique_ingressName;
  for (size_t i = 0; i < num_keys; ++i)
    if (!strcmp(keys[i].name, "ingress-metrics_name")) {
      unique_ingressName = std::string{keys[i].value.string};
      break;
    }
  try {
    auto x = read_dynmon_metrics_ingress_metrics_value_by_id(unique_name, unique_ingressName);
    nlohmann::json response_body;
    response_body = x;
    return {kOk, ::strdup(response_body.dump().c_str())};
  } catch (const std::exception &e) {
    return {kGenericError, ::strdup(e.what())};
  }
}

Response read_dynmon_open_metrics_by_id_handler(
    const char *name, const Key *keys,
    size_t num_keys) {
  // Getting the path params
  std::string unique_name{name};
  try {
    auto openMetrics = read_dynmon_open_metrics_by_id(unique_name);
    return {kOk, ::strdup(openMetrics.c_str())};
  } catch (const std::exception &e) {
    return {kGenericError, ::strdup(e.what())};
  }
}

Response replace_dynmon_by_id_handler(
    const char *name, const Key *keys,
    size_t num_keys,
    const char *value) {
  // Getting the path params
  std::string unique_name{name};
  try {
    auto request_body = nlohmann::json::parse(std::string{value});
    // Getting the body param
    DynmonJsonObject unique_value{request_body};
    unique_value.setName(unique_name);
    replace_dynmon_by_id(unique_name, unique_value);
    return {kOk, nullptr};
  } catch (const std::exception &e) {
    return {kGenericError, ::strdup(e.what())};
  }
}

Response replace_dynmon_dataplane_config_by_id_handler(
    const char *name, const Key *keys,
    size_t num_keys,
    const char *value) {
  // Getting the path params
  std::string unique_name{name};
  try {
    auto request_body = nlohmann::json::parse(std::string{value});
    // Getting the body param
    DataplaneConfigJsonObject unique_value{request_body};
    replace_dynmon_dataplane_config_by_id(unique_name, unique_value);
    return {kOk, nullptr};
  } catch (const std::exception &e) {
    return {kGenericError, ::strdup(e.what())};
  }
}

Response replace_dynmon_dataplane_config_egress_path_by_id_handler(
    const char *name, const Key *keys,
    size_t num_keys,
    const char *value) {
  // Getting the path params
  std::string unique_name{name};
  try {
    auto request_body = nlohmann::json::parse(std::string{value});
    // Getting the body param
    PathConfigJsonObject unique_value{request_body};
    replace_dynmon_dataplane_config_egress_path_by_id(unique_name, unique_value);
    return {kOk, nullptr};
  } catch (const std::exception &e) {
    return {kGenericError, ::strdup(e.what())};
  }
}

Response replace_dynmon_dataplane_config_ingress_path_by_id_handler(
    const char *name, const Key *keys,
    size_t num_keys,
    const char *value) {
  // Getting the path params
  std::string unique_name{name};
  try {
    auto request_body = nlohmann::json::parse(std::string{value});
    // Getting the body param
    PathConfigJsonObject unique_value{request_body};
    replace_dynmon_dataplane_config_ingress_path_by_id(unique_name, unique_value);
    return {kOk, nullptr};
  } catch (const std::exception &e) {
    return {kGenericError, ::strdup(e.what())};
  }
}

Response update_dynmon_by_id_handler(
    const char *name, const Key *keys,
    size_t num_keys,
    const char *value) {
  // Getting the path params
  std::string unique_name{name};
  try {
    auto request_body = nlohmann::json::parse(std::string{value});
    // Getting the body param
    DynmonJsonObject unique_value{request_body};
    unique_value.setName(unique_name);
    update_dynmon_by_id(unique_name, unique_value);
    return {kOk, nullptr};
  } catch (const std::exception &e) {
    return {kGenericError, ::strdup(e.what())};
  }
}

Response update_dynmon_dataplane_config_by_id_handler(
    const char *name, const Key *keys,
    size_t num_keys,
    const char *value) {
  // Getting the path params
  std::string unique_name{name};
  try {
    auto request_body = nlohmann::json::parse(std::string{value});
    // Getting the body param
    DataplaneConfigJsonObject unique_value{request_body};
    update_dynmon_dataplane_config_by_id(unique_name, unique_value);
    return {kOk, nullptr};
  } catch (const std::exception &e) {
    return {kGenericError, ::strdup(e.what())};
  }
}

Response update_dynmon_dataplane_config_egress_path_by_id_handler(
    const char *name, const Key *keys,
    size_t num_keys,
    const char *value) {
  // Getting the path params
  std::string unique_name{name};
  try {
    auto request_body = nlohmann::json::parse(std::string{value});
    // Getting the body param
    PathConfigJsonObject unique_value{request_body};
    update_dynmon_dataplane_config_egress_path_by_id(unique_name, unique_value);
    return {kOk, nullptr};
  } catch (const std::exception &e) {
    return {kGenericError, ::strdup(e.what())};
  }
}

Response update_dynmon_dataplane_config_egress_path_name_by_id_handler(
    const char *name, const Key *keys,
    size_t num_keys,
    const char *value) {
  // Getting the path params
  std::string unique_name{name};
  try {
    auto request_body = nlohmann::json::parse(std::string{value});
    // The conversion is done automatically by the json library
    std::string unique_value = request_body;
    update_dynmon_dataplane_config_egress_path_name_by_id(unique_name, unique_value);
    return {kOk, nullptr};
  } catch (const std::exception &e) {
    return {kGenericError, ::strdup(e.what())};
  }
}

Response update_dynmon_dataplane_config_ingress_path_by_id_handler(
    const char *name, const Key *keys,
    size_t num_keys,
    const char *value) {
  // Getting the path params
  std::string unique_name{name};
  try {
    auto request_body = nlohmann::json::parse(std::string{value});
    // Getting the body param
    PathConfigJsonObject unique_value{request_body};
    update_dynmon_dataplane_config_ingress_path_by_id(unique_name, unique_value);
    return {kOk, nullptr};
  } catch (const std::exception &e) {
    return {kGenericError, ::strdup(e.what())};
  }
}

Response update_dynmon_dataplane_config_ingress_path_name_by_id_handler(
    const char *name, const Key *keys,
    size_t num_keys,
    const char *value) {
  // Getting the path params
  std::string unique_name{name};
  try {
    auto request_body = nlohmann::json::parse(std::string{value});
    // The conversion is done automatically by the json library
    std::string unique_value = request_body;
    update_dynmon_dataplane_config_ingress_path_name_by_id(unique_name, unique_value);
    return {kOk, nullptr};
  } catch (const std::exception &e) {
    return {kGenericError, ::strdup(e.what())};
  }
}

Response update_dynmon_list_by_id_handler(
    const char *name, const Key *keys,
    size_t num_keys,
    const char *value) {
  // Getting the body param
  try {
    auto request_body = nlohmann::json::parse(std::string{value});
    // Getting the body param
    std::vector<DynmonJsonObject> unique_value;
    for (auto &j : request_body) {
      DynmonJsonObject a{j};
      unique_value.push_back(a);
    }
    update_dynmon_list_by_id(unique_value);
    return {kOk, nullptr};
  } catch (const std::exception &e) {
    return {kGenericError, ::strdup(e.what())};
  }
}

Response dynmon_dataplane_config_egress_path_metric_configs_list_by_id_help(
    const char *name, const Key *keys, size_t num_keys) {
  // Getting the path params
  std::string unique_name{name};
  nlohmann::json val = read_dynmon_dataplane_config_egress_path_metric_configs_list_by_id_get_list(unique_name);
  return {kOk, ::strdup(val.dump().c_str())};
}

Response dynmon_dataplane_config_egress_path_metric_configs_open_metrics_metadata_labels_list_by_id_help(
    const char *name, const Key *keys, size_t num_keys) {
  // Getting the path params
  std::string unique_name{name};
  std::string unique_metricConfigsName;
  for (size_t i = 0; i < num_keys; ++i)
    if (!strcmp(keys[i].name, "metric-configs_name")) {
      unique_metricConfigsName = std::string{keys[i].value.string};
      break;
    }
  nlohmann::json val = read_dynmon_dataplane_config_egress_path_metric_configs_open_metrics_metadata_labels_list_by_id_get_list(unique_name, unique_metricConfigsName);
  return {kOk, ::strdup(val.dump().c_str())};
}

Response dynmon_dataplane_config_ingress_path_metric_configs_list_by_id_help(
    const char *name, const Key *keys, size_t num_keys) {
  // Getting the path params
  std::string unique_name{name};
  nlohmann::json val = read_dynmon_dataplane_config_ingress_path_metric_configs_list_by_id_get_list(unique_name);
  return {kOk, ::strdup(val.dump().c_str())};
}

Response dynmon_dataplane_config_ingress_path_metric_configs_open_metrics_metadata_labels_list_by_id_help(
    const char *name, const Key *keys, size_t num_keys) {
  // Getting the path params
  std::string unique_name{name};
  std::string unique_metricConfigsName;
  for (size_t i = 0; i < num_keys; ++i)
    if (!strcmp(keys[i].name, "metric-configs_name")) {
      unique_metricConfigsName = std::string{keys[i].value.string};
      break;
    }
  nlohmann::json val = read_dynmon_dataplane_config_ingress_path_metric_configs_open_metrics_metadata_labels_list_by_id_get_list(unique_name, unique_metricConfigsName);
  return {kOk, ::strdup(val.dump().c_str())};
}

Response dynmon_list_by_id_help(
    const char *name, const Key *keys, size_t num_keys) {
  nlohmann::json val = read_dynmon_list_by_id_get_list();
  return {kOk, ::strdup(val.dump().c_str())};
}

Response dynmon_metrics_egress_metrics_list_by_id_help(
    const char *name, const Key *keys, size_t num_keys) {
  // Getting the path params
  std::string unique_name{name};
  nlohmann::json val = read_dynmon_metrics_egress_metrics_list_by_id_get_list(unique_name);
  return {kOk, ::strdup(val.dump().c_str())};
}

Response dynmon_metrics_ingress_metrics_list_by_id_help(
    const char *name, const Key *keys, size_t num_keys) {
  // Getting the path params
  std::string unique_name{name};
  nlohmann::json val = read_dynmon_metrics_ingress_metrics_list_by_id_get_list(unique_name);
  return {kOk, ::strdup(val.dump().c_str())};
}

#ifdef __cplusplus
}
#endif
