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

#include <list>
#include <map>
#include <set>
#include <string>

#include "polycube/services/json.hpp"

using json = nlohmann::json;

namespace polycube {
namespace polycubed {

class ExtIfaceInfo {
 public:
  ExtIfaceInfo(const std::string &name);
  ExtIfaceInfo(const std::string &name, const std::string &description);
  ~ExtIfaceInfo();
  std::string get_name() const;
  std::list<std::string> get_addresses() const;
  void add_address(const std::string &addr);

  json toJson() const;

 private:
  std::string name_;
  std::string description_;
  std::list<std::string> addresses_;
};

}  // namespace polycubed
}  // namespace polycube
