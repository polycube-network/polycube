/*
 * Copyright 2022 The Polycube Authors
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
#include "K8sdispatcher.h"
#include "Utils.h"


Ports::Ports(polycube::service::Cube<Ports> &parent,
             std::shared_ptr<polycube::service::PortIface> port,
             const PortsJsonObject &conf)
    : PortsBase(parent, port), type_{conf.getType()} {
  logger()->info("Creating Ports instance");
  auto ipIsSet = conf.ipIsSet();
  if (this->type_ == PortsTypeEnum::FRONTEND) {
    if (!ipIsSet) {
      throw std::runtime_error(
          "The IP address is mandatory for a FRONTEND port");
    }

    auto ip = conf.getIp();
    if (!utils::is_valid_ipv4_str(ip)) {
      throw std::runtime_error{"Invalid IPv4 address"};
    }
    this->ip_ = ip;
  } else {
    if (ipIsSet) {
      throw std::runtime_error(
          "The IP address in not allowed for a BACKEND port");
    }
  }
}

Ports::~Ports() {
  logger()->info("Destroying Ports instance");
}

PortsTypeEnum Ports::getType() {
  return this->type_;
}

std::string Ports::getIp() {
  return this->ip_;
}