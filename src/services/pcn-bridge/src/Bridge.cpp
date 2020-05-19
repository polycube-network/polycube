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

#include "Bridge.h"
#include "Bridge_dp.h"

Bridge::Bridge(const std::string name, const BridgeJsonObject &conf)
    : Cube(conf.getBase(),
           {generate_code(conf.getStpEnabled(), conf.getFdb().getAgingTime())},
           {}),
      BridgeBase(name),
      quit_thread_(false) {
  logger()->info("Creating Bridge instance");

  stp_enabled_ = conf.getStpEnabled();

  if (conf.macIsSet()) {
    setMac(conf.getMac());
  } else {
    setMac(polycube::service::utils::get_random_mac());
    logger()->debug("[Bridge] Bridge created with random mac: {0}", mac_);
  }

  addPortsList(conf.getPorts());
  insertFdb(conf.getFdb());
  updateStpList(conf.getStp());

  timestamp_update_thread_ = std::thread(&Bridge::updateTimestampTimer, this);
}

Bridge::~Bridge() {
  logger()->info("Destroying Bridge instance");

  quitAndJoin();
  removeFdb();
  delPortsList();
  Cube::dismount();
}

void Bridge::packet_in(Ports &port, polycube::service::PacketInMetadata &md,
                       const std::vector<uint8_t> &packet) {
  logger()->debug("Packet received from port {0}", port.name());
  try {
    switch (static_cast<SlowPathReason>(md.reason)) {
    case SlowPathReason::BROADCAST:
      logger()->debug("reason: BROADCAST");
      broadcastPacket(port, md, packet);
      break;
    case SlowPathReason::BPDU:
      logger()->debug("reason: BPDU");
      // TODO: check that the bpdu is allowed
      if (stp_enabled_)
        processBPDU(port, md, packet);
      else
        broadcastPacket(port, md, packet);
      break;
    default:
      logger()->error("Not valid reason {0} received", md.reason);
    }
  } catch (const std::exception &e) {
    logger()->error("exception during slowpath packet processing: '{0}'",
                    e.what());
  }
}

void Bridge::processBPDU(Ports &port, polycube::service::PacketInMetadata &md,
                         const std::vector<uint8_t> &packet) {
  logger()->trace("Processing BPDU..");
  bool tagged = md.metadata[1];
  uint16_t vlan = md.metadata[0];

  int port_id = port.index();
  if (stps_.count(vlan) == 0) {
    logger()->error("stp instance not present");
    return;
  }

  size_t bpdusize = 0;
  int start = ETH_HEADER_LEN + LLC_HEADER_LEN;

  // build an Ethernet packet to get the STP BPDU later on
  EthernetII ether(&packet[0], packet.size());
  if (tagged) {
    Tins::Dot1Q &dot1 = ether.rfind_pdu<Tins::Dot1Q>();
    bpdusize = dot1.payload_type() - LLC_HEADER_LEN;
    start += DOT1Q_HEADER_LEN;
  } else {
    bpdusize = ether.payload_type() - LLC_HEADER_LEN;
  }

  // take last part of vector
  std::vector<uint8_t>::const_iterator first = packet.begin() + start;
  std::vector<uint8_t>::const_iterator last = packet.begin() + start + bpdusize;
  std::vector<uint8_t> p(first, last);
  getStp(vlan)->processBPDU(port_id, p);
}

void Bridge::broadcastPacket(Ports &port,
                             polycube::service::PacketInMetadata &md,
                             const std::vector<uint8_t> &packet) {
  logger()->trace("Broadcasting packet..");

  bool tagged = md.metadata[1];
  uint16_t vlan = md.metadata[0];
  logger()->trace("metadata vlan:{0} tagged:{1}", vlan, tagged);

  for (auto &it : get_ports()) {
    if (it->name() == port.name()) {
      continue;
    }
    logger()->trace("Sending packet to port {0}", it->name());
    it->send_packet_out(packet, tagged, vlan);
  }
}

std::shared_ptr<Ports> Bridge::getPorts(const std::string &name) {
  return BridgeBase::getPorts(name);
}

std::vector<std::shared_ptr<Ports>> Bridge::getPortsList() {
  return BridgeBase::getPortsList();
}

void Bridge::addPorts(const std::string &name, const PortsJsonObject &conf) {
  BridgeBase::addPorts(name, conf);
}

void Bridge::addPortsList(const std::vector<PortsJsonObject> &conf) {
  BridgeBase::addPortsList(conf);
}

void Bridge::replacePorts(const std::string &name,
                          const PortsJsonObject &conf) {
  BridgeBase::replacePorts(name, conf);
}

void Bridge::delPorts(const std::string &name) {
  BridgeBase::delPorts(name);
}

void Bridge::delPortsList() {
  BridgeBase::delPortsList();
}

std::shared_ptr<Fdb> Bridge::getFdb() {
  if (fdb_ == nullptr)
    throw std::runtime_error("Fdb does not exist");

  return fdb_;
}

void Bridge::addFdb(const FdbJsonObject &value) {
  throw std::runtime_error("Cannot add Fdb container manually");
}

void Bridge::replaceFdb(const FdbJsonObject &conf) {
  throw std::runtime_error("Cannot replace Fdb container manually");
}

void Bridge::delFdb() {
  throw std::runtime_error("Cannot remove Fdb container manually");
}

