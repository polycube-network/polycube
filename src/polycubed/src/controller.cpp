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

#include "exceptions.h"

#include "controller.h"
#include "cube_tc.h"
#include "cube_xdp.h"
#include "datapath_log.h"
#include "netlink.h"
#include "patchpanel.h"

#include "polycube/services/utils.h"

#include <iostream>

using namespace polycube::service;

namespace polycube {
namespace polycubed {

// Sends the packet to the controller
const std::string CTRL_TC_TX = R"(
struct metadata {
  u16 cube_id;
  u16 port_id;
  u32 packet_len;
  u16 reason;
  u32 md[3];  // generic metadata
} __attribute__((packed));

BPF_PERF_OUTPUT(controller);

int controller_module_tx(struct __sk_buff *ctx) {
  pcn_log(ctx, LOG_TRACE, "[tc-encapsulator]: to controller");
  // If the packet is tagged add the tagged in the packet itself, otherwise it
  // will be lost
  if (ctx->vlan_present) {
    volatile __u32 vlan_tci = ctx->vlan_tci;
    volatile __u32 vlan_proto = ctx->vlan_proto;
    bpf_skb_vlan_push(ctx, vlan_proto, vlan_tci);
  }

  volatile u32 x; // volatile to avoid verifier error on kernels < 4.10
  x = ctx->cb[0];
  u16 in_port = x >> 16;
  u16 module_index = x & 0x7fff;
  u16 pass_to_stack = x & 0x8000;

  x = ctx->cb[1];
  u16 reason = x & 0xffff;

  struct metadata md = {0};
  md.cube_id = module_index;
  md.port_id = in_port;
  md.packet_len = ctx->len;
  md.reason = reason;

  x = ctx->cb[2];
  md.md[0] = x;
  x = ctx->cb[3];
  md.md[1] = x;
  x = ctx->cb[4];
  md.md[2] = x;

  int r = controller.perf_submit_skb(ctx, ctx->len, &md, sizeof(md));
  if (r != 0) {
    pcn_log(ctx, LOG_ERR, "[tc-encapsulator]: perf_submit_skb error: %d", r);
  }
  if (pass_to_stack) {
    pcn_log(ctx, LOG_DEBUG, "[tc-encapsulator]: passing to stack");
    return 0;
  }
  return 7;
ERROR:
  pcn_log(ctx, LOG_ERR, "[tc-encapsulator]: unknown error");
  return 7;	//TODO: check code
}
)";

// Receives packet from controller and forwards it to the Cube
const std::string CTRL_TC_RX = R"(
#include <bcc/helpers.h>
#include <bcc/proto.h>
#include <linux/kernel.h>
#include <linux/skbuff.h>

#include <linux/rcupdate.h>

BPF_TABLE("extern", int, int, nodes, _POLYCUBE_MAX_NODES);

struct metadata {
	u16 module_index;
	u16 port_id;
};

BPF_TABLE("array", u32, struct metadata, md_map_rx, MD_MAP_SIZE);
BPF_TABLE("array", u32, u32, index_map_rx, 1);

int controller_module_rx(struct __sk_buff *ctx) {
  pcn_log(ctx, LOG_TRACE, "[tc-decapsulator]: from controller");

	u32 zero = 0;
	u32 *index = index_map_rx.lookup(&zero);
	if (!index) {
    pcn_log(ctx, LOG_ERR, "[tc-decapsulator]: !index");
		return 2;
	}

	rcu_read_lock();

	(*index)++;
	*index %= MD_MAP_SIZE;
	rcu_read_unlock();

	u32 i = *index;

	struct metadata *md = md_map_rx.lookup(&i);
	if (!md) {
		pcn_log(ctx, LOG_ERR, "[tc-decapsulator]: !md");
		return 2;
	}

	u16 in_port = md->port_id;
	u16 module_index = md->module_index;

	ctx->cb[0] = in_port << 16 | module_index;
	nodes.call(ctx, module_index);
  pcn_log(ctx, LOG_ERR, "[tc-decapsulator]: 'nodes.call'. Module is: %d", module_index);
	return 2;
}
)";

