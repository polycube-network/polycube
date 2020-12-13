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
    : TransparentCube(name, service_name, PatchPanel::get_xdp_instance(), level,
                      type, attach),
      egress_next_tc_(0),
      egress_next_tc_is_netdev_(false) {
  egress_programs_table_tc_ = std::move(egress_programs_table_);
  auto table = master_program_->get_prog_table("egress_programs_xdp");
  egress_programs_table_ =
      std::unique_ptr<ebpf::BPFProgTable>(new ebpf::BPFProgTable(table));

  TransparentCube::init(ingress_code, egress_code);
}

TransparentCubeXDP::~TransparentCubeXDP() {
  // it cannot be done in Cube::~Cube() because calls a virtual method
  TransparentCube::uninit();
}

void TransparentCubeXDP::set_next(uint16_t next, ProgramType type,
                                  bool is_netdev) {
  if (type == ProgramType::EGRESS) {
    egress_next_tc_ = next;
    egress_next_tc_is_netdev_ = is_netdev;
  }

  TransparentCube::set_next(next, type, is_netdev);
}

void TransparentCubeXDP::set_egress_next(uint16_t next_xdp,
                                         bool next_xdp_is_netdev,
                                         uint16_t next_tc,
                                         bool next_tc_is_netdev) {
  egress_next_ = next_xdp;
  egress_next_is_netdev_ = next_xdp_is_netdev;
  egress_next_tc_ = next_tc;
  egress_next_tc_is_netdev_ = next_tc_is_netdev;
  
  reload_all();
}

std::string TransparentCubeXDP::get_wrapper_code() {
  return TransparentCube::get_wrapper_code() + CubeXDP::CUBEXDP_COMMON_WRAPPER +
         TRANSPARENTCUBEXDP_WRAPPER + CubeXDP::CUBEXDP_HELPERS;
}

void TransparentCubeXDP::reload(const std::string &code, int index,
                                ProgramType type) {
  std::lock_guard<std::mutex> cube_guard(cube_mutex_);

  switch (type) {
    case ProgramType::INGRESS:
      return do_reload(code, index, type, ingress_programs_, ingress_code_,
                       ingress_programs_table_, ingress_index_);
      break;

    case ProgramType::EGRESS:
      // Reload the XDP program
      do_reload(code, index, type, egress_programs_, egress_code_,
                egress_programs_table_, egress_index_);

      // Also reload the TC program
      {
        std::unique_lock<std::mutex> bcc_guard(bcc_mutex);
        std::unique_ptr<ebpf::BPF> new_bpf_program = std::unique_ptr<ebpf::BPF>(
            new ebpf::BPF(0, nullptr, false, name_, false,
                          egress_programs_.at(index).get()));

        bcc_guard.unlock();
        TransparentCubeTC::do_compile(get_id(), egress_next_tc_,
                                      egress_next_tc_is_netdev_,
                                      ProgramType::EGRESS, level_,
                                      *new_bpf_program, code, 0);
        int fd = CubeTC::do_load(*new_bpf_program);

        egress_programs_table_tc_->update_value(index, fd);

        if (index == 0) {
          PatchPanel::get_tc_instance().update(egress_index_, fd);
        }

        CubeTC::do_unload(*egress_programs_tc_.at(index));
        bcc_guard.lock();
        egress_programs_tc_[index] = std::move(new_bpf_program);
      }

      break;

    default:
      throw std::runtime_error("Bad program type");
  }
}

void TransparentCubeXDP::reload_all() {
  std::lock_guard<std::mutex> cube_guard(cube_mutex_);
  for (int i = 0; i < ingress_programs_.size(); i++) {
    if (ingress_programs_[i]) {
      do_reload(ingress_code_[i], i, ProgramType::INGRESS, ingress_programs_,
                ingress_code_, ingress_programs_table_, ingress_index_);
    }
  }

  for (int i = 0; i < egress_programs_.size(); i++) {
    if (egress_programs_[i]) {
      // Reload the XDP program
      do_reload(egress_code_[i], i, ProgramType::EGRESS, egress_programs_,
                egress_code_, egress_programs_table_, egress_index_);

      // Also reload the TC program
      {
        std::unique_lock<std::mutex> bcc_guard(bcc_mutex);
        std::unique_ptr<ebpf::BPF> new_bpf_program = std::unique_ptr<ebpf::BPF>(
            new ebpf::BPF(0, nullptr, false, name_, false,
                          egress_programs_tc_.at(i).get()));

        bcc_guard.unlock();
        TransparentCubeTC::do_compile(get_id(), egress_next_tc_,
                                      egress_next_tc_is_netdev_,
                                      ProgramType::EGRESS, level_,
                                      *new_bpf_program, egress_code_[i], 0);
        int fd = CubeTC::do_load(*new_bpf_program);

        egress_programs_table_tc_->update_value(i, fd);

        if (i == 0) {
          PatchPanel::get_tc_instance().update(egress_index_, fd);
        }

        CubeTC::do_unload(*egress_programs_tc_.at(i));
        bcc_guard.lock();
        egress_programs_tc_[i] = std::move(new_bpf_program);
      }
    }
  }
}

