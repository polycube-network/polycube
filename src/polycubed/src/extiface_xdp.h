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

#include "extiface.h"
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

class ExtIfaceXDP : public ExtIface {
 public:
  ExtIfaceXDP(const std::string &iface, int attach_flags = 0);
  virtual ~ExtIfaceXDP();

 protected:
  // XDP egress programs can't rely on the TC_EGRESS hook to be executed.
  // This version of update_indexes() also connects the egress programs chain to
  // the peer of the interface (if present).
  void update_indexes() override;

 private:
  virtual std::string get_ingress_code() const;
  virtual std::string get_egress_code() const;
  virtual std::string get_tx_code() const;
  virtual bpf_prog_type get_program_type() const;

  uint16_t redir_index_;

  static const std::string XDP_PROG_CODE;
  static const std::string XDP_REDIR_PROG_CODE;

  int attach_flags_;
};

}  // namespace polycubed
}  // namespace polycube
