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

#include <api/BPF.h>
#include <spdlog/spdlog.h>

#include <stdio.h>
#include <set>

#include "peer_iface.h"
#include "polycube/services/cube_iface.h"

using polycube::service::ProgramType;

namespace polycube {
namespace polycubed {

class ExtIface : public PeerIface {
 public:
  ExtIface(const std::string &iface);
  virtual ~ExtIface();

  // It is not possible to copy nor assign an ext_iface.
  ExtIface(const ExtIface &) = delete;
  ExtIface &operator=(const ExtIface &) = delete;

  uint16_t get_index() const override;
  uint16_t get_port_id() const override;
  void set_peer_iface(PeerIface *peer) override;
  PeerIface *get_peer_iface() override;

  void set_next_index(uint16_t index);
  bool is_used() const;

 protected:
  static std::set<std::string> used_ifaces;
  int load_ingress();
  int load_egress();
  int load_tx();

  virtual std::string get_ingress_code() const = 0;
  virtual std::string get_egress_code() const = 0;
  virtual std::string get_tx_code() const = 0;
  virtual bpf_prog_type get_program_type() const = 0;

  void update_indexes() override;
  int calculate_cube_index(int index) override;

  ebpf::BPF ingress_program_;
  ebpf::BPF egress_program_;
  void set_next(uint16_t next, ProgramType type);

  ebpf::BPF tx_;
  std::string iface_;
  PeerIface *peer_;  // the interface is connected to

  uint16_t index_;
  uint16_t next_index_;

  bool egress_program_loaded;

  mutable std::mutex iface_mutex_;

  std::shared_ptr<spdlog::logger> logger;
};

}  // namespace polycubed
}  // namespace polycube
