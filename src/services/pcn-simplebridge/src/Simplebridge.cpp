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

// Modify these methods with your own implementation

#include "Simplebridge.h"
#include "Simplebridge_dp.h"

#include <tins/ethernetII.h>
#include <tins/tins.h>
#include <thread>

using namespace Tins;

Simplebridge::Simplebridge(const std::string name,
                           const SimplebridgeJsonObject &conf)
    : Cube(conf.getBase(), {generate_code()}, {}), quit_thread_(false) {
  logger()->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [Simplebridge] [%n] [%l] %v");
  logger()->info("Creating Simplebridge instance");

  addFdb(conf.getFdb());
  addPortsList(conf.getPorts());

  timestamp_update_thread_ =
      std::thread(&Simplebridge::updateTimestampTimer, this);
}

Simplebridge::~Simplebridge() {
  // we are destroying this service, prepare for it.
  quitAndJoin();
  Cube::dismount();
}

void Simplebridge::quitAndJoin() {
  quit_thread_ = true;
  timestamp_update_thread_.join();
}

void Simplebridge::updateTimestampTimer() {
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
void Simplebridge::updateTimestamp() {
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

void Simplebridge::update(const SimplebridgeJsonObject &conf) {
  // This method updates all the object/parameter in Simplebridge object
  // specified in the conf JsonObject.
  // You can modify this implementation.

  // update base cube implementation
  Cube::set_conf(conf.getBase());

  if (conf.fdbIsSet()) {
    auto m = getFdb();
    m->update(conf.getFdb());
  }

  if (conf.portsIsSet()) {
    for (auto &i : conf.getPorts()) {
      auto name = i.getName();
      auto m = getPorts(name);
      m->update(i);
    }
  }
}

SimplebridgeJsonObject Simplebridge::toJsonObject() {
  SimplebridgeJsonObject conf;
  conf.setBase(Cube::to_json());

  conf.setFdb(getFdb()->toJsonObject());

  for (auto &i : getPortsList()) {
    conf.addPorts(i->toJsonObject());
  }

  return conf;
}

std::string Simplebridge::generate_code() {
  return simplebridge_code;
}

std::vector<std::string> Simplebridge::generate_code_vector() {
  throw std::runtime_error("Method not implemented");
}

void Simplebridge::packet_in(Ports &port,
                             polycube::service::PacketInMetadata &md,
                             const std::vector<uint8_t> &packet) {
  logger()->debug("Packet received from port {0}", port.name());

  try {
    // logger()->debug("[{0}] packet from port: '{1}' id: '{2}' peer:'{3}'",
    //  get_name(), port.name(), port.index(), port.peer());

    switch (static_cast<SlowPathReason>(md.reason)) {
    case SlowPathReason::FLOODING:
      logger()->debug("Received a pkt in slowpath - reason: FLOODING");
      flood_packet(port, md, packet);
      break;
    default:
      logger()->error("Not valid reason {0} received", md.reason);
    }
  } catch (const std::exception &e) {
    logger()->error("exception during slowpath packet processing: '{0}'",
                    e.what());
  }
}

void Simplebridge::flood_packet(Port &port, PacketInMetadata &md,
                                const std::vector<uint8_t> &packet) {
  EthernetII p(&packet[0], packet.size());

  // avoid adding or removing ports while flooding packet
  std::lock_guard<std::mutex> guard(ports_mutex_);

  for (auto &it : get_ports()) {
    if (it->name() == port.name()) {
      continue;
    }
    it->send_packet_out(p);
    logger()->trace("Packet sent to port {0} as result of flooding",
                    it->name());
  }
}

void Simplebridge::reloadCodeWithAgingtime(uint32_t aging_time) {
  logger()->debug("Reloading code with agingtime: {0}", aging_time);

  std::string aging_time_str("#define AGING_TIME " +
                             std::to_string(aging_time));

  reload(aging_time_str + simplebridge_code);

  logger()->trace("New bridge code reloaded");
}

std::shared_ptr<Ports> Simplebridge::getPorts(const std::string &name) {
  return get_port(name);
}

std::vector<std::shared_ptr<Ports>> Simplebridge::getPortsList() {
  return get_ports();
}

void Simplebridge::addPorts(const std::string &name,
                            const PortsJsonObject &conf) {
  add_port<PortsJsonObject>(name, conf);

  logger()->info("New port created with name {0}", name);
}

void Simplebridge::addPortsList(const std::vector<PortsJsonObject> &conf) {
  for (auto &i : conf) {
    std::string name_ = i.getName();
    addPorts(name_, i);
  }
}

void Simplebridge::replacePorts(const std::string &name,
                                const PortsJsonObject &conf) {
  delPorts(name);
  std::string name_ = conf.getName();
  addPorts(name_, conf);
}

void Simplebridge::delPorts(const std::string &name) {
  remove_port(name);
}

void Simplebridge::delPortsList() {
  auto ports = get_ports();
  for (auto it : ports) {
    delPorts(it->name());
  }
}

std::shared_ptr<Fdb> Simplebridge::getFdb() {
  if (fdb_ != nullptr) {
    return fdb_;
  } else {
    return std::make_shared<Fdb>(*this);
  }
}

void Simplebridge::addFdb(const FdbJsonObject &value) {
  fdb_ = std::make_shared<Fdb>(*this, value);
}

void Simplebridge::replaceFdb(const FdbJsonObject &conf) {
  delFdb();
  addFdb(conf);
}

void Simplebridge::delFdb() {
  if (fdb_ != nullptr) {
    fdb_->delEntryList();

    // I don't want to delete the Filtering database. This is very strange
    // I'll only reset the agingTime, if needed
    fdb_->setAgingTime(300);
    // parent.fdb_.reset();
    // parent.fdb_ = nullptr;
  } else {
    // This should never happen
    throw std::runtime_error("There is not filtering database in the bridge");
  }
}