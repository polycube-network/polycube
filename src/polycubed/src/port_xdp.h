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

#pragma once

#include "polycube/services/guid.h"
#include "port.h"

#include <cstdint>
#include <string>

namespace polycube {
namespace polycubed {

class PortXDP : public Port {
 public:
  PortXDP(CubeIface &parent, const std::string &name, uint16_t id,
          const nlohmann::json &conf);
  virtual ~PortXDP();

  uint32_t get_parent_index() const;
  std::string get_cube_name() const;
  int get_attach_flags() const;
  unsigned int get_peer_ifindex() const;

 private:
  void attach_xdp(const std::string &peer);
  void detach_xdp();
};

}  // namespace polycube
}  // namespace polycubed
