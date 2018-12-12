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

#pragma once

#include <mutex>
#include <set>

namespace polycube {
namespace polycubed {

class IDGenerator {
 public:
  IDGenerator(uint32_t size);
  ~IDGenerator();
  uint32_t acquire();
  void release(uint32_t v);
 private:
 std::set<uint32_t> elements_;
  std::mutex mutex_;
};

}  // namespace polycubed
}  // namespace polycube
