/*
 * Copyright 2019 The Polycube Authors
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

#include "Stp.h"
#include "Bridge.h"

Stp::Stp(Bridge &parent, const StpJsonObject &conf)
    : StpBase(parent), stop_(false) {
  logger()->info("Creating Stp instance");

  if (!conf.vlanIsSet()) {
    throw std::runtime_error("[Stp] vlan is not set");
  }
  vlan_ = conf.getVlan();

  stp_ = stp_create(parent.get_name().c_str(),
                    reverseMac(polycube::service::utils::mac_string_to_be_uint(
                        parent.getMac())),
                    sendBPDUproxy, this);

  if (!stp_) {
    throw std::runtime_error("Failed to create stp instance");
  }

  updateParameters(conf);

  stp_tick_thread_ = std::thread(&Stp::stpTickFunction, this);
}

Stp::~Stp() {
  logger()->info("Destroying Stp instance for vlan {0}", vlan_);

  stop_ = true;

  logger()->debug("Trying to join stp_tick thread");
  stp_tick_thread_.join();

  stp_unref(stp_);
}

uint16_t Stp::getVlan() {
  return vlan_;
}

uint16_t Stp::getPriority() {
  return priority_;
}

void Stp::setPriority(const uint16_t &value) {
  if (priority_ == value)
    return;

  priority_ = value;
  stp_set_bridge_priority(stp_, value);
}

uint32_t Stp::getForwardDelay() {
  return forward_delay_;
}

void Stp::setForwardDelay(const uint32_t &value) {
  if (forward_delay_ == value)
    return;

  forward_delay_ = value;
  stp_set_forward_delay(stp_, value * 1000);
}

uint32_t Stp::getHelloTime() {
  return hello_time_;
}

void Stp::setHelloTime(const uint32_t &value) {
  if (hello_time_ == value)
    return;

  hello_time_ = value;
  stp_set_hello_time(stp_, value * 1000);
}

uint32_t Stp::getMaxMessageAge() {
  return max_message_age_;
}

void Stp::setMaxMessageAge(const uint32_t &value) {
  if (max_message_age_ == value)
    return;

  max_message_age_ = value;
  stp_set_max_age(stp_, value * 1000);
}

struct stp *Stp::getStpInstance() {
  return stp_;
}

void Stp::incrementCounter() {
  counter_++;
}

void Stp::decrementCounter() {
  counter_--;

  if (counter_ == 0) {
    // delete this instance.
    parent_.removeStp(vlan_);
  }
}

void Stp::changeMac(const std::string &mac) {
  stp_set_bridge_id(
      stp_, reverseMac(polycube::service::utils::mac_string_to_be_uint(mac)));

  stp_set_bridge_priority(stp_, priority_);
}

void Stp::sendBPDUproxy(struct ofpbuf *bpdu, int port_no, void *aux) {
  Stp *br_stp = static_cast<Stp *>(aux);
  if (br_stp == nullptr) {
    throw std::runtime_error("Bad Stp");
  }

  br_stp->sendBPDU(bpdu, port_no, aux);
}

void Stp::sendBPDU(struct ofpbuf *bpdu, int port_no, void *aux) {
  logger()->debug("Sending BPDU");

  struct stp_port *sp = stp_get_port(stp_, port_no);
  if (!stp_should_forward_bpdu(stp_port_get_state(sp))) {
    logger()->error("Port should not forward BPDU");
    return;
  }

  std::shared_ptr<Ports> port;

  try {
    port = parent_.get_port(port_no);
  } catch (std::runtime_error e) {
    logger()->trace("Cannot send BPDU, port does not exist anymore");
    return;
  }

  // is a peer on the port?
  if (port->peer().empty())
    return;

  uint8_t *buf = (uint8_t *)ofpbuf_base(bpdu);

  uint64_t mac =
      polycube::service::utils::mac_string_to_be_uint(parent_.getMac());

  memcpy(buf + 6, &mac, ETH_ADDR_LEN);  // set source mac

  EthernetII packet(buf, ofpbuf_size(bpdu));
  ofpbuf_free(bpdu);

  switch (port->getMode()) {
  case PortsModeEnum::ACCESS:
    if (port->getAccess()->getVlanid() != vlan_) {
      logger()->error("[{0}]: impossible to send bpdu on access port {0}",
                      parent_.get_name(), port->name());
      return;
    }

    port->Port::send_packet_out(packet);
    break;

  case PortsModeEnum::TRUNK:
    if (!port->getTrunk()->isAllowed(vlan_)) {
      logger()->error("[{0}]: impossible to send bpdu on trunk port {0}",
                      parent_.get_name(), port->name());
      return;
    }

    if (port->getTrunk()->isNative(vlan_)) {
      port->Port::send_packet_out(packet);
    } else {
      Tins::HWAddress<6> sstp("01:00:0c:cc:cc:cd");
      packet.dst_addr(sstp);
      EthernetII tagged_p = EthernetII(packet.dst_addr(), packet.src_addr());

      Tins::Dot1Q dot1q(vlan_);
      dot1q.payload_type(packet.payload_type());
      tagged_p /= dot1q;

      Tins::PDU *p_inner = packet.inner_pdu();
      tagged_p /= *p_inner;

      port->Port::send_packet_out(tagged_p);
    }
    break;
  }
}

void Stp::stpTickFunction() {
  const auto timeWindow = std::chrono::milliseconds(500);

  auto last = std::chrono::steady_clock::now();

  while (!stop_) {
    std::this_thread::sleep_for(timeWindow);
    auto now = std::chrono::steady_clock::now();
    int ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - last)
                 .count();
    last = now;
    stp_tick(stp_, ms);
    std::lock_guard<std::mutex> lock(stp_mutex_);

    if (stop_)
      break;
    updatePortsState();
    updateFwdTable();
  }
}

void Stp::processBPDU(int port_id, const std::vector<uint8_t> &packet) {
  struct stp_port *sp = stp_get_port(stp_, port_id);
  if (!sp) {
    logger()->warn("Stp port {0} not found", port_id);
    return;
  }
  // Headers are already stripped
  size_t s = packet.size();
  const void *p = &packet[0];
  stp_received_bpdu(sp, p, s);
  std::lock_guard<std::mutex> lock(stp_mutex_);
  updatePortsState();
  updateFwdTable();
}

void Stp::updateFwdTable() {
  if (stp_check_and_reset_fdb_flush(stp_)) {
    try {
      parent_.getFdb()->flushOldEntries(vlan_, forward_delay_);
    } catch (std::runtime_error e) {
      logger()->trace("Filtering database does not exist");
      return;
    }
  }
}

void Stp::updatePortsState() {
  struct stp_port *sp = nullptr;

  while (stp_get_changed_port(stp_, &sp)) {
    if (stop_)
      return;

    uint16_t vlan;
    enum stp_state state;
    try {
      state = stp_port_get_state(sp);
      auto bp = parent_.get_port(stp_port_no(sp));
      logger()->debug("[{0}]: port {1} has changed the state to {2}",
                      parent_.get_name(), bp->name(), stp_state_name(state));

      if (bp->getMode() == PortsModeEnum::ACCESS)
        vlan = VLAN_WILDCARD;
      else if (bp->getMode() == PortsModeEnum::TRUNK)
        vlan = vlan_;
    } catch (...) {
      continue;
    }

    port_vlan_key key{
        .port = uint32_t(stp_port_no(sp)),
        .vlan = vlan,
    };

    port_vlan_value value{
        .vlan = vlan_,
        .stp_state = state,
    };

    auto port_vlan_table =
        parent_.get_hash_table<port_vlan_key, port_vlan_value>("port_vlan");
    port_vlan_table.set(key, value);
  }
}

bool Stp::portShouldForward(int port_id) {
  struct stp_port *sp = stp_get_port(stp_, port_id);
  return stp_forward_in_state(stp_port_get_state(sp));
}

uint64_t Stp::reverseMac(uint64_t mac) {
  uint64_t newMac = 0;

  for (int i = 0; i < 6; i++) {
    newMac |= (((mac >> i * 8) & 0xFF) << 40 - 8 * i);
  }

  return newMac;
}

void Stp::updateParameters(const StpJsonObject &conf) {
  if (conf.priorityIsSet())
    setPriority(conf.getPriority());

  if (conf.forwardDelayIsSet())
    setForwardDelay(conf.getForwardDelay());

  if (conf.helloTimeIsSet())
    setHelloTime(conf.getHelloTime());

  if (conf.maxMessageAgeIsSet())
    setMaxMessageAge(conf.getMaxMessageAge());
}
