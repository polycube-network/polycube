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

// TODO: Modify these methods with your own implementation

#include "PortsSecondaryip.h"
#include "Router.h"
#include "Ports.h"


PortsSecondaryip::PortsSecondaryip(Ports &parent, const PortsSecondaryipJsonObject &conf)
    : PortsSecondaryipBase(parent) {
  // TODO: check that no other router port exists in the same network
  ip_ = conf.getIp();
  parent.logger()->debug("Adding secondary address [ip: {0}]", ip_);

  int max_address = MAX_SECONDARY_ADDRESSES;
  if (parent.secondary_ips_.size() >= max_address) {
    throw std::runtime_error("Maximum number of secondary IPs on the port "
                            + parent.getName());
  }

  /*
  * Add two routes in the routing table
  */
  int index = parent.parent_.Cube::get_port(parent.getName())->index();
  parent.parent_.add_local_route(ip_, parent.getName(), index);
}

PortsSecondaryip::~PortsSecondaryip() {}

std::string PortsSecondaryip::getIp() {
  // This method retrieves the ip value.
  return ip_;
}

void PortsSecondaryip::createInControlPlane(
    Ports &parent, const std::string &ip,
    const PortsSecondaryipJsonObject &conf) {
  // This method creates the actual PortsSecondaryip object given thee key
  // param.

  // TODO: a port cannot have more than MAX_SECONDARY_ADDRESSES secondary addresses.
  // This constraint come
  // from a constant defined in the fast paths

  parent.logger()->info(
      "Adding secondary address [port: {0} - ip: {1}]", parent.getName(), ip);

  parent.logger()->info("Port {0} has already {1} secondary addresses",
                        parent.getName(), parent.secondary_ips_.size());
  auto ret = parent.secondary_ips_.emplace(PortsSecondaryip(parent, conf));
  parent.logger()->info("Now port {0} has {1} secondary addresses",
                        parent.getName(), parent.secondary_ips_.size());
}
