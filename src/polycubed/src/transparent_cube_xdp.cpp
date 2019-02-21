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

#include "transparent_cube_xdp.h"
#include "cube_tc.h"
#include "cube_xdp.h"
#include "datapath_log.h"
#include "exceptions.h"
#include "patchpanel.h"
#include "transparent_cube_tc.h"

#include <iostream>

namespace polycube {
namespace polycubed {

TransparentCubeXDP::TransparentCubeXDP(
    const std::string &name, const std::string &service_name,
    const std::vector<std::string> &ingress_code,
    const std::vector<std::string> &egress_code, LogLevel level, CubeType type,
    const service::attach_cb &attach)
    : TransparentCube(name, service_name, PatchPanel::get_xdp_instance(),
                      PatchPanel::get_tc_instance(), level, type, attach) {
  TransparentCube::init(ingress_code, egress_code);
}

TransparentCubeXDP::~TransparentCubeXDP() {
  // it cannot be done in Cube::~Cube() because calls a virtual method
  TransparentCube::uninit();
}

std::string TransparentCubeXDP::get_wrapper_code() {
  return TransparentCube::get_wrapper_code() + CubeXDP::CUBEXDP_COMMON_WRAPPER +
         TRANSPARENTCUBEXDP_WRAPPER;
}

void TransparentCubeXDP::compile(ebpf::BPF &bpf, const std::string &code,
                                 int index, ProgramType type) {
  uint16_t next;
  switch (type) {
  case ProgramType::INGRESS:
    next = ingress_next_;
    compileIngress(bpf, code, next);
    break;
  case ProgramType::EGRESS:
    next = egress_next_;
    compileEgress(bpf, code, next);
    break;
  }
}

void TransparentCubeXDP::compileIngress(ebpf::BPF &bpf, const std::string &code,
                                        uint16_t next) {
  std::string all_code(get_wrapper_code() +
                       DatapathLog::get_instance().parse_log(code));

  std::vector<std::string> cflags_(cflags);
  cflags_.push_back(std::string("-DMOD_NAME=") + std::string(name_));
  cflags_.push_back("-DCUBE_ID=" + std::to_string(get_id()));
  cflags_.push_back("-DLOG_LEVEL=LOG_" + logLevelString(level_));
  cflags_.push_back(std::string("-DRETURNCODE=") + std::string("XDP_DROP"));
  cflags_.push_back(std::string("-DPOLYCUBE_XDP=1"));
  cflags_.push_back(std::string("-DCTXTYPE=") + std::string("xdp_md"));
  cflags_.push_back(std::string("-DNEXT=" + std::to_string(next)));

  std::lock_guard<std::mutex> guard(bcc_mutex);
  auto init_res = bpf.init(all_code, cflags_);

  if (init_res.code() != 0) {
    logger->error("failed to init XDP program: {0}", init_res.msg());
    throw BPFError("failed to init XDP program: " + init_res.msg());
  }
  // logger->debug("XDP program compileed");
}

void TransparentCubeXDP::compileEgress(ebpf::BPF &bpf, const std::string &code,
                                       uint16_t next) {
  TransparentCubeTC::do_compile(get_id(), next, ProgramType::EGRESS, level_,
                                bpf, code, 0);
}

int TransparentCubeXDP::load(ebpf::BPF &bpf, ProgramType type) {
  switch (type) {
  case ProgramType::INGRESS:
    return CubeXDP::do_load(bpf);
  case ProgramType::EGRESS:
    return CubeTC::do_load(bpf);
  default:
    throw std::runtime_error("Bad program type");
  }
}

void TransparentCubeXDP::unload(ebpf::BPF &bpf, ProgramType type) {
  switch (type) {
  case ProgramType::INGRESS:
    return CubeXDP::do_unload(bpf);
  case ProgramType::EGRESS:
    return CubeTC::do_unload(bpf);
  default:
    throw std::runtime_error("Bad program type");
  }
}

const std::string TransparentCubeXDP::TRANSPARENTCUBEXDP_WRAPPER = R"(
//BPF_TABLE("extern", u32, struct pkt_metadata, port_md, 1);
//BPF_TABLE("extern", int, int, xdp_nodes, _POLYCUBE_MAX_NODES);

int handle_rx_xdp_wrapper(struct CTXTYPE *ctx) {
  u32 inport_key = 0;
  struct pkt_metadata *int_md;
  struct pkt_metadata md = {};

  int_md = port_md.lookup(&inport_key);
  if (int_md) {
    md.cube_id = CUBE_ID;
    md.packet_len = int_md->packet_len;

    int rc = handle_rx(ctx, &md);

    switch (rc) {
    case RX_DROP:
      return XDP_DROP;
    case RX_OK:
#if NEXT == 0xffff
    return XDP_PASS;
#else
    xdp_nodes.call(ctx, NEXT);
    return XDP_DROP;
#endif
    }
  }

  return XDP_DROP;
}
)";

}  // namespace polycubed
}  // namespace polycube
