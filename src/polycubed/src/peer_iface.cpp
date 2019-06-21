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

#include "peer_iface.h"

namespace polycube {
namespace polycubed {

PeerIface::PeerIface(std::mutex &mutex) : mutex_(mutex) {}

PeerIface::~PeerIface() {
  for (int i = 0; i < cubes_.size(); i++) {
    cubes_[i]->set_parent(nullptr);
  }
}

int PeerIface::calculate_cube_index(int index) {
  return index;
}

enum class Position { AUTO, FIRST, LAST };

void PeerIface::add_cube(TransparentCube *cube, std::string *position,
                         const std::string &other) {
  std::lock_guard<std::mutex> guard(mutex_);
  int index = 0;
  int other_index;

  if (!other.empty()) {
    for (other_index = 0; other_index < cubes_.size(); other_index++) {
      if (cubes_[other_index]->get_name() == other) {
        break;
      }
    }

    if (index == cubes_.size()) {
      throw std::runtime_error("Cube doesn't exist");
    }
  }

  if (*position == "auto") {
    CubePositionComparison cmp;
    // try to determine index from below
    for (int i = 0; i < cubes_.size(); i++) {
      cmp = compare_position(cube->get_service_name(),
                             cubes_[i]->get_service_name());

      // cube has to be put after this
      if (cmp == CubePositionComparison::AFTER) {
        index = i + 1;
      } else if (cmp == CubePositionComparison::UNKOWN) {
        break;
      }
    }

    // it was impossible to find position from the front, try the
    // other way around
    if (cmp == CubePositionComparison::UNKOWN) {
      for (int i = cubes_.size() - 1; i >= 0; i--) {
        cmp = compare_position(cube->get_service_name(),
                               cubes_[i]->get_service_name());

        if (cmp == CubePositionComparison::BEFORE) {
          index = i;
        } else if (cmp == CubePositionComparison::UNKOWN) {
          throw std::runtime_error("Impossible to determine auto position");
        }
      }
    }
  } else if (*position == "first") {
    index = 0;
  } else if (*position == "last") {
    index = cubes_.size();
  } else if (*position == "before") {
    index = other_index;
  } else if (*position == "after") {
    index = other_index + 1;
  }

  *position = std::to_string(index);
  cubes_.insert(cubes_.begin() + calculate_cube_index(index), cube);
  update_indexes();
}

void PeerIface::remove_cube(const std::string &cube_name) {
  std::lock_guard<std::mutex> guard(mutex_);

  // check if cube exists
  auto pos = std::find_if(cubes_.begin(), cubes_.end(),
                          [cube_name](TransparentCube *a) -> bool {
                            return a->get_name() == cube_name;
                          });

  if (pos == std::end(cubes_)) {
    throw std::runtime_error("Cube doesn't exist");
  }

  cubes_.erase(pos);
  update_indexes();
}

std::vector<std::string> PeerIface::get_cubes_names() const {
  std::lock_guard<std::mutex> guard(mutex_);

  std::vector<std::string> cubes_names;

  for (auto &cube : cubes_) {
    cubes_names.push_back(cube->get_name());
  }

  return cubes_names;
}

PeerIface::CubePositionComparison PeerIface::compare_position(
    const std::string &cube1, const std::string &cube2) {
  // cube position, index 0 is the innermost one
  static std::vector<std::string> positions = {
      "nat", "ddosmitigator",
  };

  int pos1 = -1, pos2 = -1;

  for (int i = 0; i < positions.size(); i++) {
    if (positions[i] == cube1) {
      pos1 = i;
    }

    if (positions[i] == cube2) {
      pos2 = i;
    }
  }

  if (pos1 == -1 || pos2 == -1) {
    return CubePositionComparison::UNKOWN;
  }

  if (pos1 < pos2) {
    return CubePositionComparison::BEFORE;
  } else if (pos1 > pos2) {
    return CubePositionComparison::AFTER;
  }

  return CubePositionComparison::UNKOWN;
}

}  // namespace polycubed
}  // namespace polycube