const std::string CTRL_XDP_TX = R"(
#include <bcc/helpers.h>
#include <bcc/proto.h>

#include <uapi/linux/bpf.h>
#include <uapi/linux/if_ether.h>

struct pkt_metadata {
  u16 cube_id;
  u16 in_port;
  u32 packet_len;

  // used to send data to controller
  u16 reason;
  u32 md[3];  // generic metadata
} __attribute__((packed));

BPF_PERF_OUTPUT(controller);
BPF_TABLE_PUBLIC("percpu_array", u32, struct pkt_metadata, port_md, 1);

int controller_module_tx(struct xdp_md *ctx) {
  pcn_log(ctx, LOG_TRACE, "[xdp-encapsulator]: to controller");
  void* data_end = (void*)(long)ctx->data_end;
  void* data = (void*)(long)ctx->data;
  u32 inport_key = 0;
  struct pkt_metadata *int_md;
  struct pkt_metadata md = {0};
  volatile u32 x;

  int_md = port_md.lookup(&inport_key);
  if (int_md == NULL) {
    goto ERROR;
  }

  md.cube_id = int_md->cube_id;
  md.in_port = int_md->in_port;
  md.packet_len = (u32)(data_end - data);
  md.reason = int_md->reason;

  x = int_md->md[0];
  md.md[0] = x;
  x = int_md->md[1];
  md.md[1] = x;
  x = int_md->md[2];
  md.md[2] = x;

  int r = controller.perf_submit_skb(ctx, md.packet_len, &md, sizeof(md));
  if (r != 0) {
    pcn_log(ctx, LOG_ERR, "[xdp-encapsulator]: perf_submit_skb error: %d", r);
  }

  return BPF_REDIRECT;
ERROR:
  pcn_log(ctx, LOG_ERR, "[xdp-encapsulator]: unknown error");
  return XDP_DROP;
}
)";

// Receives packet from controller and forwards it to the CubeXDP
const std::string CTRL_XDP_RX = R"(
#include <bcc/helpers.h>
#include <bcc/proto.h>

#include <uapi/linux/bpf.h>
#include <uapi/linux/if_ether.h>

#include <linux/rcupdate.h>

struct xdp_metadata {
  u16 module_index;
  u16 port_id;
};

struct pkt_metadata {
  u16 module_index;
  u16 in_port;
  u32 packet_len;

  // used to send data to controller
  u16 reason;
  u32 md[3];  // generic metadata
} __attribute__((packed));

BPF_TABLE("extern", int, int, xdp_nodes, _POLYCUBE_MAX_NODES);
BPF_TABLE("extern", u32, struct pkt_metadata, port_md, 1);
BPF_TABLE("array", u32, struct xdp_metadata, md_map_rx, MD_MAP_SIZE);
BPF_TABLE("array", u32, u32, xdp_index_map_rx, 1);

int controller_module_rx(struct xdp_md *ctx) {
  pcn_log(ctx, LOG_TRACE, "[xdp-decapsulator]: from controller");
  u32 key = 0;
  struct pkt_metadata xdp_md = {0};

  u32 zero = 0;
  u32 *index = xdp_index_map_rx.lookup(&zero);
  if (!index) {
    pcn_log(ctx, LOG_ERR, "[xdp-decapsulator]: !index");
    return XDP_DROP;
  }

  rcu_read_lock();

  (*index)++;
  *index %= MD_MAP_SIZE;
  rcu_read_unlock();

  u32 i = *index;

  struct xdp_metadata *md = md_map_rx.lookup(&i);
  if (!md) {
    pcn_log(ctx, LOG_ERR, "[xdp-decapsulator]: !md");
    return XDP_DROP;
  }

  xdp_md.in_port = md->port_id;
  xdp_md.module_index = md->module_index;

  port_md.update(&key, &xdp_md);

  xdp_nodes.call(ctx, xdp_md.module_index);
  pcn_log(ctx, LOG_ERR, "[xdp-decapsulator]: 'xdp_nodes.call'. Module is: %d", xdp_md.module_index);
  return XDP_DROP;
}
)";

