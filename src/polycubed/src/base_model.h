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

#include <string>

#include "polycube/services/json.hpp"
#include "polycube/services/response.h"

namespace polycube::polycubed {

class BaseModel {
 public:
  BaseModel() = default;
  ~BaseModel() = default;
  // polycube-base module
  Response get_type(const std::string &cube_name) const;
  Response get_uuid(const std::string &cube_name) const;
  Response get_loglevel(const std::string &cube_name) const;
  Response set_loglevel(const std::string &cube_name,
                        const nlohmann::json &json);

  Response get_parent(const std::string &cube_name) const;

  Response get_service(const std::string &cube_name) const;

  // polycube-standard-base module
  Response get_port_uuid(const std::string &cube_name,
                         const std::string &port_name) const;
  Response get_port_status(const std::string &cube_name,
                           const std::string &port_name) const;
  Response get_port_peer(const std::string &cube_name,
                         const std::string &port_name) const;
  Response set_port_peer(const std::string &cube_name,
                         const std::string &port_name,
                         const nlohmann::json &json);
  Response get_port_tcubes(const std::string &cube_name,
                           const std::string &port_name) const;
};

}  // namespace polycube::polycubed