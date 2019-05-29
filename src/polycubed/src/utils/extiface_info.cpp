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

#include "extiface_info.h"

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <linux/if_link.h>
#include <net/if.h>
#include <netdb.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <spdlog/spdlog.h>

namespace polycube {
namespace polycubed {

ExtIfaceInfo::ExtIfaceInfo(const std::string &name) : name_(name) {}

ExtIfaceInfo::ExtIfaceInfo(const std::string &name,
                           const std::string &description)
    : name_(name), description_(description) {}

ExtIfaceInfo::~ExtIfaceInfo() {}

std::string ExtIfaceInfo::get_name() const {
  return name_;
}

std::list<std::string> ExtIfaceInfo::get_addresses() const {
  return addresses_;
}

void ExtIfaceInfo::add_address(const std::string &addr) {
  addresses_.push_back(addr);
}

json ExtIfaceInfo::toJson() const {
  json j = {{"name", name_}};

  if (addresses_.size() > 0)
    j["addresses"] = addresses_;

  if (!description_.empty())
    j["description"] = description_;

  return j;
}

}  // namespace polycubed
}  // namespace polycube
