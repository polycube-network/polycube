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

#include "cube_xdp.h"
#include "cube_tc.h"
#include "datapath_log.h"
#include "exceptions.h"
#include "patchpanel.h"

#include <iostream>

namespace polycube {
namespace polycubed {

CubeXDP::CubeXDP(const std::string &name, const std::string &service_name,
                 const std::vector<std::string> &ingress_code,
                 const std::vector<std::string> &egress_code, LogLevel level,
                 CubeType type)
    : Cube(name, service_name, PatchPanel::get_xdp_instance(),
           PatchPanel::get_tc_instance(), level, type),
      attach_flags_(0) {
  switch (type) {
  // FIXME: replace by definitions in if_link.h when update to new kernel.
  case CubeType::XDP_SKB:
    attach_flags_ |= 1U << 1;
    break;
  case CubeType::XDP_DRV:
    attach_flags_ |= 1U << 2;
    break;
  }

  // it has to be done here becuase it needs the load, compile methods
  // to be ready
  Cube::init(ingress_code, egress_code);
}

CubeXDP::~CubeXDP() {
  // it cannot be done in Cube::~Cube() because calls a virtual method
  Cube::uninit();
}

void CubeXDP::compile(ebpf::BPF &bpf, const std::string &code, int index,
                      ProgramType type) {
  switch (type) {
  case ProgramType::INGRESS:
    compileIngress(bpf, code);
    break;
  case ProgramType::EGRESS:
    compileEgress(bpf, code);
    break;
  default:
    throw std::runtime_error("Bad program type");
  }
}

int CubeXDP::load(ebpf::BPF &bpf, ProgramType type) {
  switch (type) {
  case ProgramType::INGRESS:
    return do_load(bpf);
  case ProgramType::EGRESS:
    return CubeTC::do_load(bpf);
  default:
    throw std::runtime_error("Bad program type");
  }
}

void CubeXDP::unload(ebpf::BPF &bpf, ProgramType type) {
  switch (type) {
  case ProgramType::INGRESS:
    return do_unload(bpf);
  case ProgramType::EGRESS:
    return CubeTC::do_unload(bpf);
  default:
    throw std::runtime_error("Bad program type");
  }
}

void CubeXDP::compileIngress(ebpf::BPF &bpf, const std::string &code) {
  std::string all_code(CUBE_H + XDP_HELPERS + XDP_WRAPPERC +
                       DatapathLog::get_instance().parse_log(code));

  std::vector<std::string> cflags_(cflags);
  cflags_.push_back(std::string("-DMOD_NAME=") + std::string(name_));
  cflags_.push_back("-DCUBE_ID=" + std::to_string(get_id()));
  cflags_.push_back("-DLOG_LEVEL=LOG_" + logLevelString(level_));
  cflags_.push_back(std::string("-DRETURNCODE=") + std::string("XDP_DROP"));
  cflags_.push_back(std::string("-DPOLYCUBE_XDP=1"));
  cflags_.push_back(std::string("-DCTXTYPE=") + std::string("xdp_md"));

  std::lock_guard<std::mutex> guard(bcc_mutex);
  auto init_res = bpf.init(all_code, cflags_);

  if (init_res.code() != 0) {
    logger->error("failed to init XDP program: {0}", init_res.msg());
    throw BPFError("failed to init XDP program: " + init_res.msg());
  }
  logger->debug("XDP program compileed");
}

void CubeXDP::compileEgress(ebpf::BPF &bpf, const std::string &code) {
  // compile ebpf program
  std::string all_code(CubeTC::CUBE_H + CubeTC::WRAPPERC +
                       DatapathLog::get_instance().parse_log(code));

  std::vector<std::string> cflags_(cflags);
  cflags_.push_back("-DCUBE_ID=" + std::to_string(get_id()));
  cflags_.push_back("-DLOG_LEVEL=LOG_" + logLevelString(level_));
  cflags_.push_back(std::string("-DCTXTYPE=") + std::string("__sk_buff"));

  std::lock_guard<std::mutex> guard(bcc_mutex);
  auto init_res = bpf.init(all_code, cflags_);

  if (init_res.code() != 0) {
    // logger->error("failed to init bpf program: {0}", init_res.msg());
    throw BPFError("failed to init ebpf program: " + init_res.msg());
  }
}

int CubeXDP::do_load(ebpf::BPF &bpf) {
  int fd_;
  std::lock_guard<std::mutex> guard(bcc_mutex);
  auto load_res =
      bpf.load_func("handle_rx_xdp_wrapper", BPF_PROG_TYPE_XDP, fd_);

  if (load_res.code() != 0) {
    logger->error("failed to load XDP program: {0}", load_res.msg());
    throw BPFError("Failed to load XDP program: " + load_res.msg());
  }
  logger->debug("XDP program loaded with fd {0}", fd_);

  return fd_;
}

void CubeXDP::do_unload(ebpf::BPF &bpf) {
  std::lock_guard<std::mutex> guard(bcc_mutex);
  auto load_res = bpf.unload_func("handle_rx_xdp_wrapper");
  //TODO: Remove also from the xdp_prog list?
  logger->debug("XDP program unloaded");
}

const std::string CubeXDP::XDP_WRAPPERC = R"(
BPF_TABLE("extern", u32, struct pkt_metadata, port_md, 1);
BPF_TABLE("extern", int, int, xdp_nodes, _POLYCUBE_MAX_NODES);

int handle_rx_xdp_wrapper(struct CTXTYPE *ctx) {
  u32 inport_key = 0;
  struct pkt_metadata *int_md;
  struct pkt_metadata md = {};

  int_md = port_md.lookup(&inport_key);
  if (int_md) {
    md.cube_id = CUBE_ID;
    md.in_port = int_md->in_port;
    md.packet_len = int_md->packet_len;

    int rc = handle_rx(ctx, &md);

    switch (rc) {
    case RX_DROP:
      return XDP_DROP;
    case RX_OK:
      return XDP_PASS;
    default:
      return rc;
    }
  }

  return XDP_DROP;
}

static __always_inline
int pcn_pkt_redirect(struct CTXTYPE *pkt, struct pkt_metadata *md, u32 out_port) {
  u32 *next = forward_chain_.lookup(&out_port);
  if (next) {
    u32 inport_key = 0;
    md->in_port = (*next) >> 16;  // update port_id for next module.
    port_md.update(&inport_key, md);
    xdp_nodes.call(pkt, *next & 0xffff);
  }

  return XDP_ABORTED;
}

static __always_inline
int pcn_pkt_controller(struct CTXTYPE *pkt, struct pkt_metadata *md, u16 reason) {
  u32 inport_key = 0;
  md->reason = reason;

  port_md.update(&inport_key, md);

  xdp_nodes.call(pkt, CONTROLLER_MODULE_INDEX);
  //pcn_log(ctx, LOG_ERROR, md->module_index, 0, "to controller miss");
  return XDP_DROP;
}

static __always_inline
int pcn_pkt_controller_with_metadata(struct CTXTYPE *pkt, struct pkt_metadata *md,
                                     u16 reason, u32 metadata[3]) {
  md->md[0] = metadata[0];
  md->md[1] = metadata[1];
  md->md[2] = metadata[2];

  return pcn_pkt_controller(pkt, md, reason);
}
)";

const std::string CubeXDP::XDP_HELPERS = R"(
#include <linux/errno.h>  // EIVNVAL
#include <linux/string.h> // memmove
#include <uapi/linux/if_ether.h>  // struct ethhdr
#include <uapi/linux/ip.h> // struct iphdr

#define CSUM_MANGLED_0 ((__force __sum16)0xffff)

// for some reason vlan_hdr is not defined in uapi/linux/if_vlan.h, it is
// a short structure hence define it here
struct vlan_hdr {
	__be16	h_vlan_TCI;
	__be16	h_vlan_encapsulated_proto;
};

static __always_inline
bool pcn_is_vlan_present(struct xdp_md *pkt) {
  void* data_end = (void*)(long)pkt->data_end;
  void* data = (void*)(long)pkt->data;

  struct ethhdr *eth = data;
  uint64_t nh_off = 0;

  uint16_t h_proto;
  nh_off = sizeof(*eth);

  if (data + nh_off  > data_end)
    return false;

  h_proto = eth->h_proto;
  if (h_proto == htons(ETH_P_8021Q) || h_proto == htons(ETH_P_8021AD)) {
    return true;
  }
  return false;
}

static __always_inline
int pcn_get_vlan_id(struct xdp_md *pkt, uint16_t *vlan_id, uint16_t *eth_proto) {
  void* data_end = (void*)(long)pkt->data_end;
  void* data = (void*)(long)pkt->data;

  struct ethhdr *eth = data;
  uint64_t nh_off = 0;
  uint16_t h_proto;

  nh_off = sizeof(*eth);
  if (data + nh_off  > data_end)
    return -1;

  h_proto = eth->h_proto;
  if (h_proto == htons(ETH_P_8021Q) || h_proto == htons(ETH_P_8021AD)) {
    struct vlan_hdr *vhdr;
    vhdr = data + nh_off;

    nh_off += sizeof(struct vlan_hdr);

    if (data + nh_off > data_end)
      return -1;

    *vlan_id = ntohs(vhdr->h_vlan_TCI) & 0x0fff;
    h_proto = vhdr->h_vlan_encapsulated_proto;
    *eth_proto = bpf_ntohs(h_proto);
    //bpf_trace_printk("1-Found VLAN TAG: %d and eth_proto: 0x%x\n", *vlan_id, ntohs(h_proto));
  }

  //TODO: Handle double tagged packets

  return 0;
}

static __always_inline
uint8_t pcn_pop_vlan_tag(struct xdp_md *pkt) {
  void* data_end = (void*)(long)pkt->data_end;
  void* data = (void*)(long)pkt->data;

  //bpf_trace_printk("POP Vlan Tag\n");

  struct ethhdr *old_eth = data;
  struct ethhdr *new_eth;
  uint64_t nh_off = 0;
  uint16_t h_proto;

  nh_off = sizeof(*old_eth);

  if (data + nh_off  > data_end)
    return false;

  h_proto = old_eth->h_proto;
  if (h_proto == htons(ETH_P_8021Q) || h_proto == htons(ETH_P_8021AD)) {

    struct vlan_hdr *vhdr;
    __be16 vlan_proto;

    vhdr = data + nh_off;

    nh_off += sizeof(struct vlan_hdr);

    if (data + nh_off > data_end)
      return 1; //ERROR

    vlan_proto = vhdr->h_vlan_encapsulated_proto;

    new_eth = data + sizeof(struct vlan_hdr);
    memmove(new_eth, old_eth, sizeof(struct ethhdr));

    if (bpf_xdp_adjust_head(pkt, (int)sizeof(struct vlan_hdr)))
      return 1; //ERROR

    data = (void *)(long)pkt->data;
    data_end = (void *)(long)pkt->data_end;

    new_eth = data;
    nh_off = sizeof(*new_eth);
    if (data + nh_off > data_end)
      return 1; //ERROR

    new_eth->h_proto = vlan_proto;

    return 0; //OK
  } else {
    //TODO: Return error or is everything ok?
    return 0; //OK
  }

  /* TODO: Should I handle double VLAN tagged packet */
  return 1; //ERROR
}

static __always_inline
uint8_t pcn_push_vlan_tag(struct xdp_md *pkt, u16 eth_proto, u32 vlan_id) {
  void* data = (void*)(long)pkt->data;
  void* data_end = (void*)(long)pkt->data_end;

  //bpf_trace_printk("PUSH Vlan Tag: %d and eth_proto: 0x%x\n", vlan_id, ntohs(eth_proto));

  struct ethhdr *old_eth = data;
  struct ethhdr *new_eth;
  struct vlan_hdr *vhdr;
  uint64_t nh_off = 0;
  __be16 old_eth_proto = old_eth->h_proto;

  nh_off = sizeof(*old_eth);

  if (data + nh_off  > data_end)
    return 1;

  __be16 vlan_proto = htons(eth_proto);
  if(vlan_proto != htons(ETH_P_8021Q) && vlan_proto != htons(ETH_P_8021AD)) {
    vlan_proto = htons(ETH_P_8021Q);
  }

  if (bpf_xdp_adjust_head(pkt, 0 - (int)sizeof(struct vlan_hdr)))
    return 1; //ERROR

  data = (void *)(long)pkt->data;

  data_end = (void *)(long)pkt->data_end;

  new_eth = data;
  nh_off = sizeof(*new_eth);
  if (data + nh_off  > data_end)
    return 1; //ERROR

  vhdr = data + sizeof(*new_eth);
  nh_off += sizeof(*vhdr);
  if (data + nh_off  > data_end)
    return 1; //ERROR

  old_eth = data + sizeof(*vhdr);
  nh_off += sizeof(*old_eth);
  if (data + nh_off  > data_end)
    return 1; //ERROR

  memmove(new_eth, old_eth, sizeof(*new_eth));
  new_eth->h_proto = vlan_proto; //set eth proto to ETH_P_8021Q or ETH_P_8021AD

  vhdr = data + sizeof(*new_eth);
  vhdr->h_vlan_TCI = htons((u16)vlan_id);
  vhdr->h_vlan_encapsulated_proto = old_eth_proto;

  return 0;
}

static __always_inline
void pcn_update_csum(struct iphdr *iph) {
  u16 *next_iph_u16;
  u32 csum = 0;
  int i;

  next_iph_u16 = (u16 *)iph;

#pragma clang loop unroll(full)
  for (i = 0; i < sizeof(*iph) >> 1; i++)
    csum += *next_iph_u16++;

  iph->check = ~((csum & 0xffff) + (csum >> 16));
}

/* checksum related stuff */
static __always_inline __sum16 pcn_csum_fold(__wsum csum) {
  u32 sum = (__force u32)csum;
  sum = (sum & 0xffff) + (sum >> 16);
  sum = (sum & 0xffff) + (sum >> 16);
  return (__force __sum16)~sum;
}

static __always_inline __wsum pcn_csum_add(__wsum csum, __wsum addend) {
  u32 res = (__force u32)csum;
  res += (__force u32)addend;
  return (__force __wsum)(res + (res < (__force u32)addend));
}

static __always_inline __sum16 pcn_csum16_add(__sum16 csum, __be16 addend) {
  u16 res = (__force u16)csum;

  res += (__force u16)addend;
  return (__force __sum16)(res + (res < (__force u16)addend));
}

static __always_inline __wsum pcn_csum_unfold(__sum16 n) {
  return (__force __wsum)n;
}

static __always_inline void pcn_csum_replace_by_diff(__sum16 *sum, __wsum diff) {
  *sum = pcn_csum_fold(pcn_csum_add(diff, ~pcn_csum_unfold(*sum)));
}

static __always_inline
int pcn_l3_csum_replace(struct CTXTYPE *ctx, u32 csum_offset,
                        u32 old_value, u32 new_value, u32 flags) {
  __sum16 *ptr;

  if (unlikely(flags & ~(BPF_F_HDR_FIELD_MASK)))
    return -EINVAL;

  if (unlikely(csum_offset > 0xffff || csum_offset & 1))
    return -EFAULT;

  void *data2 = (void*)(long)ctx->data;
  void *data_end2 = (void*)(long)ctx->data_end;
  if (data2 + csum_offset + sizeof(*ptr) > data_end2) {
    return -EINVAL;
  }

  ptr = (__sum16 *)((void*)(long)ctx->data + csum_offset);

  switch (flags & BPF_F_HDR_FIELD_MASK	) {
  case 0:
    pcn_csum_replace_by_diff(ptr, new_value);
    break;
  case 2:
    *ptr = ~pcn_csum16_add(pcn_csum16_add(~(*ptr), ~old_value), new_value);
    break;
  case 4:
    pcn_csum_replace_by_diff(ptr, pcn_csum_add(new_value, ~old_value));
    break;
  default:
    return -EINVAL;
  }
  return 0;
}

static __always_inline
int pcn_l4_csum_replace(struct CTXTYPE *ctx, u32 csum_offset,
                           u32 old_value, u32 new_value, u32 flags) {
  bool is_pseudo = flags & BPF_F_PSEUDO_HDR;
	bool is_mmzero = flags & BPF_F_MARK_MANGLED_0;
  bool do_mforce = flags & BPF_F_MARK_ENFORCE;
  __sum16 *ptr;

  if (unlikely(flags & ~(BPF_F_MARK_MANGLED_0 | BPF_F_MARK_ENFORCE |
                         BPF_F_PSEUDO_HDR | BPF_F_HDR_FIELD_MASK)))
    return -EINVAL;

  if (unlikely(csum_offset > 0xffff || csum_offset & 1))
    return -EFAULT;

  void *data2 = (void*)(long)ctx->data;
  void *data_end2 = (void*)(long)ctx->data_end;
  if (data2 + csum_offset + sizeof(*ptr) > data_end2) {
    return -EINVAL;
  }

  ptr = (__sum16 *)((void*)(long)ctx->data + csum_offset);

  if (is_mmzero && !do_mforce && !*ptr)
    return 0;

  switch (flags & BPF_F_HDR_FIELD_MASK) {
  case 0:
    pcn_csum_replace_by_diff(ptr, new_value);
    break;
  case 2:
    *ptr = ~pcn_csum16_add(pcn_csum16_add(~(*ptr), ~old_value), new_value);
    break;
  case 4:
    pcn_csum_replace_by_diff(ptr, pcn_csum_add(new_value, ~old_value));
    break;
  default:
    return -EINVAL;
  }

  // It may happen that the checksum of UDP packets is 0;
  // in that case there is an ambiguity because 0 could be
  // considered as a packet without checksum, in that case
  // the checksum has to be "mangled" (i.e., write 0xffff instead of 0).
  if (is_mmzero && !*ptr)
    *ptr = CSUM_MANGLED_0;

  return 0;
}

static __always_inline
__wsum pcn_csum_diff(__be32 *from, u32 from_size, __be32 *to,
                     u32 to_size, __wsum seed) {
// FIXME: sometimes the LINUX_VERSION_CODE is not aligned with the kernel.
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,16,0)
  return bpf_csum_diff(from, from_size, to, to_size, seed);
#else
  if (from_size != 4 || to_size != 4)
    return -EINVAL;

  __wsum result = pcn_csum_add(*to, ~*from);
  result += seed;
	if (seed > result)
		result += 1;

  return result;
#endif
}

/* end checksum related stuff */

)";

}  // namespace polycube
}  // namespace polycubed