void Controller::call_back_proxy(void *cb_cookie, void *data, int data_size) {
  PacketIn *md = static_cast<PacketIn *>(data);

  uint8_t *data_ = static_cast<uint8_t *>(data);
  data_ += sizeof(PacketIn);

  Controller *c = static_cast<Controller *>(cb_cookie);
  if (c == nullptr)
    throw std::runtime_error("Bad controller");

  std::lock_guard<std::mutex> guard(c->cbs_mutex_);

  try {
    std::vector<uint8_t> packet(data_, data_ + md->packet_len);
    auto cb = c->cbs_.at(md->cube_id);
    cb(md, packet);
  } catch (const std::exception &e) {
    // TODO: ignore the problem, what else can we do?
    c->logger->warn("Error processing packet in event: {}", e.what());
  }
}

Controller &Controller::get_tc_instance() {
  PatchPanel::get_tc_instance();
  static Controller instance(CTRL_TC_TX, CTRL_TC_RX, BPF_PROG_TYPE_SCHED_CLS);
  static bool initialized = false;
  if (!initialized) {
    Netlink::getInstance().attach_to_tc(instance.iface_->getName(),
                                        instance.fd_rx_);
    PatchPanel::get_tc_instance().add(instance,
                                      PatchPanel::_POLYCUBE_MAX_NODES - 1);
    initialized = true;
  }
  return instance;
}

Controller &Controller::get_xdp_instance() {
  PatchPanel::get_xdp_instance();
  static Controller instance(CTRL_XDP_TX, CTRL_XDP_RX, BPF_PROG_TYPE_XDP);
  static bool initialized = false;
  if (!initialized) {
    int attach_flags = 0;
    attach_flags |= 2 << 0;
    Netlink::getInstance().attach_to_xdp(instance.iface_->getName(),
                                         instance.fd_rx_, attach_flags);
    PatchPanel::get_xdp_instance().add(instance,
                                       PatchPanel::_POLYCUBE_MAX_NODES - 1);
    initialized = true;
  }
  return instance;
}

