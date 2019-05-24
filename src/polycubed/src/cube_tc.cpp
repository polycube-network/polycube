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

#include "cube_tc.h"
#include "datapath_log.h"
#include "exceptions.h"
#include "patchpanel.h"

#include <iostream>

namespace polycube {
namespace polycubed {

CubeTC::CubeTC(const std::string &name, const std::string &service_name,
               const std::vector<std::string> &ingress_code,
               const std::vector<std::string> &egress_code, LogLevel level)
    : Cube(name, service_name, PatchPanel::get_tc_instance(),
           PatchPanel::get_tc_instance(), level, CubeType::TC) {
  // it has to be done here becuase it needs the load, compile methods
  // to be ready
  Cube::init(ingress_code, egress_code);
}

CubeTC::~CubeTC() {
  // it cannot be done in Cube::~Cube() because calls a virtual method
  Cube::uninit();
}

std::string CubeTC::get_wrapper_code() {
  return Cube::get_wrapper_code() + CUBE_TC_COMMON_WRAPPER + CUBETC_WRAPPER +
         CUBETC_HELPERS;
}

void CubeTC::do_compile(int id, ProgramType type, LogLevel level_,
                        ebpf::BPF &bpf, const std::string &code, int index) {
  // compile ebpf program
  std::string all_code(get_wrapper_code() +
                       DatapathLog::get_instance().parse_log(code));

  std::vector<std::string> cflags_(Cube::cflags);
  cflags_.push_back("-DCUBE_ID=" + std::to_string(id));
  cflags_.push_back("-DLOG_LEVEL=LOG_" + logLevelString(level_));
  cflags_.push_back(std::string("-DCTXTYPE=") + std::string("__sk_buff"));

  std::lock_guard<std::mutex> guard(bcc_mutex);
  auto init_res = bpf.init(all_code, cflags_);

  if (init_res.code() != 0) {
    // logger->error("failed to init bpf program: {0}", init_res.msg());
    throw BPFError("failed to init ebpf program: " + init_res.msg());
  }
}

int CubeTC::do_load(ebpf::BPF &bpf) {
  int fd_;

  std::lock_guard<std::mutex> guard(bcc_mutex);
  auto load_res =
      bpf.load_func("handle_rx_wrapper", BPF_PROG_TYPE_SCHED_CLS, fd_);

  if (load_res.code() != 0) {
    // logger->error("failed to load bpf program: {0}", load_res.msg());
    throw BPFError("failed to load bpf program: " + load_res.msg());
  }

  return fd_;
}

void CubeTC::do_unload(ebpf::BPF &bpf) {
  std::lock_guard<std::mutex> guard(bcc_mutex);
  auto load_res = bpf.unload_func("handle_rx_wrapper");
  // TODO: what to do with load_res?
}

void CubeTC::compile(ebpf::BPF &bpf, const std::string &code, int index,
                     ProgramType type) {
  do_compile(get_id(), type, level_, bpf, code, index);
}

int CubeTC::load(ebpf::BPF &bpf, ProgramType type) {
  return do_load(bpf);
}

void CubeTC::unload(ebpf::BPF &bpf, ProgramType type) {
  do_unload(bpf);
}

const std::string CubeTC::CUBE_TC_COMMON_WRAPPER = R"(
BPF_TABLE("extern", int, int, nodes, _POLYCUBE_MAX_NODES);

static __always_inline
int to_controller(struct CTXTYPE *skb, u16 reason) {
  skb->cb[1] = reason;
  nodes.call(skb, CONTROLLER_MODULE_INDEX);
  //bpf_trace_printk("to controller miss\n");
  return TC_ACT_OK;
}

static __always_inline
int pcn_pkt_drop(struct CTXTYPE *skb, struct pkt_metadata *md) {
  return RX_DROP;
}

static __always_inline
int pcn_pkt_controller(struct CTXTYPE *skb, struct pkt_metadata *md,
                       u16 reason) {
  md->reason = reason;
  return RX_CONTROLLER;
}

static __always_inline
int pcn_pkt_controller_with_metadata_stack(struct CTXTYPE *skb,
                                           struct pkt_metadata *md,
                                           u16 reason,
                                           u32 metadata[3]) {
  skb->cb[0] |= 0x8000;
  skb->cb[2] = metadata[0];
  skb->cb[3] = metadata[1];
  skb->cb[4] = metadata[2];
  return pcn_pkt_controller(skb, md, reason);
}

static __always_inline
int pcn_pkt_controller_with_metadata(struct CTXTYPE *skb,
                                     struct pkt_metadata *md,
                                     u16 reason,
                                     u32 metadata[3]) {
  skb->cb[2] = metadata[0];
  skb->cb[3] = metadata[1];
  skb->cb[4] = metadata[2];
  return pcn_pkt_controller(skb, md, reason);
}
)";

const std::string CubeTC::CUBETC_HELPERS = R"(
/* checksum related */
static __always_inline
int pcn_l3_csum_replace(struct CTXTYPE *ctx, u32 csum_offset,
                        u32 old_value, u32 new_value, u32 flags) {
  return bpf_l3_csum_replace(ctx, csum_offset, old_value, new_value, flags);
}

static __always_inline
int pcn_l4_csum_replace(struct CTXTYPE *ctx, u32 csum_offset,
                        u32 old_value, u32 new_value, u32 flags) {
  return bpf_l4_csum_replace(ctx, csum_offset, old_value, new_value, flags);
}

static __always_inline
__wsum pcn_csum_diff(__be32 *from, u32 from_size, __be32 *to,
                     u32 to_size, __wsum seed) {
  return bpf_csum_diff(from, from_size, to, to_size, seed);
}

/* vlan related */
static __always_inline
bool pcn_is_vlan_present(struct CTXTYPE *ctx) {
  return ctx->vlan_present;
}

static __always_inline
int pcn_get_vlan_id(struct CTXTYPE *ctx) {
  return ctx->vlan_tci & 0x0fff;
}

static __always_inline
int pcn_get_vlan_proto(struct CTXTYPE *ctx) {
  return ctx->vlan_proto;
}

static __always_inline
int pcn_vlan_pop_tag(struct CTXTYPE *ctx) {
  return bpf_skb_vlan_pop(ctx);
}

static __always_inline
int pcn_vlan_push_tag(struct CTXTYPE *ctx, u16 eth_proto, u32 vlan_id) {
  return bpf_skb_vlan_push(ctx, eth_proto, vlan_id);
}

)";

