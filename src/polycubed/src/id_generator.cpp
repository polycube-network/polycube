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

#include "id_generator.h"

namespace polycube {
namespace polycubed {

IDGenerator::IDGenerator(uint32_t size) {
  for (uint32_t i = 0; i < size; i++) {
    elements_.insert(i);
  }
}

IDGenerator::~IDGenerator() {}

uint32_t IDGenerator::acquire() {
  std::lock_guard<std::mutex> guard(mutex_);
  if (elements_.size() < 1) {
    throw std::runtime_error("No free elements in IDGenerator");
  }
  uint32_t v = *elements_.begin();
  elements_.erase(v);
  return v;
}

void IDGenerator::release(uint32_t v) {
  std::lock_guard<std::mutex> guard(mutex_);
  elements_.insert(v);
}

}  // namespace polycubed
}  // namespace polycube
