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
#include "port_xdp.h"

#include <api/BPF.h>
#include <spdlog/spdlog.h>

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <linux/if_link.h>
#include <netdb.h>
#include <spdlog/spdlog.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <list>
#include <set>

namespace polycube {
namespace polycubed {

class PortXDP;

class ExtIfaceXDP : public Node {
public:
  ExtIfaceXDP(const std::string &iface, const PortXDP &port);
  virtual ~ExtIfaceXDP();

  // It is not possible to copy or assign an ext_iface.
  ExtIfaceXDP(const ExtIfaceXDP &) = delete;
  ExtIfaceXDP &operator=(const ExtIfaceXDP &) = delete;

  int get_fd() const;

private:
  static std::set<std::string> used_ifaces;
  void load_xdp_program();
  void load_redirect_program();
  void load_egress();

  std::unique_ptr<ebpf::BPF> xdp_prog_;
  std::unique_ptr<ebpf::BPF> xdp_redir_prog_;
  const std::string iface_name_;
  const PortXDP &port_;
  static const std::string XDP_PROG_CODE;
  static const std::string XDP_REDIR_PROG_CODE;
  int fd_;
  int ifindex_;

  ebpf::BPF egress_;
  bool egress_loaded;

  std::shared_ptr<spdlog::logger> logger;
};


} // namespace polycubed
} // namespace polycube
