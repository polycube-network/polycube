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

#include <cstdint>

namespace polycube {
namespace polycubed {

class Node {
 public:
  Node() : index_(0){};
  virtual ~Node(){};
  virtual int get_fd() const = 0;
  void set_index(uint16_t index_) {
    this->index_ = index_;
  };
  uint16_t get_index() const {
    return index_;
  };

  // static const int _POLYCUBE_MAX_NODES = 1024;

 private:
  uint16_t index_;
};

}  // namespace polycubed
}  // namespace polycube
