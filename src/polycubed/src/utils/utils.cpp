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

#include "utils.h"

#include <linux/version.h>
#include <sys/utsname.h>
#include <cstring>
#include <stdexcept>

namespace polycube {
namespace polycubed {
namespace utils {

bool check_kernel_version(const std::string &version) {
  // current version
  unsigned int major, minor, patch;
  // required version
  unsigned int major_r, minor_r, patch_r;

  struct utsname buf;
  if (uname(&buf) == -1) {
    throw std::runtime_error("error getting kernel version: " +
                             std::string(std::strerror(errno)));
  }

  sscanf(buf.release, "%u.%u.%u", &major, &minor, &patch);
  sscanf(version.c_str(), "%u.%u.%u", &major_r, &minor_r, &patch_r);

  return KERNEL_VERSION(major, minor, patch) >=
         KERNEL_VERSION(major_r, minor_r, patch_r);
}

std::map<std::string, std::string> strip_port_peers(json &cubes) {
  std::map<std::string, std::string> peers;

  for (auto &cube : cubes) {
    if (!cube.count("ports")) {
      continue;
    }

    auto &ports = cube.at("ports");

    for (auto &port : ports) {
      if (port.count("peer")) {
        auto cube_name = cube.at("name").get<std::string>();
        auto port_name = port.at("name").get<std::string>();
        peers[cube_name + ":" + port_name] = port.at("peer").get<std::string>();
        port.erase("peer");
      }
    }
  }

  return peers;
}

std::map<std::string, json> strip_port_tcubes(json &jcube) {
  std::map<std::string, json> cubes;

  if (!jcube.count("ports")) {
    return cubes;
  }

  auto &jports = jcube.at("ports");

  for (auto &jport : jports) {
    if (!jport.count("tcubes")) {
      continue;
    }

    auto port_name = jport.at("name").get<std::string>();

    json jcubes = json::object();
    jcubes["tcubes"] = jport.at("tcubes");
    cubes[port_name] = jcubes;
    jport.erase("tcubes");
  }

  return cubes;
}

}  // namespace utils
}  // namespace polycubed
}  // namespace polycube
