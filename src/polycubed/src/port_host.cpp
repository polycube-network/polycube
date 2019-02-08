/*
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

#include "port_host.h"
#include "exceptions.h"
#include "patchpanel.h"

#include <iostream>

#include <net/if.h>
#include <sys/ioctl.h>

namespace polycube {
namespace polycubed {

const std::string PortHost::TX_CODE = R"(
#include <uapi/linux/pkt_cls.h>

int egress(struct __sk_buff *skb) {
#ifdef POLYCUBE_XDP
    return XDP_PASS;
#else
    return TC_ACT_OK;
#endif
}
)";

PortHost::PortHost(PortType type)
    : type_(type), logger(spdlog::get("polycubed")) {
  try {
    // load_rx();
    load_tx();
    switch (type_) {
    case PortType::TC:
      PatchPanel::get_tc_instance().add(*this);
      break;

    case PortType::XDP:
      PatchPanel::get_xdp_instance().add(*this);
      break;
    }

  } catch (...) {
    throw;
  }
}

PortHost::~PortHost() {
  switch (type_) {
  case PortType::TC:
    PatchPanel::get_tc_instance().remove(*this);
    break;

  case PortType::XDP:
    PatchPanel::get_xdp_instance().remove(*this);
    break;
  }

  tx_.unload_func("egress");
}

void PortHost::load_tx() {
  std::vector<std::string> flags;
  enum bpf_prog_type prog_type;

  switch (type_) {
  case PortType::TC:
    prog_type = BPF_PROG_TYPE_SCHED_CLS;
    break;

  case PortType::XDP:
    flags.push_back(std::string("-DPOLYCUBE_XDP"));
    prog_type = BPF_PROG_TYPE_XDP;
    break;
  }

  auto init_res = tx_.init(TX_CODE, flags);
  if (init_res.code() != 0) {
    std::cerr << init_res.msg() << std::endl;
    throw BPFError("failed to init ebpf program:" + init_res.msg());
  }

  // load tx_ program
  auto load_res = tx_.load_func("egress", prog_type, fd_);
  if (load_res.code() != 0) {
    std::cerr << load_res.msg() << std::endl;
    throw BPFError("failed to load ebpf program: " + load_res.msg());
  }
}

int PortHost::get_fd() const {
  return fd_;
}

}  // namespace polycubed
}  // namespace polycube
