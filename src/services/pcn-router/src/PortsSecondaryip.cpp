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

#include "PortsSecondaryip.h"
#include "Router.h"

#include <stdexcept>

PortsSecondaryip::PortsSecondaryip(Ports &parent,
                                   const PortsSecondaryipJsonObject &conf)
    : parent_(parent) {
  ip_ = conf.getIp();
  netmask_ = conf.getNetmask();

  parent.logger()->debug("Adding secondary address [ip: {0} - netmask: {1}]",
                         ip_, netmask_);

  // TODO: check that no other router port exists in the same network

  /*
  * Add two routes in the routing table
  */
  int index = parent.parent_.Cube::get_port(parent.getName())->index();
  parent.parent_.add_local_route(ip_, netmask_, parent.getName(), index);
}

PortsSecondaryip::~PortsSecondaryip() {}

void PortsSecondaryip::update(const PortsSecondaryipJsonObject &conf) {
  // This method updates all the object/parameter in PortsSecondaryip object
  // specified in the conf JsonObject.
  // You can modify this implementation.
}

PortsSecondaryipJsonObject PortsSecondaryip::toJsonObject() {
  PortsSecondaryipJsonObject conf;

  conf.setIp(getIp());

  conf.setNetmask(getNetmask());

  return conf;
}

// TODO: move in the Ports class? Probably it is better
void PortsSecondaryip::updatePortInDataPath(Ports &parent) {
  auto router_port =
      parent.parent_.get_hash_table<uint16_t, r_port>("router_port");

  r_port value{
      .ip = utils::ip_string_to_be_uint(parent.getIp()),
      .netmask = utils::ip_string_to_be_uint(parent.getNetmask()),
      .secondary_ip = {},
      .secondary_netmask = {},
      .mac = utils::mac_string_to_be_uint(parent.getMac()),
  };

  uint16_t index = parent.parent_.get_port(parent.getName())->index();
  int i = 0;
  for (auto &addr : parent.getSecondaryipList()) {
    value.secondary_ip[i] = utils::ip_string_to_be_uint(addr->getIp());
    value.secondary_netmask[i] =
        utils::ip_string_to_be_uint(addr->getNetmask());
    i++;
  }

  router_port.set(index, value);
}

void PortsSecondaryip::createInControlPlane(
    Ports &parent, const std::string &ip, const std::string &netmask,
    const PortsSecondaryipJsonObject &conf) {
  // This method creates the actual PortsSecondaryip object given thee key
  // param.

  // TODO: a port cannot have more than SECONDARY_ADDRESS secondary addresses.
  // This constraint come
  // from a constant defined in the fast path

  parent.logger()->info(
      "Adding secondary address [port: {0} - ip: {1} - netmask: {2}]",
      parent.getName(), ip, netmask);

  parent.logger()->info("Port {0} has already {1} secondary addresses",
                        parent.getName(), parent.secondary_ips_.size());
  auto ret = parent.secondary_ips_.emplace(PortsSecondaryip(parent, conf));
  parent.logger()->info("Now port {0} has {1} secondary addresses",
                        parent.getName(), parent.secondary_ips_.size());
}

std::string PortsSecondaryip::getIp() const {
  // This method retrieves the ip value.
  return ip_;
}

std::string PortsSecondaryip::getNetmask() const {
  // This method retrieves the netmask value.
  return netmask_;
}

std::shared_ptr<spdlog::logger> PortsSecondaryip::logger() {
  return parent_.logger();
}
