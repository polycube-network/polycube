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

#include "Bridge.h"
#include "BridgeSTP.h"

// Spanning tree related methods
BridgeSTP::BridgeSTP(Bridge &parent, uint16_t vlan_id)
    : parent(parent), vlan_id(vlan_id), l(spdlog::get("pcn-bridge")) {
  std::random_device rd;
  std::mt19937 mt(rd());
  std::uniform_int_distribution<uint64_t> dist(1, UINT64_MAX);

  uint64_t bridge_mac = dist(mt);
  bridge_mac &= ~(1ULL << 40);  // unicast
  bridge_mac |= (1ULL << 41);   // locally administrated

  // Copy mac to uint8_t array
  uint64_t t = bridge_mac;
  for (int i = 5; i >= 0; i--) {
    mac[i] = t & 0xFF;
    t >>= 8;
  }

  l->info("{0} mac is {1}", parent.get_name(), mac_to_string(mac));
  stp =
      stp_create(parent.get_name().c_str(), bridge_mac, send_bpdu_proxy, this);
  if (!stp) {
    throw std::runtime_error("failed to create stp instance");
  }

  stp_set_bridge_priority(stp, STP_DEFAULT_BRIDGE_PRIORITY);

  stop_ = false;
  std::unique_ptr<std::thread> uptr(
      new std::thread([&](void) -> void { stp_tick_function(); }));
  stp_tick_thread = std::move(uptr);
}

BridgeSTP::~BridgeSTP() {
  stop_ = true;
  if (stp_tick_thread) {
    l->debug("Trying to join stp_tick thread");
    stp_tick_thread->join();
  }
  stp_unref(stp);
}

void BridgeSTP::add_port(int port_id) {
  if (ports_.count(port_id) != 0) {
    l->error("port {0} already exists on instance {1}", port_id, vlan_id);
    return;
  }

  struct stp_port *sp = stp_get_port(stp, port_id);
  stp_port_enable(sp);
  stp_port_set_speed(sp, 1000000);
  auto p = parent.get_port(port_id);
  stp_port_set_name(sp, p->name().c_str());
  ports_.insert(port_id);
}

void BridgeSTP::remove_port(int port_id) {
  if (ports_.count(port_id) == 0) {
    l->error("port {0} does not exist on instance {1}", port_id, vlan_id);
    return;
  }
  struct stp_port *sp = stp_get_port(stp, port_id);
  stp_port_disable(sp);
  ports_.erase(port_id);
}

int BridgeSTP::ports_count() {
  return ports_.size();
}

void BridgeSTP::process_bpdu(int port_id, const std::vector<uint8_t> &packet) {
  struct stp_port *sp = stp_get_port(stp, port_id);
  if (!sp) {
    l->warn("Stp port {0} not found", port_id);
    return;
  }
  // Headers are already stripped
  size_t s = packet.size();
  const void *p = &packet[0];
  stp_received_bpdu(sp, p, s);
  std::lock_guard<std::mutex> lock(stp_mutex);
  update_ports_state();
  update_fwd_table();
}

void BridgeSTP::send_bpdu(struct ofpbuf *bpdu, int port_no, void *aux) {
  // l->debug("Sending BPDU");
  struct stp_port *sp = stp_get_port(stp, port_no);
  if (!stp_should_forward_bpdu(stp_port_get_state(sp))) {
    l->debug("Port should not forward BDPU");
    return;
  }

  auto &port = parent.get_bridge_port(port_no);
  // is a peer on the port?
  if (port.peer().empty())
    return;

  uint8_t *buf = (uint8_t *)ofpbuf_base(bpdu);
  memcpy(buf + 6, mac, ETH_ADDR_LEN);  // set source mac

  EthernetII packet(buf, ofpbuf_size(bpdu));
  ofpbuf_free(bpdu);

  switch (port.mode()) {
  case PortMode::ACCESS:
    if (port.access_vlan() != vlan_id) {
      l->info("[{0}]: impossible to send bpdu on access port {0}",
              parent.get_name(), port.name());
      return;
    }

    port.Port::send_packet_out(packet);
    break;
  case PortMode::TRUNK:
    if (!port.is_trunk_vlan_allowed(vlan_id)) {
      l->info("[{0}]: impossible to send bpdu on trunk port {0}",
              parent.get_name(), port.name());
      return;
    }

    if (vlan_id == port.native_vlan()) {
      port.Port::send_packet_out(packet);
    } else {
      HWAddress<6> sstp("01:00:0c:cc:cc:cd");
      packet.dst_addr(sstp);
      EthernetII tagged_p = EthernetII(packet.dst_addr(), packet.src_addr());

      Dot1Q dot1q(vlan_id);
      dot1q.payload_type(packet.payload_type());
      tagged_p /= dot1q;

      PDU *p_inner = packet.inner_pdu();
      tagged_p /= *p_inner;

      port.Port::send_packet_out(tagged_p);
    }
    break;
  }
}

void BridgeSTP::send_bpdu_proxy(struct ofpbuf *bpdu, int port_no, void *aux) {
  BridgeSTP *br_stp = static_cast<BridgeSTP *>(aux);
  if (br_stp == nullptr) {
    throw std::runtime_error("Bad BridgeSTP");
  }

  br_stp->send_bpdu(bpdu, port_no, aux);
}

