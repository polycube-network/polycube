/*
 * Copyright 2016 PLUMgrid
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

#include "extiface_tc.h"
#include "exceptions.h"
#include "netlink.h"
#include "patchpanel.h"

#include <iostream>

namespace polycube {
namespace polycubed {

std::set<std::string> ExtIfaceTC::used_ifaces;

const std::string ExtIfaceTC::RX_CODE = R"(
#include <uapi/linux/pkt_cls.h>

BPF_TABLE("extern", int, int, nodes, _POLYCUBE_MAX_NODES);

int ingress(struct __sk_buff *skb) {
  //bpf_trace_printk("ingress %d %x\n", skb->ifindex, ENDPOINT & 0xffff);
  skb->cb[0] = ENDPOINT;
  nodes.call(skb, ENDPOINT & 0xffff);
  //bpf_trace_printk("ingress drop\n");
  return TC_ACT_OK;
}
)";

const std::string ExtIfaceTC::TX_CODE = R"(
#include <uapi/linux/pkt_cls.h>
int egress(struct __sk_buff *skb) {
  //bpf_trace_printk("egress %d\n", INTERFACE_ID);
  bpf_redirect(INTERFACE_ID, 0);
  return TC_ACT_REDIRECT;
}
)";

ExtIfaceTC::ExtIfaceTC(const std::string &iface, const PortTC &port)
    : iface_(iface), port_(port), logger(spdlog::get("polycubed")) {
  if (used_ifaces.count(iface) != 0) {
    throw std::runtime_error("Iface already in use");
  }

  used_ifaces.insert(iface);

  try {
    load_rx();
    load_tx();
    load_egress();

    PatchPanel::get_tc_instance().add(*this);
  } catch (...) {
    used_ifaces.erase(iface);
    throw;
  }
}

ExtIfaceTC::~ExtIfaceTC() {
  PatchPanel::get_tc_instance().remove(*this);
  Netlink::getInstance().detach_from_tc(iface_);
  rx_.unload_func("ingress");
  tx_.unload_func("egress");
  if (egress_loaded) {
    Netlink::getInstance().detach_from_tc(iface_, ATTACH_MODE::EGRESS);
    egress_.unload_func("ingress");
  }
  used_ifaces.erase(iface_);
}

void ExtIfaceTC::load_rx() {
  // compile rx_ ebpf program
  std::vector<std::string> flags_rx;
  uint32_t next = port_.serialize_ingress();
  flags_rx.push_back(std::string("-DENDPOINT=") + std::to_string(next));
  flags_rx.push_back(std::string("-D_POLYCUBE_MAX_NODES=") +
                     std::to_string(_POLYCUBE_MAX_NODES));

  auto init_res = rx_.init(RX_CODE, flags_rx);
  if (init_res.code() != 0) {
    logger->error("Error compiling rx program: {}", init_res.msg());
    throw BPFError("failed to init ebpf program:" + init_res.msg());
  }

  // load rx_ program
  int fd_rx;
  auto load_res = rx_.load_func("ingress", BPF_PROG_TYPE_SCHED_CLS, fd_rx);
  if (load_res.code() != 0) {
    logger->error("Error loading rx program: {}", load_res.msg());
    throw BPFError("failed to load ebpf program: " + load_res.msg());
  }

  // attach to TC
  Netlink::getInstance().attach_to_tc(iface_, fd_rx);
}

void ExtIfaceTC::load_egress() {
  // compile rx_ ebpf program
  std::vector<std::string> flags_rx;
  uint32_t next = port_.serialize_egress();
  if (!next)
    return;
  flags_rx.push_back(std::string("-DENDPOINT=") + std::to_string(next));
  flags_rx.push_back(std::string("-D_POLYCUBE_MAX_NODES=") +
                     std::to_string(_POLYCUBE_MAX_NODES));

  auto init_res = egress_.init(RX_CODE, flags_rx);
  if (init_res.code() != 0) {
    logger->error("Error compiling egress program: {}", init_res.msg());
    throw BPFError("failed to init ebpf program:" + init_res.msg());
  }

  // load rx_ program
  int fd_rx;
  auto load_res = egress_.load_func("ingress", BPF_PROG_TYPE_SCHED_CLS, fd_rx);
  if (load_res.code() != 0) {
    logger->error("Error loading egress program: {}", load_res.msg());
    throw BPFError("failed to load ebpf program: " + load_res.msg());
  }
  egress_loaded = true;

  // attach to TC
  Netlink::getInstance().attach_to_tc(iface_, fd_rx, ATTACH_MODE::EGRESS);
}

void ExtIfaceTC::load_tx() {
  std::vector<std::string> flags;
  int iface_id = Netlink::getInstance().get_iface_index(iface_);
  flags.push_back(std::string("-DINTERFACE_ID=") + std::to_string(iface_id));
  auto init_res = tx_.init(TX_CODE, flags);
  if (init_res.code() != 0) {
    logger->error("Error compiling tx program: {}", init_res.msg());
    throw BPFError("failed to init ebpf program:" + init_res.msg());
  }

  // load tx_ program
  auto load_res = tx_.load_func("egress", BPF_PROG_TYPE_SCHED_CLS, fd_);
  if (load_res.code() != 0) {
    logger->error("Error loading tx program: {}", load_res.msg());
    throw BPFError("failed to load ebpf program: " + load_res.msg());
  }
}

int ExtIfaceTC::get_fd() const {
  return fd_;
}

}  // namespace polycubed
}  // namespace polycube
