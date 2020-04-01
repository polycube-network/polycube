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
#include "bcc_mutex.h"
#include "exceptions.h"
#include "extiface_tc.h"
#include "patchpanel.h"
#include "utils/netlink.h"
#include "transparent_cube_xdp.h"

#include <iostream>

namespace polycube {
namespace polycubed {

ExtIfaceXDP::ExtIfaceXDP(const std::string &iface, int attach_flags)
    : ExtIface(iface), attach_flags_(attach_flags) {
  try {
    std::unique_lock<std::mutex> bcc_guard(bcc_mutex);
    int fd_rx = load_ingress();
    int fd_tx = load_tx();
    load_egress();

    index_ = PatchPanel::get_xdp_instance().add(fd_tx);
    redir_index_ = index_;
    Netlink::getInstance().attach_to_xdp(iface_, fd_rx, attach_flags_);
  } catch (...) {
    used_ifaces.erase(iface);
    throw;
  }
}

ExtIfaceXDP::~ExtIfaceXDP() {
  PatchPanel::get_xdp_instance().remove(index_);
  Netlink::getInstance().detach_from_xdp(iface_, attach_flags_);
}

void ExtIfaceXDP::update_indexes() {
  uint16_t next;

  // Link cubes of ingress chain
  // peer/stack <- cubes[N-1] <- ... <- cubes[0] <- iface
  
  next = peer_ ? peer_->get_index() : 0xffff;
  for (int i = cubes_.size()-1; i >= 0; i--) {
    if (cubes_[i]->get_index(ProgramType::INGRESS)) {
      cubes_[i]->set_next(next, ProgramType::INGRESS);
      next = cubes_[i]->get_index(ProgramType::INGRESS);
    }
  }

  set_next(next, ProgramType::INGRESS);

  // Link cubes of egress chain
  // iface <- cubes[0] <- ... <- cubes[N-1] <- (peer_egress) <- peer/stack

  bool first = true;
  next = redir_index_;
  for (int i = 0; i < cubes_.size(); i++) {
    if (cubes_[i]->get_index(ProgramType::EGRESS)) {
      if (first) {
        // The last TC egress program (first in the list) must pass the packet
        // (it is executed in the TC_EGRESS hook) (lower 16 bits)
        // The last XDP egress program (first in the list) must use the XDP
        // redir program (higer 16 bits)
        cubes_[i]->set_next(redir_index_ << 16 | 0xffff, ProgramType::EGRESS);
        first = false;

      } else {
        cubes_[i]->set_next(next, ProgramType::EGRESS);
      }

      next = cubes_[i]->get_index(ProgramType::EGRESS);
    }
  }

  // If the peer cube has an egress program add it at the beginning of the chain
  Port *peer_port = dynamic_cast<Port *>(peer_);
  if (peer_port && peer_port->get_egress_index()) {
    if (first) {
      // If there are not transparent egress programs set two different next
      // indexes for the TC and the XDP version of the egress program of the
      // peer cube:
      // TC: pass the packet (0xffff), lower 16 bits
      // XDP: index of the XDP redir program, higher 16 bits
      peer_port->set_parent_egress_next(redir_index_ << 16 | 0xffff);
      first = false;
    
    } else {
      peer_port->set_parent_egress_next(next);
    }

    next = peer_port->get_egress_index();
  }

  if (first) {
    // There aren't egress cubes
    // The TC egress program must pass the packet
    set_next(0xffff, ProgramType::EGRESS);
  
  } else {
    set_next(next, ProgramType::EGRESS);
  }

  index_ = next;
  if (peer_) {
    peer_->set_next_index(next);
  }
}

std::string ExtIfaceXDP::get_ingress_code() const {
  return XDP_PROG_CODE;
}

std::string ExtIfaceXDP::get_egress_code() const {
  return ExtIfaceTC::RX_CODE;
}

std::string ExtIfaceXDP::get_tx_code() const {
  return XDP_REDIR_PROG_CODE;
}

bpf_prog_type ExtIfaceXDP::get_program_type() const {
  return BPF_PROG_TYPE_XDP;
}

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
  u32 traffic_class;
  // used to send data to controller
  u16 reason;
  u32 md[3];
} __attribute__((packed));

BPF_TABLE("extern", int, int, xdp_nodes, _POLYCUBE_MAX_NODES);
BPF_TABLE("extern", u32, struct pkt_metadata, port_md, 1);
BPF_TABLE("array", int, u32, index_map, 1);

int handler(struct xdp_md *ctx) {
    void* data_end = (void*)(long)ctx->data_end;
    void* data = (void*)(long)ctx->data;
    u32 key = 0;

    struct ethhdr *eth = data;
    if (data + sizeof(*eth)  > data_end)
        return XDP_DROP;

    struct pkt_metadata md = {};

    u32 *index = index_map.lookup(&key);
    if (!index)
      return XDP_DROP;

    md.in_port = *index >> 16;
    md.module_index = *index & 0xffff;
    md.packet_len = (u32)(data_end - data);

    port_md.update(&key, &md);

    //bpf_trace_printk("Executing tail call for port %d\n", md.in_port);
    xdp_nodes.call(ctx, md.module_index );
    //bpf_trace_printk("Tail call not executed for port %d\n", md.in_port);

    return XDP_DROP;
}
)";

const std::string ExtIfaceXDP::XDP_REDIR_PROG_CODE = R"(
int handler(struct xdp_md *ctx) {
  //bpf_trace_printk("Redirect to ifindex %d\n", INTERFACE_INDEX);
  return bpf_redirect(INTERFACE_INDEX, 0);
}
)";

}  // namespace polycubed
}  // namespace polycube
