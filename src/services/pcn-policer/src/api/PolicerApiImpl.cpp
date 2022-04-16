/**
* policer API generated from policer.yang
*
* NOTE: This file is auto generated by polycube-codegen
* https://github.com/polycube-network/polycube-codegen
*/


/* Do not edit this file manually */


#include "PolicerApiImpl.h"

namespace polycube {
namespace service {
namespace api {

using namespace polycube::service::model;

namespace PolicerApiImpl {
namespace {
std::unordered_map<std::string, std::shared_ptr<Policer>> cubes;
std::mutex cubes_mutex;

std::shared_ptr<Policer> get_cube(const std::string &name) {
  std::lock_guard<std::mutex> guard(cubes_mutex);
  auto iter = cubes.find(name);
  if (iter == cubes.end()) {
    throw std::runtime_error("Cube " + name + " does not exist");
  }

  return iter->second;
}

}

void create_policer_by_id(const std::string &name, const PolicerJsonObject &jsonObject) {
  {
    // check if name is valid before creating it
    std::lock_guard<std::mutex> guard(cubes_mutex);
    if (cubes.count(name) != 0) {
      throw std::runtime_error("There is already a cube with name " + name);
    }
  }
  auto ptr = std::make_shared<Policer>(name, jsonObject);
  std::unordered_map<std::string, std::shared_ptr<Policer>>::iterator iter;
  bool inserted;

  std::lock_guard<std::mutex> guard(cubes_mutex);
  std::tie(iter, inserted) = cubes.emplace(name, std::move(ptr));

  if (!inserted) {
    throw std::runtime_error("There is already a cube with name " + name);
  }
}

void replace_policer_by_id(const std::string &name, const PolicerJsonObject &bridge){
  throw std::runtime_error("Method not supported!");
}

void delete_policer_by_id(const std::string &name) {
  std::lock_guard<std::mutex> guard(cubes_mutex);
  if (cubes.count(name) == 0) {
    throw std::runtime_error("Cube " + name + " does not exist");
  }
  cubes.erase(name);
}

std::vector<PolicerJsonObject> read_policer_list_by_id() {
  std::vector<PolicerJsonObject> jsonObject_vect;
  for(auto &i : cubes) {
    auto m = get_cube(i.first);
    jsonObject_vect.push_back(m->toJsonObject());
  }
  return jsonObject_vect;
}

std::vector<nlohmann::fifo_map<std::string, std::string>> read_policer_list_by_id_get_list() {
  std::vector<nlohmann::fifo_map<std::string, std::string>> r;
  for (auto &x : cubes) {
    nlohmann::fifo_map<std::string, std::string> m;
    m["name"] = x.first;
    r.push_back(std::move(m));
  }
  return r;
}

/**
* @brief   Create contract by ID
*
* Create operation of resource: contract*
*
* @param[in] name ID of name
* @param[in] trafficClass ID of traffic-class
* @param[in] value contractbody object
*
* Responses:
*
*/
void
create_policer_contract_by_id(const std::string &name, const uint32_t &trafficClass, const ContractJsonObject &value) {
  auto policer = get_cube(name);

  return policer->addContract(trafficClass, value);
}

/**
* @brief   Create contract by ID
*
* Create operation of resource: contract*
*
* @param[in] name ID of name
* @param[in] value contractbody object
*
* Responses:
*
*/
void
create_policer_contract_list_by_id(const std::string &name, const std::vector<ContractJsonObject> &value) {
  auto policer = get_cube(name);
  policer->addContractList(value);
}

/**
* @brief   Create update-data by ID
*
* Create operation of resource: update-data*
*
* @param[in] name ID of name
* @param[in] trafficClass ID of traffic-class
* @param[in] value update-databody object
*
* Responses:
*
*/
void
create_policer_contract_update_data_by_id(const std::string &name, const uint32_t &trafficClass, const ContractUpdateDataInputJsonObject &value) {
  auto policer = get_cube(name);
  auto contract = policer->getContract(trafficClass);

  return contract->updateData(value);
}

/**
* @brief   Create default-contract by ID
*
* Create operation of resource: default-contract*
*
* @param[in] name ID of name
* @param[in] value default-contractbody object
*
* Responses:
*
*/
void
create_policer_default_contract_by_id(const std::string &name, const DefaultContractJsonObject &value) {
  auto policer = get_cube(name);

  return policer->addDefaultContract(value);
}

/**
* @brief   Create update-data by ID
*
* Create operation of resource: update-data*
*
* @param[in] name ID of name
* @param[in] value update-databody object
*
* Responses:
*
*/
void
create_policer_default_contract_update_data_by_id(const std::string &name, const DefaultContractUpdateDataInputJsonObject &value) {
  auto policer = get_cube(name);
  auto defaultContract = policer->getDefaultContract();

  return defaultContract->updateData(value);
}

/**
* @brief   Delete contract by ID
*
* Delete operation of resource: contract*
*
* @param[in] name ID of name
* @param[in] trafficClass ID of traffic-class
*
* Responses:
*
*/
void
delete_policer_contract_by_id(const std::string &name, const uint32_t &trafficClass) {
  auto policer = get_cube(name);

  return policer->delContract(trafficClass);
}

/**
* @brief   Delete contract by ID
*
* Delete operation of resource: contract*
*
* @param[in] name ID of name
*
* Responses:
*
*/
void
delete_policer_contract_list_by_id(const std::string &name) {
  auto policer = get_cube(name);
  policer->delContractList();
}

/**
* @brief   Delete default-contract by ID
*
* Delete operation of resource: default-contract*
*
* @param[in] name ID of name
*
* Responses:
*
*/
void
delete_policer_default_contract_by_id(const std::string &name) {
  auto policer = get_cube(name);

  return policer->delDefaultContract();
}

/**
* @brief   Read policer by ID
*
* Read operation of resource: policer*
*
* @param[in] name ID of name
*
* Responses:
* PolicerJsonObject
*/
PolicerJsonObject
read_policer_by_id(const std::string &name) {
  return get_cube(name)->toJsonObject();

}

/**
* @brief   Read action by ID
*
* Read operation of resource: action*
*
* @param[in] name ID of name
* @param[in] trafficClass ID of traffic-class
*
* Responses:
* ActionTypeEnum
*/
ActionTypeEnum
read_policer_contract_action_by_id(const std::string &name, const uint32_t &trafficClass) {
  auto policer = get_cube(name);
  auto contract = policer->getContract(trafficClass);
  return contract->getAction();

}

/**
* @brief   Read burst-limit by ID
*
* Read operation of resource: burst-limit*
*
* @param[in] name ID of name
* @param[in] trafficClass ID of traffic-class
*
* Responses:
* uint64_t
*/
uint64_t
read_policer_contract_burst_limit_by_id(const std::string &name, const uint32_t &trafficClass) {
  auto policer = get_cube(name);
  auto contract = policer->getContract(trafficClass);
  return contract->getBurstLimit();

}

/**
* @brief   Read contract by ID
*
* Read operation of resource: contract*
*
* @param[in] name ID of name
* @param[in] trafficClass ID of traffic-class
*
* Responses:
* ContractJsonObject
*/
ContractJsonObject
read_policer_contract_by_id(const std::string &name, const uint32_t &trafficClass) {
  auto policer = get_cube(name);
  return policer->getContract(trafficClass)->toJsonObject();

}

/**
* @brief   Read contract by ID
*
* Read operation of resource: contract*
*
* @param[in] name ID of name
*
* Responses:
* std::vector<ContractJsonObject>
*/
std::vector<ContractJsonObject>
read_policer_contract_list_by_id(const std::string &name) {
  auto policer = get_cube(name);
  auto &&contract = policer->getContractList();
  std::vector<ContractJsonObject> m;
  for(auto &i : contract)
    m.push_back(i->toJsonObject());
  return m;
}

/**
* @brief   Read rate-limit by ID
*
* Read operation of resource: rate-limit*
*
* @param[in] name ID of name
* @param[in] trafficClass ID of traffic-class
*
* Responses:
* uint64_t
*/
uint64_t
read_policer_contract_rate_limit_by_id(const std::string &name, const uint32_t &trafficClass) {
  auto policer = get_cube(name);
  auto contract = policer->getContract(trafficClass);
  return contract->getRateLimit();

}

/**
* @brief   Read action by ID
*
* Read operation of resource: action*
*
* @param[in] name ID of name
*
* Responses:
* ActionTypeEnum
*/
ActionTypeEnum
read_policer_default_contract_action_by_id(const std::string &name) {
  auto policer = get_cube(name);
  auto defaultContract = policer->getDefaultContract();
  return defaultContract->getAction();

}

/**
* @brief   Read burst-limit by ID
*
* Read operation of resource: burst-limit*
*
* @param[in] name ID of name
*
* Responses:
* uint64_t
*/
uint64_t
read_policer_default_contract_burst_limit_by_id(const std::string &name) {
  auto policer = get_cube(name);
  auto defaultContract = policer->getDefaultContract();
  return defaultContract->getBurstLimit();

}

/**
* @brief   Read default-contract by ID
*
* Read operation of resource: default-contract*
*
* @param[in] name ID of name
*
* Responses:
* DefaultContractJsonObject
*/
DefaultContractJsonObject
read_policer_default_contract_by_id(const std::string &name) {
  auto policer = get_cube(name);
  return policer->getDefaultContract()->toJsonObject();

}

/**
* @brief   Read rate-limit by ID
*
* Read operation of resource: rate-limit*
*
* @param[in] name ID of name
*
* Responses:
* uint64_t
*/
uint64_t
read_policer_default_contract_rate_limit_by_id(const std::string &name) {
  auto policer = get_cube(name);
  auto defaultContract = policer->getDefaultContract();
  return defaultContract->getRateLimit();

}

/**
* @brief   Replace contract by ID
*
* Replace operation of resource: contract*
*
* @param[in] name ID of name
* @param[in] trafficClass ID of traffic-class
* @param[in] value contractbody object
*
* Responses:
*
*/
void
replace_policer_contract_by_id(const std::string &name, const uint32_t &trafficClass, const ContractJsonObject &value) {
  auto policer = get_cube(name);

  return policer->replaceContract(trafficClass, value);
}

/**
* @brief   Replace contract by ID
*
* Replace operation of resource: contract*
*
* @param[in] name ID of name
* @param[in] value contractbody object
*
* Responses:
*
*/
void
replace_policer_contract_list_by_id(const std::string &name, const std::vector<ContractJsonObject> &value) {
  throw std::runtime_error("Method not supported");
}

/**
* @brief   Replace default-contract by ID
*
* Replace operation of resource: default-contract*
*
* @param[in] name ID of name
* @param[in] value default-contractbody object
*
* Responses:
*
*/
void
replace_policer_default_contract_by_id(const std::string &name, const DefaultContractJsonObject &value) {
  auto policer = get_cube(name);

  return policer->replaceDefaultContract(value);
}

/**
* @brief   Update policer by ID
*
* Update operation of resource: policer*
*
* @param[in] name ID of name
* @param[in] value policerbody object
*
* Responses:
*
*/
void
update_policer_by_id(const std::string &name, const PolicerJsonObject &value) {
  auto policer = get_cube(name);

  return policer->update(value);
}

/**
* @brief   Update contract by ID
*
* Update operation of resource: contract*
*
* @param[in] name ID of name
* @param[in] trafficClass ID of traffic-class
* @param[in] value contractbody object
*
* Responses:
*
*/
void
update_policer_contract_by_id(const std::string &name, const uint32_t &trafficClass, const ContractJsonObject &value) {
  auto policer = get_cube(name);
  auto contract = policer->getContract(trafficClass);

  return contract->update(value);
}

/**
* @brief   Update contract by ID
*
* Update operation of resource: contract*
*
* @param[in] name ID of name
* @param[in] value contractbody object
*
* Responses:
*
*/
void
update_policer_contract_list_by_id(const std::string &name, const std::vector<ContractJsonObject> &value) {
  throw std::runtime_error("Method not supported");
}

/**
* @brief   Update default-contract by ID
*
* Update operation of resource: default-contract*
*
* @param[in] name ID of name
* @param[in] value default-contractbody object
*
* Responses:
*
*/
void
update_policer_default_contract_by_id(const std::string &name, const DefaultContractJsonObject &value) {
  auto policer = get_cube(name);
  auto defaultContract = policer->getDefaultContract();

  return defaultContract->update(value);
}

/**
* @brief   Update policer by ID
*
* Update operation of resource: policer*
*
* @param[in] value policerbody object
*
* Responses:
*
*/
void
update_policer_list_by_id(const std::vector<PolicerJsonObject> &value) {
  throw std::runtime_error("Method not supported");
}



/*
 * help related
 */

std::vector<nlohmann::fifo_map<std::string, std::string>> read_policer_contract_list_by_id_get_list(const std::string &name) {
  std::vector<nlohmann::fifo_map<std::string, std::string>> r;
  auto &&policer = get_cube(name);

  auto &&contract = policer->getContractList();
  for(auto &i : contract) {
    nlohmann::fifo_map<std::string, std::string> keys;

    keys["trafficClass"] = std::to_string(i->getTrafficClass());

    r.push_back(keys);
  }
  return r;
}


}

}
}
}

