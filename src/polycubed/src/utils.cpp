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
#include <stdexcept>
#include <cstring>

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

  return KERNEL_VERSION(major, minor, patch) >= KERNEL_VERSION(major_r, minor_r, patch_r);
}

}  // namespace utils
}  // namespace polycubed
}  // namespace polycube
