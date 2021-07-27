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
#include "utils/utils.h"

#include <iostream>

namespace polycube {
namespace polycubed {

CubeTC::CubeTC(const std::string &name, const std::string &service_name,
               const std::vector<std::string> &ingress_code,
               const std::vector<std::string> &egress_code, LogLevel level,
               bool shadow, bool span)
    : Cube(name, service_name, PatchPanel::get_tc_instance(), level,
           CubeType::TC, shadow, span) {
  // it has to be done here becuase it needs the load, compile methods
  // to be ready
  Cube::init(ingress_code, egress_code);
  if (shadow && span) {
    auto res = ingress_programs_[0]->open_perf_buffer("span_slowpath", send_packet_ns_span_mode, nullptr, this);
    if (res.code() != 0) {
      logger->error("cannot open perf ring buffer for span_slowpath: {0}", res.msg());
      throw BPFError("cannot open span_slowpath perf buffer");
    }
    start_thread_span_mode();
  }
}

CubeTC::~CubeTC() {
  // it cannot be done in Cube::~Cube() because calls a virtual method
  if (get_span())
    stop_thread_span_mode();
  Cube::uninit();
}

std::string CubeTC::get_wrapper_code() {
  return Cube::get_wrapper_code() + CUBE_TC_COMMON_WRAPPER + CUBETC_WRAPPER +
         CUBETC_HELPERS;
}

std::string CubeTC::get_redirect_code() {
  std::stringstream ss;

  for(auto const& [index, val] : forward_chain_) {
    ss << "case " << index << ":\n";
    ss << "skb->cb[0] = " << val.first << ";\n";

    if (val.second) {
      // It is a net interface
      ss << "return bpf_redirect(" << (val.first & 0xffff) << ", 0);\n";
    } else {
      ss << "nodes.call(skb, " << (val.first & 0xffff) << ");\n";
    }

    ss << "break;\n";
  }

  return ss.str();
}

void CubeTC::do_compile(int id, ProgramType type, LogLevel level_,
                        ebpf::BPF &bpf, const std::string &code, int index, bool shadow, bool span) {
  std::string wrapper_code = get_wrapper_code();
  utils::replaceStrAll(wrapper_code, "_REDIRECT_CODE", get_redirect_code());

  // compile ebpf program
  std::string all_code(wrapper_code +
                       DatapathLog::get_instance().parse_log(code));

  std::vector<std::string> cflags(cflags_);
  cflags.push_back("-DCUBE_ID=" + std::to_string(id));
  cflags.push_back("-DLOG_LEVEL=LOG_" + logLevelString(level_));
  cflags.push_back(std::string("-DCTXTYPE=") + std::string("__sk_buff"));
  cflags.push_back(std::string("-DPOLYCUBE_PROGRAM_TYPE=" +
                   std::to_string(static_cast<int>(type))));
  if (shadow) {
    cflags.push_back("-DSHADOW");
    if (span)
      cflags.push_back("-DSPAN");
  }

  std::lock_guard<std::mutex> guard(bcc_mutex);
  auto init_res = bpf.init(all_code, cflags);

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
  do_compile(get_id(), type, level_, bpf, code, index, get_shadow(), get_span());
}

int CubeTC::load(ebpf::BPF &bpf, ProgramType type) {
  return do_load(bpf);
}

void CubeTC::unload(ebpf::BPF &bpf, ProgramType type) {
  do_unload(bpf);
}

void CubeTC::send_packet_ns_span_mode(void *cb_cookie, void *data, int data_size) {
  PacketIn *md = static_cast<PacketIn *>(data);

  uint8_t *data_ = static_cast<uint8_t *>(data);
  data_ += sizeof(PacketIn);

  CubeTC *cube = static_cast<CubeTC *>(cb_cookie);
  if (cube == nullptr)
    throw std::runtime_error("Bad cube");

  try {
    std::vector<uint8_t> packet(data_, data_ + md->packet_len);

    /* send packet to the namespace only if the port has even index
     * because if the port has odd index the packet is already sent
     * to the namespace by the datapath */
    if (!(md->port_id % 2)) {
      auto in_port = cube->ports_by_index_.at(md->port_id);
      in_port->send_packet_ns(packet);
    }

  } catch(const std::exception &e) {
    // TODO: ignore the problem, what else can we do?
    
    // This function (i.e., send_packet_ns_span_mode() ) is static, so we cannot use 
    // standard logging primitive (i.e., logger->warn() ) here.
    spdlog::get("polycubed")->warn("Error processing packet in span mode - event: {}", e.what());
  }
}

void CubeTC::start_thread_span_mode() {
  /* create a thread that polls the perf ring buffer "span_slowpath"
   * if there are packets to send to the namespace */
  auto f = [&]() -> void {
    stop_thread_ = false;
    while (!stop_thread_) {
      ingress_programs_[0]->poll_perf_buffer("span_slowpath", 500);
    }
  };
  std::unique_ptr<std::thread> uptr(new std::thread(f));
  pkt_in_thread_ = std::move(uptr);
}

void CubeTC::stop_thread_span_mode() {
  stop_thread_ = true;
  if (pkt_in_thread_) {
    pkt_in_thread_->join();
  }
}

void CubeTC::set_span(const bool value) {
  if (!shadow_) {
    throw std::runtime_error("Span mode is not present in no-shadow services");
  }
  if (span_ == value) {
    return;
  }
  span_ = value;

  if (!span_) {
    stop_thread_span_mode();
  }

  reload_all();

  if (span_) {
    auto res = ingress_programs_[0]->open_perf_buffer("span_slowpath", send_packet_ns_span_mode, nullptr, this);
    if (res.code() != 0) {
      logger->error("cannot open perf ring buffer for span_slowpath: {0}", res.msg());
      throw BPFError("cannot open span_slowpath perf buffer");
    }
    start_thread_span_mode();
  }
}

const std::string CubeTC::CUBE_TC_COMMON_WRAPPER = R"(
BPF_TABLE("extern", int, int, nodes, _POLYCUBE_MAX_NODES);
#if defined(SHADOW) && defined(SPAN)
BPF_PERF_OUTPUT(span_slowpath);
#endif

struct controller_table_t {
  int key;
  u32 leaf;
  /* map.perf_submit(ctx, data, data_size) */
  int (*perf_submit) (void *, void *, u32);
  int (*perf_submit_skb) (void *, u32, void *, u32);
  u32 data[0];
};
__attribute__((section("maps/extern")))
struct controller_table_t controller_tc;

#if defined(SHADOW) && defined(SPAN)
static __always_inline
int to_controller_span(struct CTXTYPE *skb, struct pkt_metadata md) {
  int r = span_slowpath.perf_submit_skb(skb, md.packet_len, &md, sizeof(md));
  return r;
}
#endif

static __always_inline
int pcn_pkt_drop(struct CTXTYPE *skb, struct pkt_metadata *md) {
  return RX_DROP;
}

static __always_inline
int pcn_pkt_controller_with_metadata(struct CTXTYPE *skb,
                                     struct pkt_metadata *md,
                                     u16 reason,
                                     u32 metadata[3]) {
  md->md[0] = metadata[0];
  md->md[1] = metadata[1];
  md->md[2] = metadata[2];
  return pcn_pkt_controller(skb, md, reason);
}

static __always_inline
void call_ingress_program_with_metadata(struct CTXTYPE *skb,
                                        struct pkt_metadata *md, int index) {
  // Save the traffic class for the next program in case it was changed
  // by the current one
  skb->mark = md->traffic_class;

