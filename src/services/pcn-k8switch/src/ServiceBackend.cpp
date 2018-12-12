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

#include "ServiceBackend.h"
#include "K8switch.h"

ServiceBackend::ServiceBackend(Service &parent, const ServiceBackendJsonObject &conf): parent_(parent) {
  if(conf.nameIsSet()) {
    setName(conf.getName());
  }

  // TODO: set default value for weight in datamodel
  if(conf.weightIsSet()) {
    weight_ = conf.getWeight();
    if(conf.getWeight() < 1 && conf.getWeight() != 0) {
      throw std::runtime_error("Variable weight is in a wrong range. Supported range is from 1 to 100");
    }
  } else {
    setWeight(1); //default weight
  }

  ip_ = conf.getIp();
  port_ = conf.getPort();
}

ServiceBackend::~ServiceBackend() { }

void ServiceBackend::update(const ServiceBackendJsonObject &conf) {
  if(conf.nameIsSet()) {
    setName(conf.getName());
  }

  if(conf.weightIsSet()) {
    setWeight(conf.getWeight());
  } else {
    setWeight(1); //default weight // TODO: do in datamodel
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

std::string ServiceBackend::getIp() {
  return ip_;
}

std::string ServiceBackend::getName() {
  return name_;
}

void ServiceBackend::setName(const std::string &value) {
  name_ = value;
}

uint16_t ServiceBackend::getWeight() {
  return weight_;
}

void ServiceBackend::setWeight(const uint16_t &value) {
  weight_ = value;
}

uint16_t ServiceBackend::getPort() {
  return port_;
}

std::shared_ptr<spdlog::logger> ServiceBackend::logger() {
  return parent_.logger();
}
