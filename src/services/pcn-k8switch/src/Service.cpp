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
#include "K8switch.h"

#include <functional>
#include <iostream>
#include <random>

using namespace polycube::service;

const std::string Service::EBPF_SERVICE_MAP = "services";
const std::string Service::EBPF_BACKEND_TO_SERVICE_MAP = "cluster_ip_reverse";
const uint Service::INITIAL_BACKEND_SIZE = 10;
const uint Service::BACKEND_REPLICAS = 3;

#define KUBE_PROXY_FLAG (65535U)

Service::Service(K8switch &parent, const ServiceJsonObject &conf)
    : parent_(parent),
      backend_size_(INITIAL_BACKEND_SIZE),
      kube_proxy_service_(false) {
  vip_ = conf.getVip();

  uint32_t vip_dec_ = parent_.ip_to_dec(vip_);
  if ((vip_dec_ & parent_.cluster_ip_mask_) == parent_.cluster_ip_subnet_) {
    type_ = ServiceType::CLUSTER_IP;
  } else {
    type_ = ServiceType::NODE_PORT;
  }

  vport_ = conf.getVport();
  proto_ = conf.getProto();

  logger()->info("Creating service {}:{}/{}", vip_, vport_,
                 ServiceJsonObject::ServiceProtoEnum_to_string(proto_));

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

Service::~Service() {
  removeServiceFromKernelMap();
}

void Service::update(const ServiceJsonObject &conf) {
  if (conf.nameIsSet()) {
    setName(conf.getName());
  }

  if (conf.backendIsSet()) {
    for (auto &i : conf.getBackend()) {
      auto ip = i.getIp();
      auto port = i.getPort();
      auto m = getBackend(ip, port);
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
  return vip_;
}

ServiceProtoEnum Service::getProto() {
  return proto_;
}

std::string Service::getName() {
  return service_name_;
}

void Service::setName(const std::string &value) {
  service_name_ = value;
}

uint16_t Service::getVport() {
  return vport_;
}

void Service::delBackend(const std::string &ip, const uint16_t &port) {
  auto key = ServiceBackend::Key(ip, port);
  logger()->debug("{}:{}/{} Removing backend {}:{}", getVip(), getVport(),
                  ServiceJsonObject::ServiceProtoEnum_to_string(getProto()), ip,
                  port);

  if (service_backends_.count(key) == 0) {
    logger()->error("Backend does not exist");
    throw std::runtime_error("Backend does not exist");
  }

  // TODO: if the service is handled by kube-proxy, check if this was the
  // backend that forced it to be handled by kube-proxy.
  // Check also if this backend was forcing *other* services to be handled
  // by kube-proxy

  int bkd_size = std::max(backend_size_, (uint)service_backends_.size());
  int vect_size = bkd_size * BACKEND_REPLICAS;
  backend_matrix_[key] = getEmptyIntVector(vect_size);

  service_backends_.erase(key);

  if (service_backends_.size()) {
    updateKernelServiceMap();
  } else {
    removeServiceFromKernelMap();
  }
}

void Service::addBackend(const std::string &ip, const uint16_t &port,
                         const ServiceBackendJsonObject &conf) {
  auto key = ServiceBackend::Key(ip, port);
  logger()->debug("{}:{}/{} Creating backend {}:{}", getVip(), getVport(),
                  ServiceJsonObject::ServiceProtoEnum_to_string(getProto()), ip,
                  port);

  if (service_backends_.count(key) != 0) {
    logger()->error("Backend {}:{} alraedy exists", ip, port);
    throw std::runtime_error("Backend exists");
  }

  if (!kube_proxy_service_ && type_ == ServiceType::CLUSTER_IP) {
    // check if the backend is present in any other service, if yes,
    // this service has to be provided by kube-proxy
    for (auto &&service : parent_.service_map_) {
      if (service.second.type_ != ServiceType::CLUSTER_IP)
        continue;

      // TODO: is this true? only same proto services are problematic.
      if (service.second.getProto() != getProto())
        continue;

      // check if service is this
      if (service.second.getVip() == getVip() &&
          service.second.getVport() == getVport())
        continue;

      if (service.second.service_backends_.count(key) != 0) {
        logger()->warn(
            "Backend {}:{} is present in {}:{}/{}, defaulting to kube-proxy",
            ip, port, service.second.getVip(), service.second.getVport(),
            ServiceJsonObject::ServiceProtoEnum_to_string(
                service.second.getProto()));
        removeServiceFromKernelMap();
        kube_proxy_service_ = true;
        break;
      }
    }
  }

  std::map<ServiceBackend::Key, ServiceBackend>::iterator iter;
  bool inserted;
  std::tie(iter, inserted) = service_backends_.emplace(
      std::piecewise_construct, std::forward_as_tuple(key),
      std::forward_as_tuple(*this, conf));
  if (!inserted) {
    throw std::runtime_error("Unable to create the backend instance");
  }

  if (kube_proxy_service_) {
    insertAsKubeProxyService();
    return;
  }

  // logger()->error("Backend {}:{} created", ip, port);

  int bkd_size = std::max(backend_size_, (uint)service_backends_.size());
  int vect_size = bkd_size * BACKEND_REPLICAS;
  backend_matrix_[key] = getRandomIntVector(vect_size);

  updateKernelServiceMap();
}

void Service::addBackendList(
    const std::vector<ServiceBackendJsonObject> &conf) {
  for (auto &i : conf) {
    std::string ip_ = i.getIp();
    uint16_t port_ = i.getPort();
    addBackend(ip_, port_, i);
  }
}

std::shared_ptr<ServiceBackend> Service::getBackend(const std::string &ip,
                                                    const uint16_t &port) {
  auto key = ServiceBackend::Key(ip, port);

  if (service_backends_.count(key) == 0) {
    logger()->error("Backend {}:{} does not exist", ip, port);
    throw std::runtime_error("Backend does not exist");
  }

  return std::shared_ptr<ServiceBackend>(&service_backends_.at(key),
                                         [](ServiceBackend *) {});
}

std::vector<std::shared_ptr<ServiceBackend>> Service::getBackendList() {
  std::vector<std::shared_ptr<ServiceBackend>> backends_vect;
  for (auto &it : service_backends_) {
    auto ptr =
        std::shared_ptr<ServiceBackend>(&it.second, [](ServiceBackend *) {});
    backends_vect.push_back(ptr);
  }

  return backends_vect;
}

void Service::delBackendList() {
  service_backends_.clear();
  backend_matrix_.clear();
  removeServiceFromKernelMap();
}

void Service::replaceBackend(const std::string &ip, const uint16_t &port,
                             const ServiceBackendJsonObject &conf) {
  throw std::runtime_error("Not implemented");
}

/*
 * This method removes all entries for a service in the datapath map
 */

void Service::removeServiceFromKernelMap() {
  if (service_backends_.size() == 0)
    return;

  logger()->trace("Removing all elements from service map");
  auto services_table = parent_.get_hash_table<vip, backend>(EBPF_SERVICE_MAP);

  for (uint16_t index = 0; index < backend_size_ * BACKEND_REPLICAS; index++) {
    vip key{
        .ip = utils::ip_string_to_be_uint(getVip()),
        .port = htons(getVport()),
        .proto = htons(Service::convertProtoToNumber(getProto())),
        .index = index,
    };

    services_table.remove(key);

    // logger()->trace("Remove service map with key: {0}", service_key);
  }

  // Remove elements in reverse lookup table if service is cluster IP
  if (type_ == ServiceType::CLUSTER_IP) {
    auto reverse_map =
        parent_.get_hash_table<backend, vip>(EBPF_BACKEND_TO_SERVICE_MAP);
    for (auto &bck : service_backends_) {
      backend key{
          .ip = utils::ip_string_to_be_uint(bck.second.getIp()),
          .port = htons(bck.second.getPort()),
          .proto = htons(Service::convertProtoToNumber(getProto())),
      };
      try {
        reverse_map.remove(key);
      } catch (...) {
      }
    }
  }
}

/*
 * The following function is used to load and update services map in the
 * datapath.
 * The map has :
 * 	-key = struct vip: {ip, port, proto, index}
 * 	-value = struct backend: {ip, port}
 */

void Service::updateKernelServiceMap() {
  auto consistent_array = getConsistentArray();
  auto services_map = parent_.get_hash_table<vip, backend>(EBPF_SERVICE_MAP);
  auto reverse_map =
      parent_.get_hash_table<backend, vip>(EBPF_BACKEND_TO_SERVICE_MAP);

  uint16_t index = 0;

  vip key{
      .ip = utils::ip_string_to_be_uint(getVip()),
      .port = htons(getVport()),
      .proto = htons(Service::convertProtoToNumber(getProto())),
      .index = index,
  };

  // This is a particular value that is used to indicate the size of the backend
  // pool.
  // In doesn't indicate a real backend
  backend value{
      .ip = utils::ip_string_to_be_uint(getVip()),
      .port = uint8_t(consistent_array.size()),
      .proto = 0,
  };
  // logger()->trace("Setting Service map with key: {0} and value: {1}",
  //                service_key, service_value);

  services_map.set(key, value);

  for (const auto &key : consistent_array) {
    index++;

    // TODO: porttt
    // auto backend = ServiceBackend::getEntry(*this, backend_ip, 8080);
    auto &bck = service_backends_.at(key);

    vip backend_key{
        .ip = utils::ip_string_to_be_uint(getVip()),
        .port = htons(getVport()),
        .proto = htons(Service::convertProtoToNumber(getProto())),
        .index = index,
    };

    backend backend_value{
        .ip = utils::ip_string_to_be_uint(bck.getIp()),
        .port = htons(bck.getPort()),
        .proto = htons(Service::convertProtoToNumber(getProto())),
    };

    services_map.set(backend_key, backend_value);
    if (type_ == ServiceType::CLUSTER_IP) {
      reverse_map.set(backend_value, backend_key);
    }
    // logger()->trace("{2} Setting Service map with key: {0} and value: {1}",
    //  backend_key, backend_value,index);
  }

  logger()->debug("Service map updated");
}

/*
 * This method returns the consistent array of the service's backends
 */
std::vector<ServiceBackend::Key> Service::getConsistentArray() {
  std::vector<ServiceBackend::Key> backend_list;
  std::vector<std::vector<int>> backend_matrix_list;
  int bkd_size = std::max(backend_size_, (uint)service_backends_.size());
  int array_rows = bkd_size * BACKEND_REPLICAS;

  for (const auto &s : backend_matrix_) {
    backend_list.push_back(s.first);
    backend_matrix_list.push_back(s.second);
  }

  // Declare the final backend list vector and initialize all elems to -1
  // std::vector<ServiceBackend::Key> final_backend_list(array_rows, "-1");
  std::vector<ServiceBackend::Key> final_backend_list(array_rows);

  // Retrieve how much entries for each backend
  std::map<ServiceBackend::Key, int> weight_backend_list = getWeightBackend();

  int index = 0;
  int count = 0;

  // TODO: verify this comment
  /*
  * We have to  scroll a matrix starting [0,0].
  * For each cell, I retrieve the number stored in that cell and lookup in the
  * final_backend_list if it exists or not,
  *  if not, it is saved in this array(final_backend_list) and we go on in the
  * matrix by checking the next cell [0,1] row0 column 1.
  *  if yes, we have to go the next row -> [1,0], retrieve the number in this
  * cell,
  * lookup in the final_backend_list and so on.
  * All of this continue until the final_backend_list is filed.
  * Each matrix's column stores a range of number from 0 to bkd_size
  * Matrix cells can have 2 types of number:
  * 1) "-2" -> go next colum (continue command to go next for loop and x++ keep
  *  align matrix column and vector of backend's name)
  * 2) ">0" -> check if you can store it in the final_backend_list
  *
  * final_backend_list has 2 types of numbers:
  * 1) "-1" -> You can store the number, in this case I check if the entry can
  * be
  *  assigned to this backend by checking the number in
  *  weight_backend_list[backend_list.at(x)] ; if it is greater than 0, it is
  *  stored and the number is decreased
  * weight_backend_list[backend_list.at(x)]--;
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
      } else if (final_backend_list.at(num).first != "") {
        index++;
        if (index >= array_rows) {
          x++;
          index = j;
          continue;
        }
        goto ret;
      } else if (final_backend_list.at(num).first == "") {
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
 * backend's weights.
 * We need to calculate the common coefficient between all backends.
 * Then we multiply this coefficient for backend's weight in order to get the
 * number of
 * entries assigned to this backend.
 */
std::map<ServiceBackend::Key, int> Service::getWeightBackend() {
  int bkd_size = std::max(backend_size_, (uint)service_backends_.size());
  int vect_size = bkd_size * BACKEND_REPLICAS;
  int total_weight = 0;
  int sum_weight = 0;
  std::map<ServiceBackend::Key, int> weight_back_pool;

  // Calculate the total of weights
  for (auto &backend : service_backends_) {
    sum_weight += backend.second.getWeight();
  }

  float coeff = (static_cast<float>(vect_size) / sum_weight);
  // logger()->trace("vec size {0} sum_weight {1} coeff {2} ", vect_size,
  // sum_weight, coeff);

  // Calculate ,for each backend, its number of entries
  for (auto &it : service_backends_) {
    auto &backend = it.second;
    auto &key = it.first;
    if (backend.getWeight() == 0)
      continue;
    int weight_backend = coeff * backend.getWeight();
    if (weight_backend == 0)
      weight_backend++;
    weight_back_pool[key] = weight_backend;
    total_weight += weight_backend;
    // logger()->trace("backend {}:{} weight {}", key.first, key.second,
    // weight_backend);
  }

  int res = vect_size - total_weight;

  // Allign all , if necessary
  while (res > 0) {
    for (auto &it : service_backends_) {
      auto &backend = it.second;
      auto &key = it.first;
      if (backend.getWeight() == 0)
        continue;

      weight_back_pool[key]++;
      res--;

      // logger()->trace("backend {}:{} weight {}", key.first, key.second,
      // weight_back_pool[key]);

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

void Service::insertAsKubeProxyService() {
  vip key{
      .ip = utils::ip_string_to_be_uint(getVip()),
      .port = htons(getVport()),
      .proto = htons(Service::convertProtoToNumber(getProto())),
      .index = 0,
  };

  // This is a special value used to indicate that the service is implemented by
  // kube-proxy
  backend value{
      .ip = utils::ip_string_to_be_uint(getVip()), .port = KUBE_PROXY_FLAG,
  };

  auto services_map = parent_.get_hash_table<vip, backend>(EBPF_SERVICE_MAP);
  services_map.set(key, value);
}

uint8_t Service::convertProtoToNumber(const ServiceProtoEnum &proto) {
  switch (proto) {
  case ServiceProtoEnum::TCP:
    return 6;
  case ServiceProtoEnum::UDP:
    return 17;
  }
}

ServiceProtoEnum Service::convertNumberToProto(const uint8_t proto) {
  if (proto == 6)
    return ServiceProtoEnum::TCP;

  if (proto == 17)
    return ServiceProtoEnum::UDP;

  // Warning: This should never happen
  return ServiceProtoEnum::TCP;
}

std::shared_ptr<spdlog::logger> Service::logger() {
  return parent_.logger();
}
