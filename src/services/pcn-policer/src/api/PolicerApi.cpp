/**
* policer API generated from policer.yang
*
* NOTE: This file is auto generated by polycube-codegen
* https://github.com/polycube-network/polycube-codegen
*/


/* Do not edit this file manually */


#include "PolicerApi.h"
#include "PolicerApiImpl.h"

using namespace polycube::service::model;
using namespace polycube::service::api::PolicerApiImpl;

#ifdef __cplusplus
extern "C" {
#endif

Response create_policer_by_id_handler(
  const char *name, const Key *keys,
  size_t num_keys ,
  const char *value) {
  // Getting the path params
  std::string unique_name { name };

  try {
    auto request_body = nlohmann::json::parse(std::string { value });
    // Getting the body param
    PolicerJsonObject unique_value { request_body };

    unique_value.setName(unique_name);
    create_policer_by_id(unique_name, unique_value);
    return { kCreated, nullptr };
  } catch(const std::exception &e) {
    return { kGenericError, ::strdup(e.what()) };
  }
}

Response create_policer_contract_by_id_handler(
  const char *name, const Key *keys,
  size_t num_keys ,
  const char *value) {
  // Getting the path params
  std::string unique_name { name };
  uint32_t unique_trafficClass;
  for (size_t i = 0; i < num_keys; ++i) {
    if (!strcmp(keys[i].name, "traffic-class")) {
      unique_trafficClass = keys[i].value.uint32;
      break;
    }
  }


  try {
    auto request_body = nlohmann::json::parse(std::string { value });
    // Getting the body param
    ContractJsonObject unique_value { request_body };

    unique_value.setTrafficClass(unique_trafficClass);
    create_policer_contract_by_id(unique_name, unique_trafficClass, unique_value);
    return { kCreated, nullptr };
  } catch(const std::exception &e) {
    return { kGenericError, ::strdup(e.what()) };
  }
}

Response create_policer_contract_list_by_id_handler(
  const char *name, const Key *keys,
  size_t num_keys ,
  const char *value) {
  // Getting the path params
  std::string unique_name { name };
  // Getting the body param
  std::vector<ContractJsonObject> unique_value;

  try {
    auto request_body = nlohmann::json::parse(std::string { value });
    // Getting the body param
    std::vector<ContractJsonObject> unique_value;
    for (auto &j : request_body) {
      ContractJsonObject a { j };
      unique_value.push_back(a);
    }
    create_policer_contract_list_by_id(unique_name, unique_value);
    return { kCreated, nullptr };
  } catch(const std::exception &e) {
    return { kGenericError, ::strdup(e.what()) };
  }
}

Response create_policer_contract_update_data_by_id_handler(
  const char *name, const Key *keys,
  size_t num_keys ,
  const char *value) {
  // Getting the path params
  std::string unique_name { name };
  uint32_t unique_trafficClass;
  for (size_t i = 0; i < num_keys; ++i) {
    if (!strcmp(keys[i].name, "traffic-class")) {
      unique_trafficClass = keys[i].value.uint32;
      break;
    }
  }


  try {
    auto request_body = nlohmann::json::parse(std::string { value });
    // Getting the body param
    ContractUpdateDataInputJsonObject unique_value { request_body };

    create_policer_contract_update_data_by_id(unique_name, unique_trafficClass, unique_value);
    return { kCreated, nullptr };
  } catch(const std::exception &e) {
    return { kGenericError, ::strdup(e.what()) };
  }
}

Response create_policer_default_contract_by_id_handler(
  const char *name, const Key *keys,
  size_t num_keys ,
  const char *value) {
  // Getting the path params
  std::string unique_name { name };

  try {
    auto request_body = nlohmann::json::parse(std::string { value });
    // Getting the body param
    DefaultContractJsonObject unique_value { request_body };

    create_policer_default_contract_by_id(unique_name, unique_value);
    return { kCreated, nullptr };
  } catch(const std::exception &e) {
    return { kGenericError, ::strdup(e.what()) };
  }
}

Response create_policer_default_contract_update_data_by_id_handler(
  const char *name, const Key *keys,
  size_t num_keys ,
  const char *value) {
  // Getting the path params
  std::string unique_name { name };

  try {
    auto request_body = nlohmann::json::parse(std::string { value });
    // Getting the body param
    DefaultContractUpdateDataInputJsonObject unique_value { request_body };

    create_policer_default_contract_update_data_by_id(unique_name, unique_value);
    return { kCreated, nullptr };
  } catch(const std::exception &e) {
    return { kGenericError, ::strdup(e.what()) };
  }
}

Response delete_policer_by_id_handler(
  const char *name, const Key *keys,
  size_t num_keys ) {
  // Getting the path params
  std::string unique_name { name };

  try {
    delete_policer_by_id(unique_name);
    return { kOk, nullptr };
  } catch(const std::exception &e) {
    return { kGenericError, ::strdup(e.what()) };
  }
}

Response delete_policer_contract_by_id_handler(
  const char *name, const Key *keys,
  size_t num_keys ) {
  // Getting the path params
  std::string unique_name { name };
  uint32_t unique_trafficClass;
  for (size_t i = 0; i < num_keys; ++i) {
    if (!strcmp(keys[i].name, "traffic-class")) {
      unique_trafficClass = keys[i].value.uint32;
      break;
    }
  }


  try {
    delete_policer_contract_by_id(unique_name, unique_trafficClass);
    return { kOk, nullptr };
  } catch(const std::exception &e) {
    return { kGenericError, ::strdup(e.what()) };
  }
}

Response delete_policer_contract_list_by_id_handler(
  const char *name, const Key *keys,
  size_t num_keys ) {
  // Getting the path params
  std::string unique_name { name };

  try {
    delete_policer_contract_list_by_id(unique_name);
    return { kOk, nullptr };
  } catch(const std::exception &e) {
    return { kGenericError, ::strdup(e.what()) };
  }
}

Response delete_policer_default_contract_by_id_handler(
  const char *name, const Key *keys,
  size_t num_keys ) {
  // Getting the path params
  std::string unique_name { name };

  try {
    delete_policer_default_contract_by_id(unique_name);
    return { kOk, nullptr };
  } catch(const std::exception &e) {
    return { kGenericError, ::strdup(e.what()) };
  }
}

Response read_policer_by_id_handler(
  const char *name, const Key *keys,
  size_t num_keys ) {
  // Getting the path params
  std::string unique_name { name };

  try {

    auto x = read_policer_by_id(unique_name);
    nlohmann::json response_body;
    response_body = x.toJson();
    return { kOk, ::strdup(response_body.dump().c_str()) };
  } catch(const std::exception &e) {
    return { kGenericError, ::strdup(e.what()) };
  }
}

Response read_policer_contract_action_by_id_handler(
  const char *name, const Key *keys,
  size_t num_keys ) {
  // Getting the path params
  std::string unique_name { name };
  uint32_t unique_trafficClass;
  for (size_t i = 0; i < num_keys; ++i) {
    if (!strcmp(keys[i].name, "traffic-class")) {
      unique_trafficClass = keys[i].value.uint32;
      break;
    }
  }


  try {

    auto x = read_policer_contract_action_by_id(unique_name, unique_trafficClass);
    nlohmann::json response_body;
    response_body = ContractJsonObject::ActionTypeEnum_to_string(x);
    return { kOk, ::strdup(response_body.dump().c_str()) };
  } catch(const std::exception &e) {
    return { kGenericError, ::strdup(e.what()) };
  }
}

Response read_policer_contract_burst_limit_by_id_handler(
  const char *name, const Key *keys,
  size_t num_keys ) {
  // Getting the path params
  std::string unique_name { name };
  uint32_t unique_trafficClass;
  for (size_t i = 0; i < num_keys; ++i) {
    if (!strcmp(keys[i].name, "traffic-class")) {
      unique_trafficClass = keys[i].value.uint32;
      break;
    }
  }


  try {

    auto x = read_policer_contract_burst_limit_by_id(unique_name, unique_trafficClass);
    nlohmann::json response_body;
    response_body = x;
    return { kOk, ::strdup(response_body.dump().c_str()) };
  } catch(const std::exception &e) {
    return { kGenericError, ::strdup(e.what()) };
  }
}

Response read_policer_contract_by_id_handler(
  const char *name, const Key *keys,
  size_t num_keys ) {
  // Getting the path params
  std::string unique_name { name };
  uint32_t unique_trafficClass;
  for (size_t i = 0; i < num_keys; ++i) {
    if (!strcmp(keys[i].name, "traffic-class")) {
      unique_trafficClass = keys[i].value.uint32;
      break;
    }
  }


  try {

    auto x = read_policer_contract_by_id(unique_name, unique_trafficClass);
    nlohmann::json response_body;
    response_body = x.toJson();
    return { kOk, ::strdup(response_body.dump().c_str()) };
  } catch(const std::exception &e) {
    return { kGenericError, ::strdup(e.what()) };
  }
}

Response read_policer_contract_list_by_id_handler(
  const char *name, const Key *keys,
  size_t num_keys ) {
  // Getting the path params
  std::string unique_name { name };

  try {

    auto x = read_policer_contract_list_by_id(unique_name);
    nlohmann::json response_body;
    for (auto &i : x) {
      response_body += i.toJson();
    }
    return { kOk, ::strdup(response_body.dump().c_str()) };
  } catch(const std::exception &e) {
    return { kGenericError, ::strdup(e.what()) };
  }
}

Response read_policer_contract_rate_limit_by_id_handler(
  const char *name, const Key *keys,
  size_t num_keys ) {
  // Getting the path params
  std::string unique_name { name };
  uint32_t unique_trafficClass;
  for (size_t i = 0; i < num_keys; ++i) {
    if (!strcmp(keys[i].name, "traffic-class")) {
      unique_trafficClass = keys[i].value.uint32;
      break;
    }
  }


  try {

    auto x = read_policer_contract_rate_limit_by_id(unique_name, unique_trafficClass);
    nlohmann::json response_body;
    response_body = x;
    return { kOk, ::strdup(response_body.dump().c_str()) };
  } catch(const std::exception &e) {
    return { kGenericError, ::strdup(e.what()) };
  }
}

Response read_policer_default_contract_action_by_id_handler(
  const char *name, const Key *keys,
  size_t num_keys ) {
  // Getting the path params
  std::string unique_name { name };

  try {

    auto x = read_policer_default_contract_action_by_id(unique_name);
    nlohmann::json response_body;
    response_body = DefaultContractJsonObject::ActionTypeEnum_to_string(x);
    return { kOk, ::strdup(response_body.dump().c_str()) };
  } catch(const std::exception &e) {
    return { kGenericError, ::strdup(e.what()) };
  }
}

Response read_policer_default_contract_burst_limit_by_id_handler(
  const char *name, const Key *keys,
  size_t num_keys ) {
  // Getting the path params
  std::string unique_name { name };

  try {

    auto x = read_policer_default_contract_burst_limit_by_id(unique_name);
    nlohmann::json response_body;
    response_body = x;
    return { kOk, ::strdup(response_body.dump().c_str()) };
  } catch(const std::exception &e) {
    return { kGenericError, ::strdup(e.what()) };
  }
}

Response read_policer_default_contract_by_id_handler(
  const char *name, const Key *keys,
  size_t num_keys ) {
  // Getting the path params
  std::string unique_name { name };

  try {

    auto x = read_policer_default_contract_by_id(unique_name);
    nlohmann::json response_body;
    response_body = x.toJson();
    return { kOk, ::strdup(response_body.dump().c_str()) };
  } catch(const std::exception &e) {
    return { kGenericError, ::strdup(e.what()) };
  }
}

Response read_policer_default_contract_rate_limit_by_id_handler(
  const char *name, const Key *keys,
  size_t num_keys ) {
  // Getting the path params
  std::string unique_name { name };

  try {

    auto x = read_policer_default_contract_rate_limit_by_id(unique_name);
    nlohmann::json response_body;
    response_body = x;
    return { kOk, ::strdup(response_body.dump().c_str()) };
  } catch(const std::exception &e) {
    return { kGenericError, ::strdup(e.what()) };
  }
}

Response read_policer_list_by_id_handler(
  const char *name, const Key *keys,
  size_t num_keys ) {


  try {

    auto x = read_policer_list_by_id();
    nlohmann::json response_body;
    for (auto &i : x) {
      response_body += i.toJson();
    }
    return { kOk, ::strdup(response_body.dump().c_str()) };
  } catch(const std::exception &e) {
    return { kGenericError, ::strdup(e.what()) };
  }
}

Response replace_policer_by_id_handler(
  const char *name, const Key *keys,
  size_t num_keys ,
  const char *value) {
  // Getting the path params
  std::string unique_name { name };

  try {
    auto request_body = nlohmann::json::parse(std::string { value });
    // Getting the body param
    PolicerJsonObject unique_value { request_body };

    unique_value.setName(unique_name);
    replace_policer_by_id(unique_name, unique_value);
    return { kOk, nullptr };
  } catch(const std::exception &e) {
    return { kGenericError, ::strdup(e.what()) };
  }
}

Response replace_policer_contract_by_id_handler(
  const char *name, const Key *keys,
  size_t num_keys ,
  const char *value) {
  // Getting the path params
  std::string unique_name { name };
  uint32_t unique_trafficClass;
  for (size_t i = 0; i < num_keys; ++i) {
    if (!strcmp(keys[i].name, "traffic-class")) {
      unique_trafficClass = keys[i].value.uint32;
      break;
    }
  }


  try {
    auto request_body = nlohmann::json::parse(std::string { value });
    // Getting the body param
    ContractJsonObject unique_value { request_body };

    unique_value.setTrafficClass(unique_trafficClass);
    replace_policer_contract_by_id(unique_name, unique_trafficClass, unique_value);
    return { kOk, nullptr };
  } catch(const std::exception &e) {
    return { kGenericError, ::strdup(e.what()) };
  }
}

Response replace_policer_contract_list_by_id_handler(
  const char *name, const Key *keys,
  size_t num_keys ,
  const char *value) {
  // Getting the path params
  std::string unique_name { name };
  // Getting the body param
  std::vector<ContractJsonObject> unique_value;

  try {
    auto request_body = nlohmann::json::parse(std::string { value });
    // Getting the body param
    std::vector<ContractJsonObject> unique_value;
    for (auto &j : request_body) {
      ContractJsonObject a { j };
      unique_value.push_back(a);
    }
    replace_policer_contract_list_by_id(unique_name, unique_value);
    return { kOk, nullptr };
  } catch(const std::exception &e) {
    return { kGenericError, ::strdup(e.what()) };
  }
}

Response replace_policer_default_contract_by_id_handler(
  const char *name, const Key *keys,
  size_t num_keys ,
  const char *value) {
  // Getting the path params
  std::string unique_name { name };

  try {
    auto request_body = nlohmann::json::parse(std::string { value });
    // Getting the body param
    DefaultContractJsonObject unique_value { request_body };

    replace_policer_default_contract_by_id(unique_name, unique_value);
    return { kOk, nullptr };
  } catch(const std::exception &e) {
    return { kGenericError, ::strdup(e.what()) };
  }
}

Response update_policer_by_id_handler(
  const char *name, const Key *keys,
  size_t num_keys ,
  const char *value) {
  // Getting the path params
  std::string unique_name { name };

  try {
    auto request_body = nlohmann::json::parse(std::string { value });
    // Getting the body param
    PolicerJsonObject unique_value { request_body };

    unique_value.setName(unique_name);
    update_policer_by_id(unique_name, unique_value);
    return { kOk, nullptr };
  } catch(const std::exception &e) {
    return { kGenericError, ::strdup(e.what()) };
  }
}

Response update_policer_contract_by_id_handler(
  const char *name, const Key *keys,
  size_t num_keys ,
  const char *value) {
  // Getting the path params
  std::string unique_name { name };
  uint32_t unique_trafficClass;
  for (size_t i = 0; i < num_keys; ++i) {
    if (!strcmp(keys[i].name, "traffic-class")) {
      unique_trafficClass = keys[i].value.uint32;
      break;
    }
  }


  try {
    auto request_body = nlohmann::json::parse(std::string { value });
    // Getting the body param
    ContractJsonObject unique_value { request_body };

    unique_value.setTrafficClass(unique_trafficClass);
    update_policer_contract_by_id(unique_name, unique_trafficClass, unique_value);
    return { kOk, nullptr };
  } catch(const std::exception &e) {
    return { kGenericError, ::strdup(e.what()) };
  }
}

Response update_policer_contract_list_by_id_handler(
  const char *name, const Key *keys,
  size_t num_keys ,
  const char *value) {
  // Getting the path params
  std::string unique_name { name };
  // Getting the body param
  std::vector<ContractJsonObject> unique_value;

  try {
    auto request_body = nlohmann::json::parse(std::string { value });
    // Getting the body param
    std::vector<ContractJsonObject> unique_value;
    for (auto &j : request_body) {
      ContractJsonObject a { j };
      unique_value.push_back(a);
    }
    update_policer_contract_list_by_id(unique_name, unique_value);
    return { kOk, nullptr };
  } catch(const std::exception &e) {
    return { kGenericError, ::strdup(e.what()) };
  }
}

Response update_policer_default_contract_by_id_handler(
  const char *name, const Key *keys,
  size_t num_keys ,
  const char *value) {
  // Getting the path params
  std::string unique_name { name };

  try {
    auto request_body = nlohmann::json::parse(std::string { value });
    // Getting the body param
    DefaultContractJsonObject unique_value { request_body };

    update_policer_default_contract_by_id(unique_name, unique_value);
    return { kOk, nullptr };
  } catch(const std::exception &e) {
    return { kGenericError, ::strdup(e.what()) };
  }
}

Response update_policer_list_by_id_handler(
  const char *name, const Key *keys,
  size_t num_keys ,
  const char *value) {

  // Getting the body param
  std::vector<PolicerJsonObject> unique_value;

  try {
    auto request_body = nlohmann::json::parse(std::string { value });
    // Getting the body param
    std::vector<PolicerJsonObject> unique_value;
    for (auto &j : request_body) {
      PolicerJsonObject a { j };
      unique_value.push_back(a);
    }
    update_policer_list_by_id(unique_value);
    return { kOk, nullptr };
  } catch(const std::exception &e) {
    return { kGenericError, ::strdup(e.what()) };
  }
}


Response policer_contract_list_by_id_help(
  const char *name, const Key *keys, size_t num_keys) {
  // Getting the path params
  std::string unique_name { name };
  nlohmann::json val = read_policer_contract_list_by_id_get_list(unique_name);

  return { kOk, ::strdup(val.dump().c_str()) };
}

Response policer_list_by_id_help(
  const char *name, const Key *keys, size_t num_keys) {

  nlohmann::json val = read_policer_list_by_id_get_list();

  return { kOk, ::strdup(val.dump().c_str()) };
}

#ifdef __cplusplus
}
#endif