  call_ingress_program(skb, index);
}

static __always_inline
void call_egress_program_with_metadata(struct CTXTYPE *skb,
                                       struct pkt_metadata *md, int index) {
  // Save the traffic class for the next program in case it was changed
  // by the current one
  skb->mark = md->traffic_class;

  call_egress_program(skb, index);
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
  switch (out_port) {
    _REDIRECT_CODE;
  }

  return TC_ACT_SHOT;
}

#if defined(SHADOW) && defined(SPAN)
static __always_inline
void packet_span(struct CTXTYPE *skb, u32 out_port) {
  // Send the incoming packet in the namespace
  struct pkt_metadata md = {};
  md.in_port = out_port;
  md.cube_id = CUBE_ID;
  md.packet_len = skb->len;
  to_controller_span(skb, md);
}
#endif

int handle_rx_wrapper(struct CTXTYPE *skb) {
  //bpf_trace_printk("" MODULE_UUID_SHORT ": rx:%d\n", skb->cb[0]);
  struct pkt_metadata md = {};
  volatile u32 x = skb->cb[0]; // volatile to avoid a rare verifier error
  md.in_port = x >> 16;
  md.cube_id = CUBE_ID;
  md.packet_len = skb->len;
  md.traffic_class = skb->mark;
  skb->cb[0] = md.in_port << 16 | CUBE_ID;

  // Check if the cube is shadow and the in_port has odd index
#ifdef SHADOW
  if (md.in_port % 2) {
    u32 port_out = md.in_port - 1;
    // forward on the port with even index
    return forward(skb, port_out);
  }
#ifdef SPAN
  packet_span(skb, md.in_port);
#endif
#endif

  int rc = handle_rx(skb, &md);

  // Save the traffic class for the next program in case it was changed
  // by the current one
  skb->mark = md.traffic_class;

  switch (rc) {
    case RX_REDIRECT:
      // FIXME: reason is right, we are reusing the field
      return forward(skb, md.reason);
    case RX_DROP:
      return TC_ACT_SHOT;
    case RX_OK: {
#if POLYCUBE_PROGRAM_TYPE == 1  // EGRESS
      // If there is another egress program call it, otherwise let the packet
      // pass

      int port = md.in_port;
      u16 *next = egress_next.lookup(&port);
      if (!next) {
        return TC_ACT_SHOT;
      }

      if (*next == 0) {
        return TC_ACT_SHOT;
      } else if (*next == 0xffff) {
        return TC_ACT_OK;
      } else {
        nodes.call(skb, *next);
      }

#else  // INGRESS
      return TC_ACT_OK;
#endif
    }
  }
  return TC_ACT_SHOT;
}

#if POLYCUBE_PROGRAM_TYPE == 0  // Only INGRESS programs can redirect
static __always_inline
int pcn_pkt_redirect(struct CTXTYPE *skb,
                     struct pkt_metadata *md, u32 port) {
  // FIXME: this is just to reuse this field
  md->reason = port;
#if defined(SHADOW) && defined(SPAN)
  if (!(port % 2))
    packet_span(skb, port);
#endif
  return RX_REDIRECT;
}
#endif

static __always_inline
int pcn_pkt_redirect_ns(struct CTXTYPE *skb,
                     struct pkt_metadata *md, u32 port) {
// if SPAN is true the packet is not sent to the namespace to avoid duplication
#if defined(SHADOW) && !defined(SPAN)
  // FIXME: this is just to reuse this field
  // forward on the port with odd index
  md->reason = port + 1;
  return RX_REDIRECT;
#endif
  return TC_ACT_SHOT;
}

static __always_inline
int pcn_pkt_controller(struct CTXTYPE *skb, struct pkt_metadata *md,
                       u16 reason) {
  // If the packet is tagged add the tagged in the packet itself, otherwise it
  // will be lost
  if (skb->vlan_present) {
    volatile __u32 vlan_tci = skb->vlan_tci;
    volatile __u32 vlan_proto = skb->vlan_proto;
    bpf_skb_vlan_push(skb, vlan_proto, vlan_tci);
  }

  md->cube_id = CUBE_ID;
  md->packet_len = skb->len;
  md->reason = reason;

  return controller_tc.perf_submit_skb(skb, skb->len, md, sizeof(*md));
}
)";

}  // namespace polycubed
}  // namespace polycube
