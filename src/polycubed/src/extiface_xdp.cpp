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
    : ExtIface(iface), attach_flags_(attach_flags) {}

ExtIfaceXDP::~ExtIfaceXDP() {
  if (ingress_program_) {
    Netlink::getInstance().detach_from_xdp(iface_, attach_flags_);
  }
}

int ExtIfaceXDP::load_ingress() {
  if (ingress_next_ == 0xffff) {
    // No next program, remove the rx program
    if (ingress_program_) {
      Netlink::getInstance().detach_from_xdp(iface_, attach_flags_);
      ingress_program_.reset();
    }

    return -1;
  }

  ebpf::StatusTuple res(0);

  std::vector<std::string> flags;

  flags.push_back(std::string("-DMOD_NAME=") + iface_);
  flags.push_back(std::string("-D_POLYCUBE_MAX_NODES=") +
                  std::to_string(PatchPanel::_POLYCUBE_MAX_NODES));
  flags.push_back(std::string("-DNEXT_PROGRAM=") +
                  std::to_string(ingress_next_));
  flags.push_back(std::string("-DNEXT_PORT=") + std::to_string(ingress_port_));

  std::unique_ptr<ebpf::BPF> program = std::make_unique<ebpf::BPF>();

  res = program->init(get_ingress_code(), flags);
  if (res.code() != 0) {
    logger->error("Error compiling ingress program: {}", res.msg());
    throw BPFError("failed to init ebpf program:" + res.msg());
  }

  int fd;
  res = program->load_func("handler", BPF_PROG_TYPE_XDP, fd);
  if (res.code() != 0) {
    logger->error("Error loading ingress program: {}", res.msg());
    throw BPFError("failed to load ebpf program: " + res.msg());
  }

  Netlink::getInstance().attach_to_xdp(iface_, fd, attach_flags_);

  ingress_program_ = std::move(program);

  return fd;
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
  next = 0xffff;
  for (int i = 0; i < cubes_.size(); i++) {
    if (cubes_[i]->get_index(ProgramType::EGRESS)) {
      if (first) {
        // The last XDP egress program (first in the list) must redir the packet
        // to the interface
        // The last TC egress program (first in the list) must pass the packet
        // (it is executed in the TC_EGRESS hook)
        auto *cube = dynamic_cast<TransparentCubeXDP *>(cubes_[i]);
        cube->set_egress_next(get_port_id(), true, 0xffff, false);
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
      peer_port->set_parent_egress_next(0xffff);
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

  if (peer_ && index_ != next) {
    peer_->set_next_index(next);
  }

  index_ = next;
}

std::string ExtIfaceXDP::get_ingress_code() {
  return XDP_PROG_CODE;
}

std::string ExtIfaceXDP::get_egress_code() {
  return ExtIfaceTC::RX_CODE;
}

bpf_prog_type ExtIfaceXDP::get_program_type() const {
  return BPF_PROG_TYPE_XDP;
}

const std::string ExtIfaceXDP::XDP_PROG_CODE = R"(
#include <linux/string.h>

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

int handler(struct xdp_md *ctx) {
  int zero = 0;

  struct pkt_metadata *md = port_md.lookup(&zero);
  if (!md) {
    return XDP_ABORTED;
  }

  // Initialize metadata
  md->in_port = NEXT_PORT;
  md->packet_len = ctx->data_end - ctx->data;
  md->traffic_class = 0;
  memset(md->md, 0, sizeof(md->md));

  xdp_nodes.call(ctx, NEXT_PROGRAM);

  return XDP_ABORTED;
}
)";

}  // namespace polycubed
}  // namespace polycube
