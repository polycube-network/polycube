/*
 * Copyright 2017 The Polycube Authors
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

#include "port_xdp.h"

#include "cube_xdp.h"
#include "utils/netlink.h"

namespace polycube {
namespace polycubed {

PortXDP::PortXDP(CubeIface &parent, const std::string &name, uint16_t id,
                 const nlohmann::json &conf)
    : Port(parent, name, id, conf) {
  type_ = PortType::XDP;
}

PortXDP::~PortXDP() {
  set_peer("");
}

uint32_t PortXDP::get_parent_index() const {
  return parent_.get_index(ProgramType::INGRESS);
};

std::string PortXDP::get_cube_name() const {
  return parent_.get_name();
}

int PortXDP::get_attach_flags() const {
  CubeXDP &p = dynamic_cast<CubeXDP &>(parent_);
  return p.attach_flags_;
}

unsigned int PortXDP::get_peer_ifindex() const {
  return Netlink::getInstance().get_iface_index(peer_);
}

}  // namespace polycube
}  // namespace polycubed