Controller::Controller(const std::string &tx_code, const std::string &rx_code,
                       enum bpf_prog_type type)
    : ctrl_rx_md_index_(0),
      logger(spdlog::get("polycubed")),
      id_(PatchPanel::_POLYCUBE_MAX_NODES - 1) {
  ebpf::StatusTuple res(0);

  if (type == BPF_PROG_TYPE_XDP)
    iface_ = std::unique_ptr<viface::VIface>(
        new viface::VIface("pcn_xdp_cp", true, -1));
  else
    iface_ = std::unique_ptr<viface::VIface>(
        new viface::VIface("pcn_tc_cp", true, -1));

  std::vector<std::string> flags;
  flags.push_back(std::string("-D_POLYCUBE_MAX_NODES=") +
                  std::to_string(PatchPanel::_POLYCUBE_MAX_NODES));
  flags.push_back(std::string("-DCUBE_ID=") + std::to_string(get_id()));
  flags.push_back(std::string("-DMD_MAP_SIZE=") + std::to_string(MD_MAP_SIZE));
  // FIXME: this should be taken from a global log level conf
  flags.push_back(std::string("-DLOG_LEVEL=") + std::string("LOG_INFO"));

  // replace code with parsed debug
  auto &&datapath_log = DatapathLog::get_instance();

  handle_log_msg = [&](const LogMsg *msg) -> void { this->log_msg(msg); };

  datapath_log.register_cb(get_id(), handle_log_msg);

  std::string tx_code_(datapath_log.parse_log(tx_code));
  std::string rx_code_(datapath_log.parse_log(rx_code));

  res = tx_module_.init(tx_code_, flags);
  if (res.code() != 0) {
    logger->error("cannot init ctrl_tx: {0}", res.msg());
    throw BPFError("cannot init controller tx program");
  }

  res = tx_module_.load_func("controller_module_tx", type, fd_tx_);
  if (res.code() != 0) {
    logger->error("cannot load ctrl_tx: {0}", res.msg());
    throw BPFError("cannot load controller_module_tx");
  }

  res =
      tx_module_.open_perf_buffer("controller", call_back_proxy, nullptr, this);
  if (res.code() != 0) {
    logger->error("cannot open perf ring buffer for controller: {0}",
                  res.msg());
    throw BPFError("cannot open controller perf buffer");
  }

  std::string cmd_string = "sysctl -w net.ipv6.conf." + iface_->getName() +
                           ".disable_ipv6=1" + "> /dev/null";
  system(cmd_string.c_str());
  iface_->setMTU(9000);
  iface_->up();

  res = rx_module_.init(rx_code_, flags);
  if (res.code() != 0) {
    logger->error("cannot init ctrl_rx: {0}", res.msg());
    throw BPFError("cannot init controller rx program");
  }

  res = rx_module_.load_func("controller_module_rx", type, fd_rx_);
  if (res.code() != 0) {
    logger->error("cannot load ctrl_rx: {0}", res.msg());
    throw BPFError("cannot load controller_module_rx");
  }

  auto t = rx_module_.get_array_table<metadata>("md_map_rx");
  metadata_table_ = std::unique_ptr<ebpf::BPFArrayTable<metadata>>(
      new ebpf::BPFArrayTable<metadata>(t));

  start();
}

Controller::~Controller() {
  stop();
}

uint32_t Controller::get_id() const {
  return id_;
}

void Controller::register_cb(int id, const packet_in_cb &cb) {
  std::lock_guard<std::mutex> guard(cbs_mutex_);
  cbs_.insert(std::pair<int, const packet_in_cb &>(id, cb));
}

void Controller::unregister_cb(int id) {
  std::lock_guard<std::mutex> guard(cbs_mutex_);
  cbs_.erase(id);
}

// caller must guarantee that module_index and port_id are valid
void Controller::send_packet_to_cube(uint16_t module_index, uint16_t port_id,
                                     const std::vector<uint8_t> &packet) {
  ctrl_rx_md_index_++;
  ctrl_rx_md_index_ %= MD_MAP_SIZE;

  metadata md_temp = {module_index, port_id};
  metadata_table_->update_value(ctrl_rx_md_index_, md_temp);

  iface_->send(const_cast<std::vector<uint8_t> &>(packet));
}

void Controller::start() {
  // create a thread that polls the perf ring buffer
  auto f = [&]() -> void {
    stop_ = false;
    while (!stop_) {
      tx_module_.poll_perf_buffer("controller", 500);
    }

    // TODO: this causes a segmentation fault
    //  logger->debug("controller: stopping");
  };

  std::unique_ptr<std::thread> uptr(new std::thread(f));
  pkt_in_thread_ = std::move(uptr);
}

void Controller::stop() {
  //  logger->debug("controller stop()");
  stop_ = true;
  if (pkt_in_thread_) {
    //  logger->debug("trying to join controller thread");
    pkt_in_thread_->join();
  }
}

int Controller::get_fd() const {
  return fd_tx_;
}

void Controller::log_msg(const LogMsg *msg) {
  spdlog::level::level_enum level_ =
      logLevelToSPDLog((polycube::LogLevel)msg->level);
  auto print =
      polycube::service::utils::format_debug_string(msg->msg, msg->args);
  logger->log(level_, print.c_str());
}

}  // namespace polycubed
}  // namespace polycube
