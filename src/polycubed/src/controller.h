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

#pragma once

#include "cube_tc.h"

#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <thread>
#include <vector>

#include <api/BPF.h>

#include <spdlog/spdlog.h>

#include "viface/viface.hpp"

#include "polycube/services/cube_factory.h"

using polycube::service::PacketIn;
using polycube::service::packet_in_cb;
using polycube::service::log_msg_cb;
using polycube::service::LogMsg;

namespace polycube {
namespace polycubed {


// struct used when sending a packet to an cube
struct __attribute__((__packed__)) metadata {
  uint16_t module_index;
  uint16_t port_id;

#define MD_PKT_FROM_CONTROLLER  (1UL << 0)
#define MD_EGRESS_CONTEXT       (1UL << 1)
  uint32_t flags;
};

/*
 * controller represents a node that is used to send packets to the slow-path
 * Internally it is composed of two eBPF programs, one to manageme the
 * packet-ins (packets going to the slow-path) and the other one managing the
 * packet-outs (packets coming from the slow-path).
 */
class Controller {
 public:
  static Controller &get_tc_instance();
  static Controller &get_xdp_instance();
  ~Controller();

  void register_cb(int id, const packet_in_cb &cb);
  void unregister_cb(int id);
  void send_packet_to_cube(uint16_t module_index, uint16_t port_id,
                           const std::vector<uint8_t> &packet,
                           service::Direction direction=service::Direction::INGRESS,
                           bool mac_overwrite=false);

  int get_fd() const;
  uint32_t get_id() const;

  static void call_back_proxy(void *cb_cookie, void *data, int data_size);

 private:
  Controller(const std::string &tx_code, const std::string &rx_code,
             enum bpf_prog_type type);
  log_msg_cb handle_log_msg;
  void log_msg(const LogMsg *msg);

  static const int MD_MAP_SIZE = 1024;

  uint32_t id_;

  void start();
  void stop();

  std::unique_ptr<ebpf::BPFArrayTable<metadata>> metadata_table_;

  bool stop_;

  ebpf::BPF tx_module_;
  ebpf::BPF rx_module_;
  int fd_tx_;
  int fd_rx_;

  std::unique_ptr<viface::VIface> iface_;
  int ctrl_rx_md_index_;  // next position to write in the metadata map

  std::unique_ptr<std::thread> pkt_in_thread_;

  static std::map<int, const packet_in_cb &> cbs_;
  static std::mutex cbs_mutex_;  // protects the cbs_ container

  std::shared_ptr<spdlog::logger> logger;
};

}  // namespace polycubed
}  // namespace polycube
