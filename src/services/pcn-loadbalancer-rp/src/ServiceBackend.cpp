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

//Modify these methods with your own implementation


#include "ServiceBackend.h"
#include "Lbrp.h"

ServiceBackend::ServiceBackend(Service &parent, const ServiceBackendJsonObject &conf): parent_(parent) {
  logger()->info("[ServiceBackend] Creating ServiceBackend instance");

  if(conf.nameIsSet()) {
    setName(conf.getName());
  }

  if(conf.weightIsSet()) {
    weight_ = conf.getWeight();
    logger()->debug("[ServiceBackend] Set weight {0}", getWeight());
  } else {
    setWeight(1); //default weight
    logger()->debug("[ServiceBackend] Set default weight {0}", getWeight());
  }

  ip_ = conf.getIp();
  port_ = conf.getPort();
}

ServiceBackend::~ServiceBackend() { }

void ServiceBackend::update(const ServiceBackendJsonObject &conf) {
  //This method updates all the object/parameter in ServiceBackend object specified in the conf JsonObject.
  //You can modify this implementation.
  if(conf.nameIsSet()) {
    setName(conf.getName());
  }

  if(conf.weightIsSet()) {
    setWeight(conf.getWeight());
  } else {
    setWeight(1); //default weight
  }

  if(conf.portIsSet()) {
    setPort(conf.getPort());
  }
}

ServiceBackendJsonObject ServiceBackend::toJsonObject(){
  ServiceBackendJsonObject conf;

  conf.setIp(getIp());
  conf.setName(getName());
  conf.setWeight(getWeight());
  conf.setPort(getPort());

  return conf;
}


void ServiceBackend::create(Service &parent, const std::string &ip, const ServiceBackendJsonObject &conf){
  parent.logger()->debug("[ServiceBackend] Received request to create new backend for service {0}, {1}, {2}", parent.getVip(), parent.getVport(), ServiceJsonObject::ServiceProtoEnum_to_string(parent.getProto()));
  parent.logger()->debug("[ServiceBackend] Backend IP: {0}", ip);

  if(parent.service_backends_.count(ip) != 0) {
    parent.logger()->error("[ServiceBackend] Key {0} already exists in the backends map", ip);
    throw std::runtime_error("Backend " + ip + " already exists in the service map");
  }

  if(conf.weightIsSet()){
    if(conf.getWeight() < 1 && conf.getWeight()!=0 ){
      throw std::runtime_error("Variable weight is in a wrong range. Supported range is from 1 to 100");
    }
  }

  try {
    std::unordered_map<std::string, ServiceBackend>::iterator iter;
    bool inserted;
    std::tie(iter, inserted) = parent.service_backends_.emplace(std::piecewise_construct,
                                                                std::forward_as_tuple(conf.getIp()),
                                                                std::forward_as_tuple(parent, conf));

    if (!inserted) {
      throw std::runtime_error("Unable to create the backend instance");
    } else {
      parent.logger()->debug("[ServiceBackend] Backend {0} created successfully", conf.getIp());
    }
  } catch (const std::exception &e) {
    parent.logger()->error("[ServiceBackend] Error while creating the backend {0}", conf.getIp());
    //We probably do not need to remove the service from the map because the
    //constructor raised an error
    throw;
  }

  parent.addBackendToServiceMatrix(ip);
  parent.updateConsistentHashMap();
}

std::shared_ptr<ServiceBackend> ServiceBackend::getEntry(Service &parent, const std::string &ip){
  parent.logger()->trace("[ServiceBackend] Received request to read new backend for service {0}, {1}, {2}", parent.getVip(), parent.getVport(), ServiceJsonObject::ServiceProtoEnum_to_string(parent.getProto()));
  parent.logger()->trace("[ServiceBackend] Backend IP: {0}", ip);

  if (parent.service_backends_.count(ip) == 0) {
    parent.logger()->error("[ServiceBackend] There are no entries associated with that key");
    throw std::runtime_error("There are no entries associated with that key");
  }

  return std::shared_ptr<ServiceBackend>(&parent.service_backends_.at(ip), [](ServiceBackend*){});
}

void ServiceBackend::removeEntry(Service &parent, const std::string &ip){
  parent.logger()->trace("[ServiceBackend] Received request to remove backend for service {0}, {1}, {2}",
                          parent.getVip(), parent.getVport(),
                          ServiceJsonObject::ServiceProtoEnum_to_string(parent.getProto()));
  parent.logger()->trace("[ServiceBackend] Backend IP: {0}", ip);

  if (parent.service_backends_.count(ip) == 0) {
    parent.logger()->error("[ServiceBackend] There are no entries associated with that key");
    throw std::runtime_error("There are no entries associated with that key");
  }

  parent.service_backends_.erase(ip);
  parent.removeBackendFromServiceMatrix(ip);

  if (parent.service_backends_.size() != 0)
      parent.updateConsistentHashMap();

  if (parent.service_backends_.size() == 0) {
    //If there are no backends let's remove all entries from the eBPF map
    parent.backend_matrix_.clear();
    parent.removeServiceFromKernelMap();
  }
}

std::vector<std::shared_ptr<ServiceBackend>> ServiceBackend::get(Service &parent){
  std::vector<std::shared_ptr<ServiceBackend>> backends_vect;
  for(auto &it : parent.service_backends_)
    backends_vect.push_back(ServiceBackend::getEntry(parent, it.first));

  return backends_vect;
}

void ServiceBackend::remove(Service &parent){
  parent.service_backends_.clear();
  parent.backend_matrix_.clear();
  parent.removeServiceFromKernelMap();
}

std::string ServiceBackend::getIp(){
  //This method retrieves the ip value.
  return ip_;
}


std::string ServiceBackend::getName(){
  //This method retrieves the name value.
  return name_;
}

void ServiceBackend::setName(const std::string &value){
  //This method set the name value.
  name_ = value;
}

uint16_t ServiceBackend::getWeight(){
  //This method retrieves the weight value.
  return weight_;
}

void ServiceBackend::setWeight(const uint16_t &value){
  //This method set the weight value.
  weight_ = value;
}

uint16_t ServiceBackend::getPort(){
  //This method retrieves the port value.
  return port_;
}

void ServiceBackend::setPort(const uint16_t &value){
  //This method set the port value.
  port_ = value;

  //This will update the kernel map with the new port for the backend
  parent_.updateConsistentHashMap();
}

std::shared_ptr<spdlog::logger> ServiceBackend::logger() {
  return parent_.logger();
}