void BridgeSTP::update_ports_state() {
  struct stp_port *sp = nullptr;

  while (stp_get_changed_port(stp, &sp)) {
    uint32_t vlan;
    enum stp_state state;
    try {
      state = stp_port_get_state(sp);
      auto &bp = parent.get_bridge_port(stp_port_no(sp));
      l->info("[{0}]: port {1} has changed the state to {2}", parent.get_name(),
              bp.name(), stp_state_name(state));

      if (bp.mode() == PortMode::ACCESS)
        vlan = VLAN_WILDCARD;
      else if (bp.mode() == PortMode::TRUNK)
        vlan = vlan_id;
    } catch (...) {
      continue;
    }

    port_vlan_key key{
        .port = uint32_t(stp_port_no(sp)), .vlan = vlan,
    };

    port_vlan_value value{
        .vlan = vlan_id, .stp_state = state,
    };

    auto port_vlan_table =
        parent.get_hash_table<port_vlan_key, port_vlan_value>("port_vlan");
    port_vlan_table.set(key, value);
  }
}

void BridgeSTP::update_fwd_table() {
  // should the fwd table be flushed?
  if (stp_check_and_reset_fdb_flush(stp)) {
    l->info("[{0}]: flushing forwarding table", parent.get_name());
    auto fwdtable = parent.get_hash_table<fwd_key, fwd_entry>("fwdtable");
    fwdtable.remove_all();
    // TODO: only entries regarding this vlan should be removed
  }
}

void BridgeSTP::stp_tick_function(void) {
  const auto timeWindow = std::chrono::milliseconds(500);

  auto last = std::chrono::steady_clock::now();

  while (!stop_) {
    std::this_thread::sleep_for(timeWindow);
    auto now = std::chrono::steady_clock::now();
    int ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - last)
                 .count();
    last = now;
    stp_tick(stp, ms);
    std::lock_guard<std::mutex> lock(stp_mutex);
    update_ports_state();
    update_fwd_table();
  }
}

bool BridgeSTP::port_should_forward(int port_id) {
  struct stp_port *sp = stp_get_port(stp, port_id);
  return stp_forward_in_state(stp_port_get_state(sp));
}

void BridgeSTP::set_priority(uint16_t priority) {
  stp_set_bridge_priority(stp, priority);
}

void BridgeSTP::set_hello_time(int hello_time) {
  stp_set_hello_time(stp, hello_time * 1000);
}

void BridgeSTP::set_max_age(int max_age) {
  stp_set_max_age(stp, max_age * 1000);
}

void BridgeSTP::set_forward_delay(int forward_delay) {
  stp_set_forward_delay(stp, forward_delay * 1000);
}

uint16_t BridgeSTP::get_priority() const {
  stp_identifier id = stp_get_bridge_id(stp);
  return (id >> 48) & 0xffff;
}

std::string BridgeSTP::get_address() const {
  return mac_to_string(mac);
}

std::string BridgeSTP::get_designated_root() const {
  uint8_t mac[ETH_ADDR_LEN];
  uint64_t t = stp_get_designated_root(stp);
  t &= 0xffffffffffffULL;
  for (int i = 5; i >= 0; i--) {
    mac[i] = t & 0xFF;
    t >>= 8;
  }

  return mac_to_string(mac);
}

bool BridgeSTP::is_root_bridge() const {
  return stp_is_root_bridge(stp);
}

int BridgeSTP::get_root_path_cost() const {
  return stp_get_root_path_cost(stp);
}

int BridgeSTP::get_hello_time() const {
  return stp_get_hello_time(stp) / 1000;
}

int BridgeSTP::get_max_age() const {
  return stp_get_max_age(stp) / 1000;
}

int BridgeSTP::get_forward_delay() const {
  return stp_get_forward_delay(stp) / 1000;
}

bool BridgeSTP::check_and_reset_fdb_flush() const {
  return stp_check_and_reset_fdb_flush(stp);
}

enum stp_state BridgeSTP::get_port_state(int port_id) {
  return stp_port_get_state(stp_get_port(stp, port_id));
}

std::string BridgeSTP::get_port_state_str(int port_id) {
  return stp_state_name(get_port_state(port_id));
}

enum stp_role BridgeSTP::get_port_role(int port_id) {
  return stp_port_get_role(stp_get_port(stp, port_id));
}

void BridgeSTP::set_port_priority(int port_id, uint8_t new_priority) {
  stp_port_set_priority(stp_get_port(stp, port_id), new_priority);
}

void BridgeSTP::set_port_path_cost(int port_id, uint16_t path_cost) {
  stp_port_set_path_cost(stp_get_port(stp, port_id), path_cost);
}

void BridgeSTP::set_port_speed(int port_id, unsigned int speed) {
  stp_port_set_speed(stp_get_port(stp, port_id), speed);
}

void BridgeSTP::enable_port_change_detection(int port_id) {
  stp_port_enable_change_detection(stp_get_port(stp, port_id));
}

void BridgeSTP::disable_port_change_detection(int port_id) {
  stp_port_disable_change_detection(stp_get_port(stp, port_id));
}

std::string BridgeSTP::mac_to_string(const uint8_t *mac) const {
  std::stringstream ret;
  ret << std::setfill('0') << std::setw(2) << std::hex << (int)mac[0] << ":";
  ret << std::setfill('0') << std::setw(2) << std::hex << (int)mac[1] << ":";
  ret << std::setfill('0') << std::setw(2) << std::hex << (int)mac[2] << ":";
  ret << std::setfill('0') << std::setw(2) << std::hex << (int)mac[3] << ":";
  ret << std::setfill('0') << std::setw(2) << std::hex << (int)mac[4] << ":";
  ret << std::setfill('0') << std::setw(2) << std::hex << (int)mac[5];
  return ret.str();
}
