/*
 * Copyright 2018 The Polycube Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Modify these methods with your own implementation

#include "Service.h"
#include "Lbrp.h"

#include <functional>
#include <iostream>
#include <random>

using namespace polycube::service;

const std::string Service::EBPF_SERVICE_MAP = "services";
const std::string Service::EBPF_BACKEND_TO_SERVICE_MAP = "backend_to_service";
const uint16_t Service::ICMP_EBPF_PORT = 0;
const uint Service::INITIAL_BACKEND_SIZE = 10;
const uint Service::BACKEND_REPLICAS = 3;

Service::Service(Lbrp &parent, const ServiceJsonObject &conf)
    : parent_(parent), backend_size_(INITIAL_BACKEND_SIZE) {
  logger()->info("Creating Service instance");

  vip_ = conf.getVip();
  vport_ = conf.getVport();
  proto_ = conf.getProto();

  if (conf.nameIsSet()) {
    setName(conf.getName());
  }

  // FIXME: There is another option here instead of creating every backend at
  // time
  // We could just add the backend to the matrix map and call the
  // updateConsistentHashMap
  // only at the end. In this way it will be called only once.
  // Now this function is called every time a new backend is added
  addBackendList(conf.getBackend());
}

Service::~Service() {}

void Service::update(const ServiceJsonObject &conf) {
  // This method updates all the object/parameter in Service object specified in
  // the conf JsonObject.
  // You can modify this implementation.

  if (conf.nameIsSet()) {
    setName(conf.getName());
  }

  if (conf.backendIsSet()) {
    for (auto &i : conf.getBackend()) {
      auto ip = i.getIp();
      auto m = getBackend(ip);
      m->update(i);
    }
  }
}

ServiceJsonObject Service::toJsonObject() {
  ServiceJsonObject conf;

  conf.setVip(getVip());
  conf.setProto(getProto());
  conf.setName(getName());
  conf.setVport(getVport());

  for (auto &i : getBackendList()) {
    conf.addServiceBackend(i->toJsonObject());
  }

  return conf;
}

std::string Service::getVip() {
  // This method retrieves the vip value.
  return vip_;
}

ServiceProtoEnum Service::getProto() {
  // This method retrieves the proto value.
  return proto_;
}

std::string Service::getName() {
  // This method retrieves the name value.
  return service_name_;
}

void Service::setName(const std::string &value) {
  // This method set the name value.
  service_name_ = value;
}

uint16_t Service::getVport() {
  // This method retrieves the vport value.
  return vport_;
}

/*
 * This method removes all entries for a service in the datapath map
 */

void Service::removeServiceFromKernelMap() {
  logger()->trace("Removing all elements from service map");

  auto services_table = parent_.get_hash_table<vip, backend>(EBPF_SERVICE_MAP);

  for (uint16_t index = 0; index < backend_size_ * BACKEND_REPLICAS; index++) {
    vip service_key{
        .ip = utils::ip_string_to_nbo_uint(getVip()),
        .port = htons(getVport()),
        .proto = htons(Service::convertProtoToNumber(getProto())),
        .index = index,
    };

    services_table.remove(service_key);
  }
}

void Service::removeBackendFromServiceMatrix(std::string backend_ip) {
  int bkd_size = std::max(backend_size_, (uint)service_backends_.size());
  int vect_size = bkd_size * BACKEND_REPLICAS;
  backend_matrix_[backend_ip] = getEmptyIntVector(vect_size);
}

void Service::addBackendToServiceMatrix(std::string backend_ip) {
  int bkd_size = std::max(backend_size_, (uint)service_backends_.size());
  int vect_size = bkd_size * BACKEND_REPLICAS;
  backend_matrix_[backend_ip] = getRandomIntVector(vect_size);
  if (getProto() != ServiceProtoEnum::ICMP) {
    // updatesrcIpRewriteMap(backend_ip);
    // update_mapping_bcksIP(backend_ip);
  }
}

