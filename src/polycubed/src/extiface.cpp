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

#include "extiface.h"
#include "bcc_mutex.h"
#include "exceptions.h"
#include "netlink.h"
#include "patchpanel.h"
#include "port.h"

#include "extiface_tc.h"

#include <iostream>

namespace polycube {
namespace polycubed {

std::set<std::string> ExtIface::used_ifaces;

ExtIface::ExtIface(const std::string &iface)
    : iface_(iface), peer_(nullptr), logger(spdlog::get("polycubed")) {
  if (used_ifaces.count(iface) != 0) {
    throw std::runtime_error("Iface already in use");
  }

  used_ifaces.insert(iface);
}

ExtIface::~ExtIface() {
  // std::unique_lock<std::mutex> bcc_guard(bcc_mutex);
  ingress_program_.unload_func("handler");
  tx_.unload_func("handler");
  if (egress_program_loaded) {
    Netlink::getInstance().detach_from_tc(iface_, ATTACH_MODE::EGRESS);
    egress_program_.unload_func("handler");
  }
  used_ifaces.erase(iface_);
}

uint16_t ExtIface::get_index() const {
  return index_;
}

int ExtIface::load_ingress() {
  ebpf::StatusTuple res(0);

  std::vector<std::string> flags;

  flags.push_back(std::string("-DMOD_NAME=") + iface_);
  flags.push_back(std::string("-D_POLYCUBE_MAX_NODES=") +
                  std::to_string(PatchPanel::_POLYCUBE_MAX_NODES));

  res = ingress_program_.init(get_ingress_code(), flags);
  if (res.code() != 0) {
    logger->error("Error compiling ingress program: {}", res.msg());
    throw BPFError("failed to init ebpf program:" + res.msg());
  }

  int fd_rx;
  res = ingress_program_.load_func("handler", get_program_type(), fd_rx);
  if (res.code() != 0) {
    logger->error("Error loading ingress program: {}", res.msg());
    throw BPFError("failed to load ebpf program: " + res.msg());
  }

  return fd_rx;
}

int ExtIface::load_tx() {
  ebpf::StatusTuple res(0);
  std::vector<std::string> flags;

  int iface_index = Netlink::getInstance().get_iface_index(iface_);
  flags.push_back(std::string("-DINTERFACE_INDEX=") +
                  std::to_string(iface_index));

  res = tx_.init(get_tx_code(), flags);
  if (res.code() != 0) {
    logger->error("Error compiling tx program: {}", res.msg());
    throw BPFError("failed to init ebpf program:" + res.msg());
  }

  int fd_tx_;
  auto load_res = tx_.load_func("handler", get_program_type(), fd_tx_);
  if (load_res.code() != 0) {
    logger->error("Error loading tx program: {}", load_res.msg());
    throw BPFError("failed to load ebpf program: " + load_res.msg());
  }

  return fd_tx_;
}

int ExtIface::load_egress() {
  ebpf::StatusTuple res(0);
  std::vector<std::string> flags;
  flags.push_back(std::string("-D_POLYCUBE_MAX_NODES=") +
                  std::to_string(PatchPanel::_POLYCUBE_MAX_NODES));

  res = egress_program_.init(get_egress_code(), flags);
  if (res.code() != 0) {
    logger->error("Error compiling egress program: {}", res.msg());
    throw BPFError("failed to init ebpf program:" + res.msg());
  }

  int fd_egress;
  res =
      egress_program_.load_func("handler", BPF_PROG_TYPE_SCHED_CLS, fd_egress);
  if (res.code() != 0) {
    logger->error("Error loading egress program: {}", res.msg());
    throw std::runtime_error("failed to load ebpf program: " + res.msg());
  }
  egress_program_loaded = true;

  // attach to TC
  Netlink::getInstance().attach_to_tc(iface_, fd_egress, ATTACH_MODE::EGRESS);

  return fd_egress;
}

uint16_t ExtIface::get_port_id() const {
  return 0;
}

void ExtIface::set_peer_iface(PeerIface *peer) {
  peer_ = peer;
  set_next_index(peer_ ? peer_->get_index() : 0);

  Port *peer_port = dynamic_cast<Port *>(peer);
  if (peer_port) {
    set_next(peer_port->get_egress_index(), ProgramType::EGRESS);
  }
}

PeerIface *ExtIface::get_peer_iface() {
  return peer_;
}

void ExtIface::set_next_index(uint16_t index) {
  set_next(index, ProgramType::INGRESS);
}

void ExtIface::set_next(uint16_t next, ProgramType type) {
  uint16_t port_id = 0;
  if (peer_) {
    port_id = peer_->get_port_id();
  }
  uint32_t value = port_id << 16 | next;
  if (type == ProgramType::INGRESS) {
    auto index_map = ingress_program_.get_array_table<uint32_t>("index_map");
    index_map.update_value(0, value);
  } else if (type == ProgramType::EGRESS) {
    auto index_map = egress_program_.get_array_table<uint32_t>("index_map");
    index_map.update_value(0, value);
  }
}

}  // namespace polycubed
}  // namespace polycube