const std::string CubeTC::CUBETC_WRAPPER = R"(
static __always_inline
int forward(struct CTXTYPE *skb, u32 out_port) {
  u32 *next = forward_chain_.lookup(&out_port);
  if (next) {
    skb->cb[0] = *next;
    //bpf_trace_printk("fwd: port: %d, next: 0x%x\n", out_port, *next);
    nodes.call(skb, *next & 0xffff);
  }
  //bpf_trace_printk("fwd:%d=0\n", out_port);
  return TC_ACT_SHOT;
}

int handle_rx_wrapper(struct CTXTYPE *skb) {
  //bpf_trace_printk("" MODULE_UUID_SHORT ": rx:%d\n", skb->cb[0]);
  struct pkt_metadata md = {};
  volatile u32 x = skb->cb[0]; // volatile to avoid a rare verifier error
  md.in_port = x >> 16;
  md.cube_id = CUBE_ID;
  md.packet_len = skb->len;
  skb->cb[0] = md.in_port << 16 | CUBE_ID;
  int rc = handle_rx(skb, &md);

  switch (rc) {
    case RX_REDIRECT:
      // FIXME: reason is right, we are reusing the field
      return forward(skb, md.reason);
    case RX_DROP:
      return TC_ACT_SHOT;
    case RX_CONTROLLER:
      return to_controller(skb, md.reason);
    case RX_OK:
      return TC_ACT_OK;
  }
  return TC_ACT_SHOT;
}

static __always_inline
int pcn_pkt_redirect(struct CTXTYPE *skb,
                     struct pkt_metadata *md, u32 port) {
  // FIXME: this is just to reuse this field
  md->reason = port;
  return RX_REDIRECT;
}
)";

}  // namespace polycubed
}  // namespace polycube