void Service::updateConsistentHashMap() {
  logger()->debug(
      "[Service] Updating consistent hash map for service {0}, {1}, {2}",
      getVip(), getVport(),
      ServiceJsonObject::ServiceProtoEnum_to_string(getProto()));

  std::vector<std::string> consistent_array = getConsistentArray();

  updateKernelServiceMap(std::move(consistent_array));
}

/*
 * The following function is used to load and update services map in the
 * datapath.
 * The map has :
 * 	-key = struct vip: {ip, port, proto, index}
 * 	-value = struct backend: {ip, port}
 */

void Service::updateKernelServiceMap(
    const std::vector<std::string> consistent_array) {
  auto services_table = parent_.get_hash_table<vip, backend>(EBPF_SERVICE_MAP);
  auto backend_to_service =
      parent_.get_hash_table<backend, vip>(EBPF_BACKEND_TO_SERVICE_MAP);
  uint16_t index = 0;

  vip service_key{
      .ip = utils::ip_string_to_nbo_uint(getVip()),
      .port = htons(getVport()),
      .proto = htons(Service::convertProtoToNumber(getProto())),
      .index = index,
  };

  // This is a particular value that is used to indicate the size of the backend
  // pool.
  // In doesn't indicate a real backend
  backend service_value{
      .ip = utils::ip_string_to_nbo_uint(getVip()),
      .port = uint16_t(consistent_array.size()),
      .proto = 0,
  };

  services_table.set(service_key, service_value);

  for (const auto &backend_ip : consistent_array) {
    index++;

    auto bck = getBackend(backend_ip);

    vip key{
        .ip = utils::ip_string_to_nbo_uint(getVip()),
        .port = htons(getVport()),
        .proto = htons(Service::convertProtoToNumber(getProto())),
        .index = index,
    };

    backend value{
        .ip = utils::ip_string_to_nbo_uint(bck->getIp()),
        .port = htons(bck->getPort()),
        .proto = htons(Service::convertProtoToNumber(getProto())),
    };

    services_table.set(key, value);
    backend_to_service.set(value, key);
  }

  logger()->debug("[Service] Service map updated");
}

/*
 * This method returns the consistent array of the service's backends
 */
std::vector<std::string> Service::getConsistentArray() {
  std::vector<std::string> backend_list;
  std::vector<std::vector<int>> backend_matrix_list;
  int bkd_size = std::max(backend_size_, (uint)service_backends_.size());
  int array_rows = bkd_size * BACKEND_REPLICAS;

  for (const auto &s : backend_matrix_) {
    backend_list.push_back(s.first);
    backend_matrix_list.push_back(s.second);
  }

  // Declare the final backend list vector and initialize all elems to -1
  std::vector<std::string> final_backend_list(array_rows, "-1");
  // Retrieve how much entries for each backend
  std::map<std::string, int> weight_backend_list = get_weight_backend();

  int index = 0;
  int count = 0;

  /* We have to  scroll a matrix , starting from first column , first row [0,0].
   *For each cell , i retrieve the number stored in that cell and lookup in the
   *final_backend_list if it exists or
   * not , if not , it is saved in this array(final_backend_list) and we go on
   *in the matrix by checking the next cell [0,1] row0 column 1 ;
   * if Yes, we ho to the next row -> [1,0] , retrieve the number in this cell ,
   *lookup in the final_backend_list and so on ;
   * All of this continue until the final_backend_list is filed;
   * Each Matrix's column stores a range of number from 0 to bkd_size
   * Matrix cells can have 2 types of number :
   * 1) -2  -> go next colum (continue command to go next for loop and x++ keep
   *allign matrix column and vector of backend's name)
   * 2) X>0 -> check if u can store it in the final_backend_list
   * final_backend_list has 2 types of numbers:
   * 1) -1  -> u can store the number , in this case i check if the entry can be
   *assigned to this backend by checking the number in
   *weight_backend_list[backend_list.at(x)] ; if it is greater
   *	than 0, it is stored and the number is decreased
   *weight_backend_list[backend_list.at(x)]--;
   * 2) X>0 -> go next row (index++)
   */

  for (int j = 0; j < array_rows; j++) {
    int x = 0;
    index = j;
    for (const auto &elem : backend_matrix_list) {
      if (count == array_rows) {
        goto exit;
      }

    ret:
      int num = elem[index];
      if (num == -2) {
        x++;
        continue;
      } else if (final_backend_list.at(num) != "-1") {
        index++;
        if (index >= array_rows) {
          x++;
          index = j;
          continue;
        }
        goto ret;
      } else if (final_backend_list.at(num) == "-1") {
        if (weight_backend_list[backend_list.at(x)] > 0) {
          final_backend_list[num] = backend_list.at(x);
          count++;
          index = j;
          weight_backend_list[backend_list.at(x)]--;
          x++;
        } else {
          x++;
          index = j;
        }
      }
    }
  }

exit:
  return final_backend_list;
}

