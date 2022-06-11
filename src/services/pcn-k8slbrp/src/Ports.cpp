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

#include "Ports.h"
#include "K8slbrp.h"


Ports::Ports(polycube::service::Cube<Ports> &parent,
    std::shared_ptr<polycube::service::PortIface> port,
    const PortsJsonObject &conf)
    : PortsBase(parent, port) {
    logger()->info("creating Ports instance");

    auto type = conf.getType();
    auto ipIsSet = conf.ipIsSet();

    port_type_ = type;
    if (ipIsSet) ip_ = conf.getIp();
}

Ports::~Ports() {}

PortsTypeEnum Ports::getType() {
    return port_type_;
}

void Ports::setType(const PortsTypeEnum &value) {
    throw std::runtime_error("port type cannot be changed at runtime");
}

std::string Ports::getIp() {
    return ip_;
}

void Ports::setIp(const std::string &value) {
    ip_ = value;
}


