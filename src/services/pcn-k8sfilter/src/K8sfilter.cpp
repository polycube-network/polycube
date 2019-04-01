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

#include "K8sfilter.h"
#include "K8sfilter_dp.h"

#include <cinttypes>

K8sfilter::K8sfilter(const std::string name, const K8sfilterJsonObject &conf)
    : Cube(conf.getBase(), {generate_code()}, {}) {
  logger()->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [K8sfilter] [%n] [%l] %v");
  logger()->info("Creating K8sfilter instance");

  addPortsList(conf.getPorts());
  setNodeportRange(conf.getNodeportRange());
}

K8sfilter::~K8sfilter() {}

void K8sfilter::update(const K8sfilterJsonObject &conf) {
  // This method updates all the object/parameter in K8sfilter object specified
  // in the conf JsonObject.
  // You can modify this implementation.
  Cube::set_conf(conf.getBase());

  if (conf.portsIsSet()) {
    for (auto &i : conf.getPorts()) {
      auto name = i.getName();
      auto m = getPorts(name);
      m->update(i);
    }
  }
  if (conf.nodeportRangeIsSet()) {
    setNodeportRange(conf.getNodeportRange());
  }
}

K8sfilterJsonObject K8sfilter::toJsonObject() {
  K8sfilterJsonObject conf;
  conf.setBase(Cube::to_json());

  for (auto &i : getPortsList()) {
    conf.addPorts(i->toJsonObject());
  }

  conf.setNodeportRange(getNodeportRange());

  return conf;
}

std::string K8sfilter::generate_code() {
  return k8sfilter_code;
}

std::vector<std::string> K8sfilter::generate_code_vector() {
  throw std::runtime_error("Method not implemented");
}

void K8sfilter::packet_in(Ports &port, polycube::service::PacketInMetadata &md,
                          const std::vector<uint8_t> &packet) {
  logger()->info("Packet received from port {0}", port.name());
}

std::string K8sfilter::getNodeportRange() {
  return nodeport_range_;
}

void K8sfilter::setNodeportRange(const std::string &value) {
  uint16_t low;
  uint16_t high;
  int ret = std::sscanf(value.c_str(), "%" SCNu16 "-%" SCNu16, &low, &high);
  if (ret != 2) {
    logger()->error("value {} is not valid for node port range", value);
    throw std::runtime_error("Invalid node port range");
  }

  if (low >= high) {
    throw std::runtime_error("Invalid node port range");
  }

  nodeport_range_low_ = low;
  nodeport_range_high_ = high;

  nodeport_range_ = value;
  reloadConfig();
}

void K8sfilter::reloadConfig() {
  std::string flags;

  // ports
  uint16_t external_port = 0;
  uint16_t internal_port = 0;

  for (auto &it : get_ports()) {
    switch (it->getType()) {
    case PortsTypeEnum::EXTERNAL:
      external_port = it->index();
      break;
    case PortsTypeEnum::INTERNAL:
      internal_port = it->index();
      break;
    }
  }

  flags += "#define EXTERNAL_PORT " + std::to_string(external_port) + "\n";
  flags += "#define INTERNAL_PORT " + std::to_string(internal_port) + "\n";
  flags += "#define NODEPORT_RANGE_LOW " + std::to_string(nodeport_range_low_) +
           "\n";
  flags += "#define NODEPORT_RANGE_HIGH " +
           std::to_string(nodeport_range_high_) + "\n";

  logger()->debug("Reloading code with flags port: {}", flags);

  reload(flags + k8sfilter_code);

  logger()->trace("New lbrp code loaded");
}