/*
 * This function return a map<backend_ip,#entries_assigned> after checking
 backend's weights.
    We need to calculate the common coefficient between all backends.
    Then we multiply this coefficient for backend's weight in order to get the
 number of entries assigned to this backend.
 */
std::map<std::string, int> Service::get_weight_backend() {
  int bkd_size = std::max(backend_size_, (uint)service_backends_.size());
  int vect_size = bkd_size * BACKEND_REPLICAS;
  int total_weight = 0;
  int sum_weight = 0;
  std::map<std::string, int> weight_back_pool;

  // Calculate the total of weights
  for (auto &backend : service_backends_) {
    sum_weight += backend.second.getWeight();
  }
  float coeff = (static_cast<float>(vect_size) / sum_weight);
  logger()->debug("vec size {0} sum_weight {1} coeff {2} ", vect_size,
                  sum_weight, coeff);

  // Calculate ,for each backend, its number of entries
  for (auto &backend : service_backends_) {
    if (backend.second.getWeight() == 0)
      continue;
    int weight_backend = (backend.second.getWeight()) * (coeff);
    if (weight_backend == 0)
      weight_backend++;
    weight_back_pool[backend.first] = weight_backend;
    total_weight += weight_backend;
    logger()->debug("backend {0} weight {1} ", backend.first, weight_backend);
  }

  int res = vect_size - total_weight;

  // Allign all , if necessary
  while (res > 0) {
    for (auto &backend : service_backends_) {
      if (backend.second.getWeight() == 0)
        continue;

      weight_back_pool[backend.first]++;
      res--;

      logger()->debug("backend {0} weight {1} ", backend.first,
                      weight_back_pool[backend.first]);

      if (res == 0)
        break;
    }
  }

  return weight_back_pool;
}

std::vector<int> Service::getEmptyIntVector(int vect_size) {
  // Initialize the entire vector with the value -2
  std::vector<int> final_vect(vect_size, -2);

  return final_vect;
}

std::vector<int> Service::getRandomIntVector(int vect_size) {
  std::vector<int> final_vect(vect_size);
  // Fill the vector with number starting from 0 to vect_size
  std::iota(std::begin(final_vect), std::end(final_vect), 0);

  std::random_device rand;
  std::mt19937 engine(rand());

  std::shuffle(std::begin(final_vect), std::end(final_vect), engine);

  return final_vect;
}

uint8_t Service::convertProtoToNumber(const ServiceProtoEnum &proto) {
  switch (proto) {
  case ServiceProtoEnum::ICMP:
    return 1;
  case ServiceProtoEnum::TCP:
    return 6;
  case ServiceProtoEnum::UDP:
    return 17;
  case ServiceProtoEnum::ALL:
    // 143-252 is in the Unassigned IANA protocol numbers
    return 143;
  }
}

ServiceProtoEnum Service::convertNumberToProto(const uint8_t proto) {
  if (proto == 1)
    return ServiceProtoEnum::ICMP;

  if (proto == 6)
    return ServiceProtoEnum::TCP;

  if (proto == 17)
    return ServiceProtoEnum::UDP;

  if (proto == 143)
    return ServiceProtoEnum::ALL;

  // Warning: This should never happen
  return ServiceProtoEnum::ALL;
}

std::shared_ptr<spdlog::logger> Service::logger() {
  return parent_.logger();
}

