/**
* gtphandler API generated from gtphandler.yang
*
* NOTE: This file is auto generated by polycube-codegen
* https://github.com/polycube-network/polycube-codegen
*/


/* Do not edit this file manually */


#include "GtphandlerApi.h"
#include "GtphandlerApiImpl.h"

using namespace polycube::service::model;
using namespace polycube::service::api::GtphandlerApiImpl;

#ifdef __cplusplus
extern "C" {
#endif

Response create_gtphandler_by_id_handler(
  const char *name, const Key *keys,
  size_t num_keys ,
  const char *value) {
  // Getting the path params
  std::string unique_name { name };

  try {
    auto request_body = nlohmann::json::parse(std::string { value });
    // Getting the body param
    GtphandlerJsonObject unique_value { request_body };

    unique_value.setName(unique_name);
    create_gtphandler_by_id(unique_name, unique_value);
    return { kCreated, nullptr };
  } catch(const std::exception &e) {
    return { kGenericError, ::strdup(e.what()) };
  }
}

Response create_gtphandler_user_equipment_by_id_handler(
  const char *name, const Key *keys,
  size_t num_keys ,
  const char *value) {
  // Getting the path params
  std::string unique_name { name };
  std::string unique_ip;
  for (size_t i = 0; i < num_keys; ++i) {
    if (!strcmp(keys[i].name, "ip")) {
      unique_ip = std::string { keys[i].value.string };
      break;
    }
  }


  try {
    auto request_body = nlohmann::json::parse(std::string { value });
    // Getting the body param
    UserEquipmentJsonObject unique_value { request_body };

    unique_value.setIp(unique_ip);
    create_gtphandler_user_equipment_by_id(unique_name, unique_ip, unique_value);
    return { kCreated, nullptr };
  } catch(const std::exception &e) {
    return { kGenericError, ::strdup(e.what()) };
  }
}

Response create_gtphandler_user_equipment_list_by_id_handler(
  const char *name, const Key *keys,
  size_t num_keys ,
  const char *value) {
  // Getting the path params
  std::string unique_name { name };
  // Getting the body param
  std::vector<UserEquipmentJsonObject> unique_value;

  try {
    auto request_body = nlohmann::json::parse(std::string { value });
    // Getting the body param
    std::vector<UserEquipmentJsonObject> unique_value;
    for (auto &j : request_body) {
      UserEquipmentJsonObject a { j };
      unique_value.push_back(a);
    }
    create_gtphandler_user_equipment_list_by_id(unique_name, unique_value);
    return { kCreated, nullptr };
  } catch(const std::exception &e) {
    return { kGenericError, ::strdup(e.what()) };
  }
}

Response delete_gtphandler_by_id_handler(
  const char *name, const Key *keys,
  size_t num_keys ) {
  // Getting the path params
  std::string unique_name { name };

  try {
    delete_gtphandler_by_id(unique_name);
    return { kOk, nullptr };
  } catch(const std::exception &e) {
    return { kGenericError, ::strdup(e.what()) };
  }
}

Response delete_gtphandler_user_equipment_by_id_handler(
  const char *name, const Key *keys,
  size_t num_keys ) {
  // Getting the path params
  std::string unique_name { name };
  std::string unique_ip;
  for (size_t i = 0; i < num_keys; ++i) {
    if (!strcmp(keys[i].name, "ip")) {
      unique_ip = std::string { keys[i].value.string };
      break;
    }
  }


  try {
    delete_gtphandler_user_equipment_by_id(unique_name, unique_ip);
    return { kOk, nullptr };
  } catch(const std::exception &e) {
    return { kGenericError, ::strdup(e.what()) };
  }
}

Response delete_gtphandler_user_equipment_list_by_id_handler(
  const char *name, const Key *keys,
  size_t num_keys ) {
  // Getting the path params
  std::string unique_name { name };

  try {
    delete_gtphandler_user_equipment_list_by_id(unique_name);
    return { kOk, nullptr };
  } catch(const std::exception &e) {
    return { kGenericError, ::strdup(e.what()) };
  }
}

Response read_gtphandler_by_id_handler(
  const char *name, const Key *keys,
  size_t num_keys ) {
  // Getting the path params
  std::string unique_name { name };

  try {

    auto x = read_gtphandler_by_id(unique_name);
    nlohmann::json response_body;
    response_body = x.toJson();
    return { kOk, ::strdup(response_body.dump().c_str()) };
  } catch(const std::exception &e) {
    return { kGenericError, ::strdup(e.what()) };
  }
}

Response read_gtphandler_list_by_id_handler(
  const char *name, const Key *keys,
  size_t num_keys ) {


  try {

    auto x = read_gtphandler_list_by_id();
    nlohmann::json response_body;
    for (auto &i : x) {
      response_body += i.toJson();
    }
    return { kOk, ::strdup(response_body.dump().c_str()) };
  } catch(const std::exception &e) {
    return { kGenericError, ::strdup(e.what()) };
  }
}

Response read_gtphandler_user_equipment_by_id_handler(
  const char *name, const Key *keys,
  size_t num_keys ) {
  // Getting the path params
  std::string unique_name { name };
  std::string unique_ip;
  for (size_t i = 0; i < num_keys; ++i) {
    if (!strcmp(keys[i].name, "ip")) {
      unique_ip = std::string { keys[i].value.string };
      break;
    }
  }


  try {

    auto x = read_gtphandler_user_equipment_by_id(unique_name, unique_ip);
    nlohmann::json response_body;
    response_body = x.toJson();
    return { kOk, ::strdup(response_body.dump().c_str()) };
  } catch(const std::exception &e) {
    return { kGenericError, ::strdup(e.what()) };
  }
}

Response read_gtphandler_user_equipment_list_by_id_handler(
  const char *name, const Key *keys,
  size_t num_keys ) {
  // Getting the path params
  std::string unique_name { name };

  try {

    auto x = read_gtphandler_user_equipment_list_by_id(unique_name);
    nlohmann::json response_body;
    for (auto &i : x) {
      response_body += i.toJson();
    }
    return { kOk, ::strdup(response_body.dump().c_str()) };
  } catch(const std::exception &e) {
    return { kGenericError, ::strdup(e.what()) };
  }
}

Response read_gtphandler_user_equipment_teid_by_id_handler(
  const char *name, const Key *keys,
  size_t num_keys ) {
  // Getting the path params
  std::string unique_name { name };
  std::string unique_ip;
  for (size_t i = 0; i < num_keys; ++i) {
    if (!strcmp(keys[i].name, "ip")) {
      unique_ip = std::string { keys[i].value.string };
      break;
    }
  }


  try {

    auto x = read_gtphandler_user_equipment_teid_by_id(unique_name, unique_ip);
    nlohmann::json response_body;
    response_body = x;
    return { kOk, ::strdup(response_body.dump().c_str()) };
  } catch(const std::exception &e) {
    return { kGenericError, ::strdup(e.what()) };
  }
}

Response read_gtphandler_user_equipment_tunnel_endpoint_by_id_handler(
  const char *name, const Key *keys,
  size_t num_keys ) {
  // Getting the path params
  std::string unique_name { name };
  std::string unique_ip;
  for (size_t i = 0; i < num_keys; ++i) {
    if (!strcmp(keys[i].name, "ip")) {
      unique_ip = std::string { keys[i].value.string };
      break;
    }
  }


  try {

    auto x = read_gtphandler_user_equipment_tunnel_endpoint_by_id(unique_name, unique_ip);
    nlohmann::json response_body;
    response_body = x;
    return { kOk, ::strdup(response_body.dump().c_str()) };
  } catch(const std::exception &e) {
    return { kGenericError, ::strdup(e.what()) };
  }
}

Response replace_gtphandler_by_id_handler(
  const char *name, const Key *keys,
  size_t num_keys ,
  const char *value) {
  // Getting the path params
  std::string unique_name { name };

  try {
    auto request_body = nlohmann::json::parse(std::string { value });
    // Getting the body param
    GtphandlerJsonObject unique_value { request_body };

    unique_value.setName(unique_name);
    replace_gtphandler_by_id(unique_name, unique_value);
    return { kOk, nullptr };
  } catch(const std::exception &e) {
    return { kGenericError, ::strdup(e.what()) };
  }
}

Response replace_gtphandler_user_equipment_by_id_handler(
  const char *name, const Key *keys,
  size_t num_keys ,
  const char *value) {
  // Getting the path params
  std::string unique_name { name };
  std::string unique_ip;
  for (size_t i = 0; i < num_keys; ++i) {
    if (!strcmp(keys[i].name, "ip")) {
      unique_ip = std::string { keys[i].value.string };
      break;
    }
  }


  try {
    auto request_body = nlohmann::json::parse(std::string { value });
    // Getting the body param
    UserEquipmentJsonObject unique_value { request_body };

    unique_value.setIp(unique_ip);
    replace_gtphandler_user_equipment_by_id(unique_name, unique_ip, unique_value);
    return { kOk, nullptr };
  } catch(const std::exception &e) {
    return { kGenericError, ::strdup(e.what()) };
  }
}

Response replace_gtphandler_user_equipment_list_by_id_handler(
  const char *name, const Key *keys,
  size_t num_keys ,
  const char *value) {
  // Getting the path params
  std::string unique_name { name };
  // Getting the body param
  std::vector<UserEquipmentJsonObject> unique_value;

  try {
    auto request_body = nlohmann::json::parse(std::string { value });
    // Getting the body param
    std::vector<UserEquipmentJsonObject> unique_value;
    for (auto &j : request_body) {
      UserEquipmentJsonObject a { j };
      unique_value.push_back(a);
    }
    replace_gtphandler_user_equipment_list_by_id(unique_name, unique_value);
    return { kOk, nullptr };
  } catch(const std::exception &e) {
    return { kGenericError, ::strdup(e.what()) };
  }
}

Response update_gtphandler_by_id_handler(
  const char *name, const Key *keys,
  size_t num_keys ,
  const char *value) {
  // Getting the path params
  std::string unique_name { name };

  try {
    auto request_body = nlohmann::json::parse(std::string { value });
    // Getting the body param
    GtphandlerJsonObject unique_value { request_body };

    unique_value.setName(unique_name);
    update_gtphandler_by_id(unique_name, unique_value);
    return { kOk, nullptr };
  } catch(const std::exception &e) {
    return { kGenericError, ::strdup(e.what()) };
  }
}

Response update_gtphandler_list_by_id_handler(
  const char *name, const Key *keys,
  size_t num_keys ,
  const char *value) {

  // Getting the body param
  std::vector<GtphandlerJsonObject> unique_value;

  try {
    auto request_body = nlohmann::json::parse(std::string { value });
    // Getting the body param
    std::vector<GtphandlerJsonObject> unique_value;
    for (auto &j : request_body) {
      GtphandlerJsonObject a { j };
      unique_value.push_back(a);
    }
    update_gtphandler_list_by_id(unique_value);
    return { kOk, nullptr };
  } catch(const std::exception &e) {
    return { kGenericError, ::strdup(e.what()) };
  }
}

Response update_gtphandler_user_equipment_by_id_handler(
  const char *name, const Key *keys,
  size_t num_keys ,
  const char *value) {
  // Getting the path params
  std::string unique_name { name };
  std::string unique_ip;
  for (size_t i = 0; i < num_keys; ++i) {
    if (!strcmp(keys[i].name, "ip")) {
      unique_ip = std::string { keys[i].value.string };
      break;
    }
  }


  try {
    auto request_body = nlohmann::json::parse(std::string { value });
    // Getting the body param
    UserEquipmentJsonObject unique_value { request_body };

    unique_value.setIp(unique_ip);
    update_gtphandler_user_equipment_by_id(unique_name, unique_ip, unique_value);
    return { kOk, nullptr };
  } catch(const std::exception &e) {
    return { kGenericError, ::strdup(e.what()) };
  }
}

Response update_gtphandler_user_equipment_list_by_id_handler(
  const char *name, const Key *keys,
  size_t num_keys ,
  const char *value) {
  // Getting the path params
  std::string unique_name { name };
  // Getting the body param
  std::vector<UserEquipmentJsonObject> unique_value;

  try {
    auto request_body = nlohmann::json::parse(std::string { value });
    // Getting the body param
    std::vector<UserEquipmentJsonObject> unique_value;
    for (auto &j : request_body) {
      UserEquipmentJsonObject a { j };
      unique_value.push_back(a);
    }
    update_gtphandler_user_equipment_list_by_id(unique_name, unique_value);
    return { kOk, nullptr };
  } catch(const std::exception &e) {
    return { kGenericError, ::strdup(e.what()) };
  }
}

Response update_gtphandler_user_equipment_teid_by_id_handler(
  const char *name, const Key *keys,
  size_t num_keys ,
  const char *value) {
  // Getting the path params
  std::string unique_name { name };
  std::string unique_ip;
  for (size_t i = 0; i < num_keys; ++i) {
    if (!strcmp(keys[i].name, "ip")) {
      unique_ip = std::string { keys[i].value.string };
      break;
    }
  }


  try {
    auto request_body = nlohmann::json::parse(std::string { value });
    // The conversion is done automatically by the json library
    uint32_t unique_value = request_body;
    update_gtphandler_user_equipment_teid_by_id(unique_name, unique_ip, unique_value);
    return { kOk, nullptr };
  } catch(const std::exception &e) {
    return { kGenericError, ::strdup(e.what()) };
  }
}

Response update_gtphandler_user_equipment_tunnel_endpoint_by_id_handler(
  const char *name, const Key *keys,
  size_t num_keys ,
  const char *value) {
  // Getting the path params
  std::string unique_name { name };
  std::string unique_ip;
  for (size_t i = 0; i < num_keys; ++i) {
    if (!strcmp(keys[i].name, "ip")) {
      unique_ip = std::string { keys[i].value.string };
      break;
    }
  }


  try {
    auto request_body = nlohmann::json::parse(std::string { value });
    // The conversion is done automatically by the json library
    std::string unique_value = request_body;
    update_gtphandler_user_equipment_tunnel_endpoint_by_id(unique_name, unique_ip, unique_value);
    return { kOk, nullptr };
  } catch(const std::exception &e) {
    return { kGenericError, ::strdup(e.what()) };
  }
}


Response gtphandler_list_by_id_help(
  const char *name, const Key *keys, size_t num_keys) {

  nlohmann::json val = read_gtphandler_list_by_id_get_list();

  return { kOk, ::strdup(val.dump().c_str()) };
}

Response gtphandler_user_equipment_list_by_id_help(
  const char *name, const Key *keys, size_t num_keys) {
  // Getting the path params
  std::string unique_name { name };
  nlohmann::json val = read_gtphandler_user_equipment_list_by_id_get_list(unique_name);

  return { kOk, ::strdup(val.dump().c_str()) };
}

#ifdef __cplusplus
}
#endif