void Bridge::insertFdb(const FdbJsonObject &value) {
  if (fdb_ != nullptr)
    throw std::runtime_error("Fdb already exists");

  fdb_ = std::make_shared<Fdb>(*this, value);
}

void Bridge::removeFdb() {
  fdb_.reset();
}

bool Bridge::getStpEnabled() {
  return stp_enabled_;
}

void Bridge::setStpEnabled(const bool &value) {
  if (stp_enabled_ == value)
    return;

  try {
    reloadCodeWithStp(value);
  } catch (std::exception &e) {
    logger()->error("Failed to reload the code with stp: {0}", value);
  }

  updatePorts(value);
}

void Bridge::updatePorts(bool enable_stp) {
  if (enable_stp == true) {
    stp_enabled_ = true;
    for (auto port : getPortsList()) {
      port->setStpEnabled();
    }
  } else {
    for (auto port : getPortsList()) {
      port->setStpDisabled();
    }
    stp_enabled_ = false;
  }
}

std::string Bridge::getMac() {
  return mac_;
}

void Bridge::setMac(const std::string &value) {
  if (mac_ == value)
    return;

  mac_ = value;

  if (stp_enabled_) {
    for (auto stp : stps_) {
      stp.second->changeMac(value);
    }
  }
}

std::shared_ptr<Stp> Bridge::getStp(const uint16_t &vlan) {
  if (stps_.count(vlan) == 0) {
    throw std::runtime_error("STP instance does not exist");
  }

  return stps_.at(vlan);
}

std::shared_ptr<Stp> Bridge::getStpCreate(const uint16_t &vlan) {
  if (!stp_enabled_) {
    throw std::runtime_error("STP is disabled");
  }

  if (stps_.count(vlan) == 0) {
    StpJsonObject conf = StpJsonObject();
    conf.setVlan(vlan);
    insertStp(vlan, conf);
  }

  return stps_.at(vlan);
}

std::vector<std::shared_ptr<Stp>> Bridge::getStpList() {
  std::vector<std::shared_ptr<Stp>> stpsv;

  stpsv.reserve(stps_.size());

  for (auto const &value : stps_)
    stpsv.push_back(value.second);

  return stpsv;
}

void Bridge::addStp(const uint16_t &vlan, const StpJsonObject &conf) {
  throw std::runtime_error("Cannot add a STP instance manually");
}

void Bridge::addStpList(const std::vector<StpJsonObject> &conf) {
  throw std::runtime_error("Cannot add STP instances manually");
}

void Bridge::replaceStp(const uint16_t &vlan, const StpJsonObject &conf) {
  throw std::runtime_error("Cannot replace a STP instance manually");
}

void Bridge::delStp(const uint16_t &vlan) {
  throw std::runtime_error("Cannot remove a STP instance manually");
}

void Bridge::delStpList() {
  throw std::runtime_error("Cannot remove STP instances manually");
}

void Bridge::insertStp(const uint16_t &vlan, const StpJsonObject &conf) {
  if (!stp_enabled_) {
    throw std::runtime_error("STP is disabled");
  }

  if (stps_.count(vlan) != 0) {
    throw std::runtime_error("STP instance for this vlan already exists");
  }

  stps_[vlan] = std::make_shared<Stp>(*this, conf);
}

void Bridge::updateStpList(const std::vector<StpJsonObject> &conf) {
  for (auto &i : conf) {
    uint16_t vlan = i.getVlan();

    if (stps_.count(vlan) != 0)
      stps_.at(vlan)->updateParameters(i);
  }
}

void Bridge::removeStp(const uint16_t &vlan) {
  if (!stp_enabled_) {
    throw std::runtime_error("STP is disabled");
  }

  if (stps_.count(vlan) == 0) {
    throw std::runtime_error("STP instance for this vlan does not exist");
  }

  stps_.erase(vlan);
}

// generate the .c code depending on if the stp is enabled or not and on the
// aging time
std::string Bridge::generate_code(bool stp_enabled, uint32_t aging_time) {
  std::string aging_time_str("#define AGING_TIME " +
                             std::to_string(aging_time) + "\n");

  std::string stp_enabled_str("#define STP_ENABLED " +
                              std::to_string(stp_enabled) + "\n");

  return aging_time_str + stp_enabled_str + bridge_code;
}

// reload code if agingtime is changed
void Bridge::reloadCodeWithAgingTime(uint32_t aging_time) {
  reload(generate_code(stp_enabled_, aging_time));
}

// reload code if stpenabled is changed
void Bridge::reloadCodeWithStp(bool stp_enabled) {
  reload(generate_code(stp_enabled, fdb_->getAgingTime()));
}

void Bridge::quitAndJoin() {
  quit_thread_ = true;
  timestamp_update_thread_.join();
}

void Bridge::updateTimestampTimer() {
  do {
    sleep(1);
    updateTimestamp();
  } while (!quit_thread_);
}

/*
 * This method is in charge of updating the timestamp table
 * that is used in the dataplane to avoid calling the bpf_ktime helper
 * that introduces a non-negligible overhead to the eBPF program.
 */
void Bridge::updateTimestamp() {
  try {
    // get timestamp from system
    struct timespec now_timespec;
    clock_gettime(CLOCK_MONOTONIC, &now_timespec);
    auto timestamp_table = get_array_table<uint32_t>("timestamp");
    timestamp_table.set(0, now_timespec.tv_sec);
  } catch (...) {
    logger()->error("Error while updating the timestamp table");
  }
}