std::shared_ptr<ServiceBackend> Service::getBackend(const std::string &ip) {
  logger()->trace(
      "[ServiceBackend] Received request to read new backend for service {0}, "
      "{1}, {2}",
      getVip(), getVport(),
      ServiceJsonObject::ServiceProtoEnum_to_string(getProto()));
  logger()->trace("[ServiceBackend] Backend IP: {0}", ip);

  if (service_backends_.count(ip) == 0) {
    logger()->error(
        "[ServiceBackend] There are no entries associated with that key");
    throw std::runtime_error("There are no entries associated with that key");
  }

  return std::shared_ptr<ServiceBackend>(&service_backends_.at(ip),
                                         [](ServiceBackend *) {});
}

std::vector<std::shared_ptr<ServiceBackend>> Service::getBackendList() {
  std::vector<std::shared_ptr<ServiceBackend>> backends_vect;
  for (auto &it : service_backends_)
    backends_vect.push_back(getBackend(it.first));

  return backends_vect;
}

void Service::addBackend(const std::string &ip,
                         const ServiceBackendJsonObject &conf) {
  logger()->debug(
      "[ServiceBackend] Received request to create new backend for service "
      "{0}, {1}, {2}",
      getVip(), getVport(),
      ServiceJsonObject::ServiceProtoEnum_to_string(getProto()));
  logger()->debug("[ServiceBackend] Backend IP: {0}", ip);

  if (service_backends_.count(ip) != 0) {
    logger()->error(
        "[ServiceBackend] Key {0} already exists in the backends map", ip);
    throw std::runtime_error("Backend " + ip +
                             " already exists in the service map");
  }

  if (conf.weightIsSet()) {
    if (conf.getWeight() < 1 && conf.getWeight() != 0) {
      throw std::runtime_error(
          "Variable weight is in a wrong range. Supported range is from 1 to "
          "100");
    }
  }

  try {
    std::unordered_map<std::string, ServiceBackend>::iterator iter;
    bool inserted;
    std::tie(iter, inserted) = service_backends_.emplace(
        std::piecewise_construct, std::forward_as_tuple(conf.getIp()),
        std::forward_as_tuple(*this, conf));

    if (!inserted) {
      throw std::runtime_error("Unable to create the backend instance");
    } else {
      logger()->debug("[ServiceBackend] Backend {0} created successfully",
                      conf.getIp());
    }
  } catch (const std::exception &e) {
    logger()->error("[ServiceBackend] Error while creating the backend {0}",
                    conf.getIp());
    // We probably do not need to remove the service from the map because the
    // constructor raised an error
    throw;
  }

  addBackendToServiceMatrix(ip);
  updateConsistentHashMap();
}

void Service::addBackendList(
    const std::vector<ServiceBackendJsonObject> &conf) {
  for (auto &i : conf) {
    std::string ip_ = i.getIp();
    addBackend(ip_, i);
  }
}

void Service::replaceBackend(const std::string &ip,
                             const ServiceBackendJsonObject &conf) {
  delBackend(ip);
  std::string ip_ = conf.getIp();
  addBackend(ip_, conf);
}

void Service::delBackend(const std::string &ip) {
  logger()->trace(
      "[ServiceBackend] Received request to remove backend for service {0}, "
      "{1}, {2}",
      getVip(), getVport(),
      ServiceJsonObject::ServiceProtoEnum_to_string(getProto()));
  logger()->trace("[ServiceBackend] Backend IP: {0}", ip);

  if (service_backends_.count(ip) == 0) {
    logger()->error(
        "[ServiceBackend] There are no entries associated with that key");
    throw std::runtime_error("There are no entries associated with that key");
  }

  service_backends_.erase(ip);
  removeBackendFromServiceMatrix(ip);

  if (service_backends_.size() != 0)
    updateConsistentHashMap();

  if (service_backends_.size() == 0) {
    // If there are no backends let's remove all entries from the eBPF map
    backend_matrix_.clear();
    removeServiceFromKernelMap();
  }
}

void Service::delBackendList() {
  service_backends_.clear();
  backend_matrix_.clear();
  removeServiceFromKernelMap();
}