int TransparentCubeXDP::add_program(const std::string &code, int index, ProgramType type) {
  std::lock_guard<std::mutex> cube_guard(cube_mutex_);

  switch (type) {
    case ProgramType::INGRESS:
      return do_add_program(code, index, type, ingress_programs_, ingress_code_,
                            ingress_programs_table_, &ingress_index_);

    case ProgramType::EGRESS:
      // Add XDP program
      index = do_add_program(code, index, type,  egress_programs_, egress_code_,
                             egress_programs_table_, &egress_index_);

      // Also add the TC program
      {
        std::unique_lock<std::mutex> bcc_guard(bcc_mutex);

        // Create a bpf program using the same maps as the corresponding XDP one
        egress_programs_tc_[index] =
            std::unique_ptr<ebpf::BPF>(
                  new ebpf::BPF(0, nullptr, false, name_, false,
                                egress_programs_.at(index).get()));

        bcc_guard.unlock();
        TransparentCubeTC::do_compile(get_id(), egress_next_tc_,
                                      egress_next_tc_is_netdev_,
                                      ProgramType::EGRESS, level_,
                                      *egress_programs_tc_.at(index), code, 0);
        int fd = CubeTC::do_load(*egress_programs_tc_.at(index));
        bcc_guard.lock();

        egress_programs_table_tc_->update_value(index, fd);
        if (index == 0) {
          PatchPanel::get_tc_instance().add(fd, egress_index_);
        }
      }

      return index;

    default:
      throw std::runtime_error("Bad program type");
  }
}

void TransparentCubeXDP::del_program(int index, ProgramType type) {
  std::lock_guard<std::mutex> cube_guard(cube_mutex_);

  switch (type) {
    case ProgramType::INGRESS:
      do_del_program(index, type, ingress_programs_, ingress_code_,
                     ingress_programs_table_);
      break;

    case ProgramType::EGRESS:
      // Delete XDP program
      do_del_program(index, type, egress_programs_, egress_code_,
                     egress_programs_table_);

      // Also delete TC program
      {
        egress_programs_table_tc_->remove_value(index);
        CubeTC::do_unload(*egress_programs_tc_.at(index));
        std::unique_lock<std::mutex> bcc_guard(bcc_mutex);
        egress_programs_tc_.at(index).reset();
        bcc_guard.unlock();
      }

      break;

    default:
      throw std::runtime_error("Bad program type");
  }
}

void TransparentCubeXDP::compile(ebpf::BPF &bpf, const std::string &code,
                                 int index, ProgramType type) {
  uint16_t next;
  bool is_netdev;
  switch (type) {
  case ProgramType::INGRESS:
    next = ingress_next_;
    is_netdev = false;
    break;
  case ProgramType::EGRESS:
    next = egress_next_;
    is_netdev = egress_next_is_netdev_;
    break;
  }

  std::string all_code(get_wrapper_code() +
                       DatapathLog::get_instance().parse_log(code));

  std::vector<std::string> cflags(cflags_);
  cflags.push_back(std::string("-DMOD_NAME=") + std::string(name_));
  cflags.push_back("-DCUBE_ID=" + std::to_string(get_id()));
  cflags.push_back("-DLOG_LEVEL=LOG_" + logLevelString(level_));
  cflags.push_back(std::string("-DRETURNCODE=") + std::string("XDP_DROP"));
  cflags.push_back(std::string("-DPOLYCUBE_XDP=1"));
  cflags.push_back(std::string("-DCTXTYPE=") + std::string("xdp_md"));
  cflags.push_back(std::string("-DNEXT=" + std::to_string(next)));
  cflags.push_back(std::string("-DNEXT_IS_NETDEV=") +
                   std::to_string(int(is_netdev)));
  cflags.push_back(std::string("-DPOLYCUBE_PROGRAM_TYPE=") +
                   std::to_string(static_cast<int>(type)));

  std::lock_guard<std::mutex> guard(bcc_mutex);
  auto init_res = bpf.init(all_code, cflags);

  if (init_res.code() != 0) {
    logger->error("failed to init XDP program: {0}", init_res.msg());
    throw BPFError("failed to init XDP program: " + init_res.msg());
  }
}

int TransparentCubeXDP::load(ebpf::BPF &bpf, ProgramType type) {
  return CubeXDP::do_load(bpf);
}

void TransparentCubeXDP::unload(ebpf::BPF &bpf, ProgramType type) {
  return CubeXDP::do_unload(bpf);
}

const std::string TransparentCubeXDP::TRANSPARENTCUBEXDP_WRAPPER = R"(
//BPF_TABLE("extern", u32, struct pkt_metadata, port_md, 1);
//BPF_TABLE("extern", int, int, xdp_nodes, _POLYCUBE_MAX_NODES);

int handle_rx_xdp_wrapper(struct CTXTYPE *ctx) {
  int zero = 0;

  struct pkt_metadata *md = port_md.lookup(&zero);
  if (!md) {
    return XDP_ABORTED;
  }

  int rc = handle_rx(ctx, md);

  switch (rc) {
    case RX_DROP:
      return XDP_DROP;

    case RX_OK:
#if NEXT_IS_NETDEV
      return bpf_redirect(NEXT, 0);
#elif NEXT == 0xffff
      return XDP_PASS;
#else
      xdp_nodes.call(ctx, NEXT);
      return XDP_ABORTED;
#endif
  }

  return XDP_ABORTED;
}

static __always_inline
int pcn_pkt_controller(struct CTXTYPE *pkt, struct pkt_metadata *md,
                       u16 reason) {
  u16 in_port = md->in_port;

  md->cube_id = CUBE_ID;
  // For transparent cubes in_port is used by the controller to know the
  // direction of the packet
  md->in_port = POLYCUBE_PROGRAM_TYPE;
  md->packet_len = pkt->data_end - pkt->data;
  md->reason = reason;

  return controller_xdp.perf_submit_skb(pkt, md->packet_len, md, sizeof(*md));

  // Restore original port
  md->in_port = in_port;
}
)";

}  // namespace polycubed
}  // namespace polycube
