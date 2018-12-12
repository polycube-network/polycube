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

#include "node.h"
#include "port_tc.h"

#include <api/BPF.h>
#include <spdlog/spdlog.h>

#include <stdio.h>

#include <set>

namespace polycube {
namespace polycubed {

class PortTC;
class ExtIfaceXDP;

class ExtIfaceTC : public Node {
  friend class ExtIfaceXDP;
 public:
  ExtIfaceTC(const std::string &iface, const PortTC &port);
  virtual ~ExtIfaceTC();

  // It is not possible to copy nor assign an ext_iface.
  ExtIfaceTC(const ExtIfaceTC &) = delete;
  ExtIfaceTC &operator=(const ExtIfaceTC &) = delete;

  int get_fd() const;

 private:
  static std::set<std::string> used_ifaces;
  void load_rx();
  void load_tx();
  void load_egress();

  ebpf::BPF rx_;
  ebpf::BPF tx_;
  ebpf::BPF egress_;
  std::string iface_;
  const PortTC &port_;   // where is the iface_ connected to?
  int fd_;             // fd_ of the tx_ program

  static const std::string RX_CODE;
  static const std::string TX_CODE;

  bool egress_loaded;

  std::shared_ptr<spdlog::logger> logger;
};

}  // namespace polycubed
}  // namespace polycube
