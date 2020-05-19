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
#include "patchpanel.h"
#include "utils/netlink.h"

#include "polycube/services/utils.h"

#include <iostream>
#include <tins/tins.h>

using namespace polycube::service;
using namespace Tins;

namespace polycube {
namespace polycubed {

std::map<int, const packet_in_cb &> Controller::cbs_;

std::mutex Controller::cbs_mutex_;

// Perf buffer used to send packets to the controller
const std::string CTRL_PERF_BUFFER = R"(
BPF_TABLE_PUBLIC("perf_output", int, __u32, _BUFFER_NAME, 0);
)";

// Receives packet from controller and forwards it to the Cube
const std::string CTRL_TC_RX = R"(
#include <bcc/helpers.h>
#include <bcc/proto.h>
#include <uapi/linux/bpf.h>
#include <linux/skbuff.h>

#include <linux/rcupdate.h>

/* TODO: move the definition to a file shared by control & data path*/
#define MD_PKT_FROM_CONTROLLER  (1UL << 0)
#define MD_EGRESS_CONTEXT       (1UL << 1)

BPF_TABLE("extern", int, int, nodes, _POLYCUBE_MAX_NODES);

struct metadata {
	u16 module_index;
  u8 is_netdev;
	u16 port_id;
  u32 flags;
} __attribute__((packed));

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
  u8 is_netdev = md->is_netdev;
  u32 flags = md->flags;

	ctx->cb[0] = in_port << 16 | module_index;
  ctx->cb[2] = flags;
  if (is_netdev) {
    return bpf_redirect(module_index, 0);
  } else if (module_index == 0xffff) {
    pcn_log(ctx, LOG_INFO, "[tc-decapsulator]: NH is stack, flags: 0x%x", flags);
    if (flags & MD_EGRESS_CONTEXT) {
        return bpf_redirect(in_port, 0);
    } else {
        return 0;
    }
  }

	nodes.call(ctx, module_index);
  pcn_log(ctx, LOG_ERR, "[tc-decapsulator]: 'nodes.call'. Module is: %d", module_index);
	return 2;
}
)";

// Receives packet from controller and forwards it to the CubeXDP
const std::string CTRL_XDP_RX = R"(
#include <linux/string.h>
#include <linux/rcupdate.h>

struct xdp_metadata {
  u16 module_index;
  u8 is_netdev;
  u16 port_id;
} __attribute__((packed));

struct pkt_metadata {
  u16 module_index;
  u16 in_port;
  u32 packet_len;
  u32 traffic_class;

  // used to send data to controller
  u16 reason;
  u32 md[3];  // generic metadata
} __attribute__((packed));

BPF_TABLE("extern", int, int, xdp_nodes, _POLYCUBE_MAX_NODES);
BPF_TABLE_PUBLIC("percpu_array", u32, struct pkt_metadata, port_md, 1);
BPF_TABLE("array", u32, struct xdp_metadata, md_map_rx, MD_MAP_SIZE);
BPF_TABLE("array", u32, u32, xdp_index_map_rx, 1);

