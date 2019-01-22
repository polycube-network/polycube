/*
 * Copyright 2019 The Polycube Authors
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

#include <cstdint>

namespace polycube {
namespace polycubed {

class PeerIface {
 public:
  virtual uint16_t get_index() const = 0;
  virtual uint16_t get_port_id() const = 0;
  virtual void set_next_index(uint16_t index) = 0;
  virtual void set_peer_iface(PeerIface *peer) = 0;
  virtual PeerIface *get_peer_iface() = 0;
};

}  // namespace polycubed
}  // namespace polycube