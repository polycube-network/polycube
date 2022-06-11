/*
 * Copyright 2021 Leonardo Di Giovanna
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
#include "K8slbrp.h"


ServiceBackend::ServiceBackend(Service &parent, const ServiceBackendJsonObject &conf)
    : ServiceBackendBase(parent) {
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

std::string ServiceBackend::getName() {
    return name_;
}

void ServiceBackend::setName(const std::string &value) {
    name_ = value;
}

std::string ServiceBackend::getIp() {
    return ip_;
}

uint16_t ServiceBackend::getPort() {
    return port_;
}

void ServiceBackend::setPort(const uint16_t &value) {
    // This method set the port value.
    port_ = value;

    // This will update the kernel map with the new port for the backend
    parent_.updateConsistentHashMap();
}

uint16_t ServiceBackend::getWeight() {
    return weight_;
}

void ServiceBackend::setWeight(const uint16_t &value) {
    weight_ = value;
}