int controller_module_rx(struct xdp_md *ctx) {
  pcn_log(ctx, LOG_TRACE, "[xdp-decapsulator]: from controller");
  int zero = 0;

  struct pkt_metadata *md = port_md.lookup(&zero);
  if (!md) {
    return XDP_ABORTED;
  }

  u32 *index = xdp_index_map_rx.lookup(&zero);
  if (!index) {
    pcn_log(ctx, LOG_ERR, "[xdp-decapsulator]: !index");
    return XDP_ABORTED;
  }

  rcu_read_lock();

  (*index)++;
  *index %= MD_MAP_SIZE;
  rcu_read_unlock();

  u32 i = *index;

  struct xdp_metadata *xdp_md = md_map_rx.lookup(&i);
  if (!xdp_md) {
    pcn_log(ctx, LOG_ERR, "[xdp-decapsulator]: !xdp_md");
    return XDP_ABORTED;
  }

  // Initialize metadata
  md->in_port = xdp_md->port_id;
  md->packet_len = ctx->data_end - ctx->data;
  md->traffic_class = 0;
  memset(md->md, 0, sizeof(md->md));

  if (xdp_md->is_netdev) {
    return bpf_redirect(xdp_md->module_index, 0);
  } else if (xdp_md->module_index == 0xffff) {
    pcn_log(ctx, LOG_INFO, "[xdp-decapsulator]: NH is stack");
    return XDP_PASS;
  } else {
    xdp_nodes.call(ctx, xdp_md->module_index);
    pcn_log(ctx, LOG_ERR, "[xdp-decapsulator]: 'xdp_nodes.call'. Module is: %d", xdp_md->module_index);
    return XDP_ABORTED;
  }
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
  static Controller instance("controller_tc", CTRL_TC_RX,
                             BPF_PROG_TYPE_SCHED_CLS);
  static bool initialized = false;
  if (!initialized) {
    Netlink::getInstance().attach_to_tc(instance.iface_->getName(),
                                        instance.fd_rx_);
    initialized = true;
  }
  return instance;
}

Controller &Controller::get_xdp_instance() {
  static Controller instance("controller_xdp", CTRL_XDP_RX, BPF_PROG_TYPE_XDP);
  static bool initialized = false;
  if (!initialized) {
    int attach_flags = 0;
    attach_flags |= 2 << 0;
    Netlink::getInstance().attach_to_xdp(instance.iface_->getName(),
                                         instance.fd_rx_, attach_flags);
    initialized = true;
  }
  return instance;
}

Controller::Controller(const std::string &buffer_name,
                       const std::string &rx_code, enum bpf_prog_type type)
    : buffer_name_(buffer_name),
      ctrl_rx_md_index_(0),
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

  // Load perf buffer
  std::string buffer_code(CTRL_PERF_BUFFER);
  buffer_code.replace(buffer_code.find("_BUFFER_NAME"), 12, buffer_name);
 
  res = buffer_module_.init(buffer_code, flags);
  if (res.code() != 0) {
    logger->error("cannot init ctrl perf buffer: {0}", res.msg());
    throw BPFError("cannot init controller perf buffer");
  }

  res = buffer_module_.open_perf_buffer(buffer_name_, call_back_proxy, nullptr,
                                        this);
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

  res = rx_module_.init(datapath_log.parse_log(rx_code), flags);
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
void Controller::send_packet_to_cube(uint16_t module_index, bool is_netdev,
                                     uint16_t port_id,
                                     const std::vector<uint8_t> &packet,
                                     service::Direction direction,
                                     bool mac_overwrite) {
  ctrl_rx_md_index_++;
  ctrl_rx_md_index_ %= MD_MAP_SIZE;

  metadata md_temp = {module_index, (uint8_t)int(is_netdev), port_id,
                      MD_PKT_FROM_CONTROLLER};
  if (direction == service::Direction::EGRESS) {
      md_temp.flags |= MD_EGRESS_CONTEXT;
  }
  metadata_table_->update_value(ctrl_rx_md_index_, md_temp);

  if (mac_overwrite) {
      /* if the packet is coming from the ingress context of a
       * transparent cube that is attached to a net device
       * interface, the destination MAC address needs to be
       * adjusted to the controller interface otherwise kernel
       * stack will drop the packet (PACKET_OTHERS)
       */
      EthernetII pkt(&packet[0], packet.size());
      HWAddress<6> mac(iface_->getMAC());
      pkt.dst_addr(mac);
      const std::vector<uint8_t> &p = pkt.serialize();
      iface_->send(const_cast<std::vector<uint8_t> &>(p));
  } else {
      iface_->send(const_cast<std::vector<uint8_t> &>(packet));
  }
}

void Controller::start() {
  // create a thread that polls the perf ring buffer
  auto f = [&]() -> void {
    stop_ = false;
    while (!stop_) {
      buffer_module_.poll_perf_buffer(buffer_name_, 500);
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
