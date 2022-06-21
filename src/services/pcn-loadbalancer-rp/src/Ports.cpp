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

#include "Ports.h"
#include "Lbrp.h"

Ports::Ports(polycube::service::Cube<Ports> &parent,
             std::shared_ptr<polycube::service::PortIface> port,
             const PortsJsonObject &conf)
    : Port(port), parent_(static_cast<Lbrp &>(parent)) {
  logger()->info("Creating Ports instance");

  auto type = conf.getType();
  auto ipIsSet = conf.ipIsSet();

  port_type_ = type;
  if (ipIsSet) ip_ = conf.getIp();
}

Ports::~Ports() {}

void Ports::update(const PortsJsonObject &conf) {
  // This method updates all the object/parameter in Ports object specified in
  // the conf JsonObject.
  // You can modify this implementation.
  Port::set_conf(conf.getBase());

  if (conf.typeIsSet()) {
    setType(conf.getType());
  }
}

PortsJsonObject Ports::toJsonObject() {
  PortsJsonObject conf;
  conf.setBase(Port::to_json());

  conf.setType(getType());
  conf.setIp(getIp());

  return conf;
}

PortsTypeEnum Ports::getType() {
  // This method retrieves the type value.
  return port_type_;
}

void Ports::setType(const PortsTypeEnum &value) {
  // This method set the type value.
  throw std::runtime_error("Error: Port type cannot be changed at runtime.");
}

std::string Ports::getIp() {
  return ip_;
}

void Ports::setIp(const std::string &value) {
  ip_ = value;
}

std::shared_ptr<spdlog::logger> Ports::logger() {
  return parent_.logger();
}
