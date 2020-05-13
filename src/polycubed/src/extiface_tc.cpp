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
#include "bcc_mutex.h"
#include "exceptions.h"
#include "patchpanel.h"
#include "utils/netlink.h"

#include <iostream>

namespace polycube {
namespace polycubed {

ExtIfaceTC::ExtIfaceTC(const std::string &iface) : ExtIface(iface) {}

ExtIfaceTC::~ExtIfaceTC() {
  if (ingress_program_) {
    Netlink::getInstance().detach_from_tc(iface_);
  }
}

int ExtIfaceTC::load_ingress() {
  if (ingress_next_ == 0xffff) {
    // No next program, remove the rx program
    if (ingress_program_) {
      Netlink::getInstance().detach_from_tc(iface_, ATTACH_MODE::INGRESS);
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
  res = program->load_func("handler", BPF_PROG_TYPE_SCHED_CLS, fd);
  if (res.code() != 0) {
    logger->error("Error loading ingress program: {}", res.msg());
    throw BPFError("failed to load ebpf program: " + res.msg());
  }

  Netlink::getInstance().attach_to_tc(iface_, fd, ATTACH_MODE::INGRESS);

  ingress_program_ = std::move(program);

  return fd;
}

std::string ExtIfaceTC::get_ingress_code() const {
  return RX_CODE;
}

std::string ExtIfaceTC::get_egress_code() const {
  return RX_CODE;
}

bpf_prog_type ExtIfaceTC::get_program_type() const {
  return BPF_PROG_TYPE_SCHED_CLS;
}

const std::string ExtIfaceTC::RX_CODE = R"(
#include <uapi/linux/pkt_cls.h>

BPF_TABLE("extern", int, int, nodes, _POLYCUBE_MAX_NODES);

int handler(struct __sk_buff *skb) {
  skb->cb[0] = NEXT_PORT << 16;
  nodes.call(skb, NEXT_PROGRAM);

  return TC_ACT_SHOT;
}
)";

}  // namespace polycubed
}  // namespace polycube
