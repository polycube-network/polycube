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

#include "ServiceBackend.h"
#include "Lbrp.h"

ServiceBackend::ServiceBackend(Service &parent,
                               const ServiceBackendJsonObject &conf)
    : parent_(parent) {
  logger()->info("[ServiceBackend] Creating ServiceBackend instance");

  if (conf.nameIsSet()) {
    setName(conf.getName());
  }

  if (conf.weightIsSet()) {
    weight_ = conf.getWeight();
    logger()->debug("[ServiceBackend] Set weight {0}", getWeight());
  } else {
    setWeight(1);  // default weight
    logger()->debug("[ServiceBackend] Set default weight {0}", getWeight());
  }

  ip_ = conf.getIp();
  port_ = conf.getPort();
}

ServiceBackend::~ServiceBackend() {}

void ServiceBackend::update(const ServiceBackendJsonObject &conf) {
  // This method updates all the object/parameter in ServiceBackend object
  // specified in the conf JsonObject.
  // You can modify this implementation.
  if (conf.nameIsSet()) {
    setName(conf.getName());
  }

  if (conf.weightIsSet()) {
    setWeight(conf.getWeight());
  } else {
    setWeight(1);  // default weight
  }

  if (conf.portIsSet()) {
    setPort(conf.getPort());
  }
}

ServiceBackendJsonObject ServiceBackend::toJsonObject() {
  ServiceBackendJsonObject conf;

  conf.setIp(getIp());
  conf.setName(getName());
  conf.setWeight(getWeight());
  conf.setPort(getPort());

  return conf;
}

std::string ServiceBackend::getIp() {
  // This method retrieves the ip value.
  return ip_;
}

std::string ServiceBackend::getName() {
  // This method retrieves the name value.
  return name_;
}

void ServiceBackend::setName(const std::string &value) {
  // This method set the name value.
  name_ = value;
}

uint16_t ServiceBackend::getWeight() {
  // This method retrieves the weight value.
  return weight_;
}

void ServiceBackend::setWeight(const uint16_t &value) {
  // This method set the weight value.
  weight_ = value;
}

uint16_t ServiceBackend::getPort() {
  // This method retrieves the port value.
  return port_;
}

void ServiceBackend::setPort(const uint16_t &value) {
  // This method set the port value.
  port_ = value;

  // This will update the kernel map with the new port for the backend
  parent_.updateConsistentHashMap();
}

std::shared_ptr<spdlog::logger> ServiceBackend::logger() {
  return parent_.logger();
}
