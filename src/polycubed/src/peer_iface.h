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
#include <mutex>

#include "polycube/services/cube_iface.h"
#include "transparent_cube.h"

using polycube::service::TransparentCubeIface;
using polycube::service::ProgramType;

namespace polycube {
namespace polycubed {

class PeerIface {
 public:
  PeerIface(std::mutex &mutex);
  virtual ~PeerIface();
  virtual uint16_t get_index() const = 0;
  virtual uint16_t get_port_id() const = 0;
  virtual void set_next_index(uint16_t index) = 0;
  virtual void set_peer_iface(PeerIface *peer) = 0;
  virtual PeerIface *get_peer_iface() = 0;

  void add_cube(TransparentCube *cube, const std::string &position,
                const std::string &other);
  void remove_cube(const std::string &type);

 protected:
  enum class CubePositionComparison {
    BEFORE,
    AFTER,
    EQUAL,
    UNKOWN,
  };

  static CubePositionComparison compare_position(const std::string &cube1,
                                                 const std::string &cube2);
  virtual void update_indexes() = 0;
  virtual int calculate_cube_index(int index);

  std::vector<TransparentCube *> cubes_;

 private:
  std::mutex &mutex_;
};

}  // namespace polycubed
}  // namespace polycube