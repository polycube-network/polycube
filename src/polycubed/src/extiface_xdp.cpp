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

#include "extiface_xdp.h"
#include "exceptions.h"
#include "extiface_tc.h"
#include "netlink.h"
#include "patchpanel.h"
#include "port_xdp.h"

#include <iostream>

namespace polycube {
namespace polycubed {

std::set<std::string> ExtIfaceXDP::used_ifaces;

const std::string ExtIfaceXDP::XDP_PROG_CODE = R"(
#define KBUILD_MODNAME "MOD_NAME"
#include <bcc/helpers.h>
#include <bcc/proto.h>

#include <uapi/linux/bpf.h>
#include <uapi/linux/if_ether.h>
#include <uapi/linux/if_packet.h>
#include <uapi/linux/if_vlan.h>
#include <uapi/linux/in.h>
#include <uapi/linux/ip.h>
#include <uapi/linux/ipv6.h>

struct pkt_metadata {
  u16 module_index;
  u16 in_port;
  u32 packet_len;

  // used to send data to controller
  u16 reason;
  u32 md[3];
} __attribute__((packed));

BPF_TABLE("extern", int, int, xdp_nodes, _POLYCUBE_MAX_NODES);
BPF_TABLE("extern", u32, struct pkt_metadata, port_md, 1);

int xdp_ingress(struct xdp_md *ctx) {
    void* data_end = (void*)(long)ctx->data_end;
    void* data = (void*)(long)ctx->data;
    u32 key = 0;
    u16 inport = INPORT;
    u16 module_id = ENDPOINT;
    struct pkt_metadata md = {};

    struct ethhdr *eth = data;
    u64 nh_off;

    nh_off = sizeof(*eth);
    if (data + nh_off  > data_end)
        return XDP_DROP;

    md.in_port = inport;
    md.module_index = module_id;
    md.packet_len = (u32)(data_end - data);

    port_md.update(&key, &md);

    //bpf_trace_printk("Executing tail call for port %d\n", inport);
    xdp_nodes.call(ctx, ENDPOINT);
    //bpf_trace_printk("Tail call not executed for port %d\n", inport);
    return XDP_DROP;
}
)";

const std::string ExtIfaceXDP::XDP_REDIR_PROG_CODE = R"(
int xdp_egress(struct xdp_md *ctx) {
  //bpf_trace_printk("Redirect to ifindex %d\n", INTERFACE_ID);
  return bpf_redirect(INTERFACE_ID, 0);
}
)";

ExtIfaceXDP::ExtIfaceXDP(const std::string &iface, const PortXDP &port)
    : iface_name_(iface), port_(port), logger(spdlog::get("polycubed")) {
  if (used_ifaces.count(iface) != 0) {
    throw std::runtime_error("Iface already in use");
  }

  used_ifaces.insert(iface);

  xdp_prog_ = std::unique_ptr<ebpf::BPF>(
      new ebpf::BPF(0, nullptr, true, port.get_cube_name()));
  xdp_redir_prog_ = std::unique_ptr<ebpf::BPF>(
      new ebpf::BPF(0, nullptr, true, port.get_cube_name()));

  ifindex_ = Netlink::getInstance().get_iface_index(iface);
  try {
    load_xdp_program();
    load_redirect_program();
    load_egress();
    PatchPanel::get_xdp_instance().add(*this);
  } catch (...) {
    used_ifaces.erase(iface);
    throw;
  }
}

ExtIfaceXDP::~ExtIfaceXDP() {
  PatchPanel::get_xdp_instance().remove(*this);
  Netlink::getInstance().detach_from_xdp(iface_name_, port_.get_attach_flags());
  xdp_prog_->unload_func("xdp_ingress");
  xdp_redir_prog_->unload_func("xdp_egress");
  if (egress_loaded) {
    Netlink::getInstance().detach_from_tc(iface_name_, ATTACH_MODE::EGRESS);
    egress_.unload_func("ingress");
  }
  used_ifaces.erase(iface_name_);
}

void ExtIfaceXDP::load_xdp_program() {
  // compile XDP ebpf program
  std::vector<std::string> flags_rx;
  flags_rx.push_back(std::string("-DENDPOINT=") +
                     std::to_string(port_.get_parent_index()));
  flags_rx.push_back(std::string("-DINPORT=") + std::to_string(port_.index()));
  flags_rx.push_back(std::string("-DMOD_NAME=") + port_.get_cube_name());
  flags_rx.push_back(std::string("-D_POLYCUBE_MAX_NODES=") +
                     std::to_string(_POLYCUBE_MAX_NODES));

  auto init_res = xdp_prog_->init(XDP_PROG_CODE, flags_rx);
  if (init_res.code() != 0) {
    logger->error("failed to init ebpf program: {0}", init_res.msg());
    throw BPFError("failed to init ebpf program:" + init_res.msg());
  }

  int fd_rx;
  // load rx_ program
  auto load_res = xdp_prog_->load_func("xdp_ingress", BPF_PROG_TYPE_XDP, fd_rx);
  if (load_res.code() != 0) {
    logger->error("failed to load ebpf program: {0}", load_res.msg());
    throw BPFError("failed to load ebpf program: " + load_res.msg());
  }

  // attach to XDP
  Netlink::getInstance().attach_to_xdp(iface_name_, fd_rx,
                                       port_.get_attach_flags());
}

void ExtIfaceXDP::load_redirect_program() {
  std::vector<std::string> flags;

  flags.push_back(std::string("-DINTERFACE_ID=") + std::to_string(ifindex_));

  auto init_res = xdp_redir_prog_->init(XDP_REDIR_PROG_CODE, flags);
  if (init_res.code() != 0) {
    logger->error("failed to init ebpf program: {0}", init_res.msg());
    throw BPFError("failed to init ebpf program:" + init_res.msg());
  }

  // load tx_ program
  auto load_res =
      xdp_redir_prog_->load_func("xdp_egress", BPF_PROG_TYPE_XDP, fd_);
  if (load_res.code() != 0) {
    logger->error("failed to load ebpf program: {0}", load_res.msg());
    throw BPFError("failed to load ebpf program: " + load_res.msg());
  }
}

// TODO: this function is the same in ExtIfaceTC, try to reduce to a single one
void ExtIfaceXDP::load_egress() {
  logger->info("ExtIfaceXDP::load_egress()");
  // compile rx_ ebpf program
  std::vector<std::string> flags_rx;
  uint32_t next = port_.serialize_egress();
  if (!next)
    return;
  flags_rx.push_back(std::string("-DENDPOINT=") + std::to_string(next));
  flags_rx.push_back(std::string("-D_POLYCUBE_MAX_NODES=") +
                     std::to_string(_POLYCUBE_MAX_NODES));

  auto init_res = egress_.init(ExtIfaceTC::RX_CODE, flags_rx);
  if (init_res.code() != 0) {
    std::cerr << init_res.msg() << std::endl;
    throw BPFError("failed to init ebpf program:" + init_res.msg());
  }

  // load rx_ program
  int fd_rx;
  auto load_res = egress_.load_func("ingress", BPF_PROG_TYPE_SCHED_CLS, fd_rx);
  if (load_res.code() != 0) {
    std::cerr << load_res.msg() << std::endl;
    throw BPFError("failed to load ebpf program: " + load_res.msg());
  }
  egress_loaded = true;

  // attach to TC
  Netlink::getInstance().attach_to_tc(iface_name_, fd_rx, ATTACH_MODE::EGRESS);
}

int ExtIfaceXDP::get_fd() const {
  return fd_;
}

}  // namespace polycubed
}  // namespace polycube
