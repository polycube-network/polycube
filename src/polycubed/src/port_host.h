
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

#include "node.h"
#include "polycube/services/port_iface.h"

#include <api/BPF.h>
#include <spdlog/spdlog.h>

#include <netdb.h>
#include <spdlog/spdlog.h>
#include <stdio.h>
#include <stdlib.h>

#include <list>
#include <set>

using polycube::service::PortType;

namespace polycube {
namespace polycubed {

// Used to connect cubes with the linux networking stack
// this interface is special because only allow ongoing packets.
class PortHost : public Node {
 public:
  PortHost(PortType type);
  virtual ~PortHost();

  // It is not possible to copy nor assign an port_host.
  PortHost(const PortHost &) = delete;
  PortHost &operator=(const PortHost &) = delete;

  int get_fd() const;

 private:
  void load_tx();

  ebpf::BPF tx_;
  int fd_;  // fd_ of the tx_ program
  static const std::string TX_CODE;
  PortType type_;
  std::shared_ptr<spdlog::logger> logger;
};

}  // namespace polycubed
}  // namespace polycube
