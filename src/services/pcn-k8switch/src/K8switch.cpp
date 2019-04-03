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

#include "K8switch.h"
#include <tins/ethernetII.h>
#include <tins/tins.h>
#include <cinttypes>
#include "K8switch_dp.h"

using namespace polycube::service;
using namespace Tins;

K8switch::K8switch(const std::string name, const K8switchJsonObject &conf)
    : Cube(conf.getBase(), {}, {}) {
  logger()->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [K8switch] [%n] [%l] %v");
  logger()->info("Creating K8switch instance");

  // call do... functions to avoid reloading code so many times
  doSetClusterIpSubnet(conf.getClusterIpSubnet());
  doSetClientSubnet(conf.getClientSubnet());
  doSetVirtualClientSubnet(conf.getVirtualClientSubnet());

  // reload code a single time
  reloadConfig();

  addServiceList(conf.getService());
  addPortsList(conf.getPorts());

  // fill list of free ports and control structures
  buf_size = 65536;
  ncpus = polycube::get_possible_cpu_count();

  auto free_ports_buffer = get_percpuarray_table<uint64_t>("free_ports_buffer");
  auto free_ports_cb = get_array_table<cb_control>("free_ports_cb");

  // fill buffer with list of ports
  uint16_t n = (65535 - 1024) / ncpus;
  uint64_t port_counter = 1024;
  for (int i = 0; i < n; i++) {
    std::vector<uint64_t> values;
    for (int j = 0; j < ncpus; j++) {
      values.push_back(port_counter++);
    }
    // update value
    free_ports_buffer.set(i, values);
  }

  // init control structures
  for (int j = 0; j < ncpus; j++) {
    cb_control value{
        .reader = 0, .writer = n, .size = buf_size,
    };
    free_ports_cb.set(j, value);
  }

  stop_ = false;
  std::unique_ptr<std::thread> uptr(
      new std::thread([&](void) -> void { tick(); }));
  tick_thread = std::move(uptr);
}

K8switch::~K8switch() {
  stop_ = true;
  if (tick_thread) {
    logger()->debug("Trying to join tick thread");
    tick_thread->join();
  }
}

void K8switch::update(const K8switchJsonObject &conf) {
  Cube::set_conf(conf.getBase());

  if (conf.serviceIsSet()) {
    for (auto &i : conf.getService()) {
      auto vip = i.getVip();
      auto vport = i.getVport();
      auto proto = i.getProto();
      auto m = getService(vip, vport, proto);
      m->update(i);
    }
  }

  if (conf.portsIsSet()) {
    for (auto &i : conf.getPorts()) {
      auto name = i.getName();
      auto m = getPorts(name);
      m->update(i);
    }
  }
}

K8switchJsonObject K8switch::toJsonObject() {
  K8switchJsonObject conf;
  conf.setBase(Cube::to_json());

  for (auto &i : getServiceList()) {
    conf.addService(i->toJsonObject());
  }

  for (auto &i : getPortsList()) {
    conf.addPorts(i->toJsonObject());
  }

  conf.setClusterIpSubnet(getClusterIpSubnet());
  conf.setClientSubnet(getClientSubnet());
  conf.setVirtualClientSubnet(getVirtualClientSubnet());

  //  Remove comments when you implement all sub-methods
  // for(auto &i : getFwdTableList()){
  //  conf.addFwdTable(i->toJsonObject());
  //}
  return conf;
}

void K8switch::packet_in(Ports &port, polycube::service::PacketInMetadata &md,
                         const std::vector<uint8_t> &packet) {}

std::string K8switch::getClusterIpSubnet() {
  return cluster_ip_cidr_;
}

void K8switch::setClusterIpSubnet(const std::string &value) {
  doSetClusterIpSubnet(value);
  reloadConfig();
}

void K8switch::doSetClusterIpSubnet(const std::string &value) {
  parse_cidr(value, &cluster_ip_subnet_, &cluster_ip_mask_);
  cluster_ip_cidr_ = value;
}

std::string K8switch::getClientSubnet() {
  return client_cidr_;
}

void K8switch::setClientSubnet(const std::string &value) {
  doSetClientSubnet(value);
  reloadConfig();
}

void K8switch::doSetClientSubnet(const std::string &value) {
  parse_cidr(value, &client_subnet_, &client_mask_);
  client_cidr_ = value;
}

std::string K8switch::getVirtualClientSubnet() {
  return virtual_client_cidr_;
}

void K8switch::setVirtualClientSubnet(const std::string &value) {
  doSetVirtualClientSubnet(value);
  reloadConfig();
}

void K8switch::doSetVirtualClientSubnet(const std::string &value) {
  parse_cidr(value, &virtual_client_subnet_, &virtual_client_mask_);
  virtual_client_cidr_ = value;
}

void K8switch::reloadConfig() {
  std::string flags;

  // ports
  uint16_t nodeport_port = 0;

  for (auto &it : get_ports()) {
    switch (it->getType()) {
    case PortsTypeEnum::NODEPORT:
      nodeport_port = it->index();
      break;
    }
  }

  flags += "#define NODEPORT_PORT " + std::to_string(nodeport_port) + "\n";

  flags += "#define CLUSTER_IP_SUBNET " +
           std::to_string(htonl(cluster_ip_subnet_)) + "\n";
  flags += "#define CLUSTER_IP_MASK " +
           std::to_string(htonl(cluster_ip_mask_)) + "\n";

  flags += "#define CLIENT_SUBNET_MASK " + std::to_string(htonl(client_mask_)) +
           "\n";
  flags +=
      "#define CLIENT_SUBNET " + std::to_string(htonl(client_subnet_)) + "\n";
  flags += "#define VIRTUAL_CLIENT_SUBNET " +
           std::to_string(htonl(virtual_client_subnet_)) + "\n";

  logger()->debug("Reloading code with flags port: {}", flags);

  reload(flags + k8switch_code);

  logger()->trace("New k8switch code loaded");
}

std::shared_ptr<Ports> K8switch::getNodePortPort() {
  for (auto &it : get_ports()) {
    if (it->getType() == PortsTypeEnum::NODEPORT) {
      return it;
    }
  }
  return nullptr;
}

void K8switch::cleanupSessionTable() {
  try {
    auto session_table_out =
        get_hash_table<nodeport_session_in, nodeport_session_out>(
            "nodeport_session_out");
    auto session_table_in =
        get_hash_table<nodeport_session_out, nodeport_session_in>(
            "nodeport_session_in");
    auto free_ports_buffer =
        get_percpuarray_table<uint64_t>("free_ports_buffer");
    auto free_ports_cb = get_array_table<cb_control>("free_ports_cb");

    auto session_table_offline = session_table_out.get_all();

    for (auto &pair : session_table_offline) {
      auto key = pair.first;
      auto value = pair.second;
      // is the entry too old and have to be cleanup?
      if (timestampToAge(value.timestamp) > sessionTableTimeout) {
        nodeport_session_out key_in = value;
        key_in.timestamp = 0;  // entries are saved without timestamp
        auto value_in = session_table_in.get(key_in);
        if (timestampToAge(value_in.timestamp) > sessionTableTimeout) {
          session_table_out.remove(key);
          session_table_in.remove(key_in);

          // add port of that entry
          uint16_t free_port = ntohs(key.port_src);
          logger()->trace("port {0} is now free", free_port);

          uint32_t cpu = free_port % ncpus;
          // get writer position
          cb_control cb = free_ports_cb.get(cpu);

          // insert port in circular buffer
          // this is racy, it is possible that ports is changes between this
          // updates:
          // https://github.com/iovisor/bcc/issues/1886
          auto ports = free_ports_buffer.get(cb.writer);
          ports[cpu] = free_port;
          free_ports_buffer.set(cb.writer, ports);
          // update writer position
          cb.writer++;
          cb.writer %= buf_size;

          free_ports_cb.set(cpu, cb);
        }
      }
    }
  } catch (const std::exception &e) {
    logger()->error("{0}", e.what());
  }
}

uint32_t K8switch::timestampToAge(const uint64_t timestamp) {
  struct timespec now_timespec;
  clock_gettime(CLOCK_MONOTONIC, &now_timespec);
  const uint64_t SEC2NANOSEC = 1000000000ULL;
  uint64_t now = now_timespec.tv_sec * SEC2NANOSEC + now_timespec.tv_nsec;
  return (now - timestamp) / SEC2NANOSEC;
}

void K8switch::tick() {
  const auto timeWindow = std::chrono::seconds(1);

  unsigned int counter = 0;

  while (!stop_) {
    std::this_thread::sleep_for(timeWindow);
    counter++;
    if (counter == 60 && !stop_) {
      cleanupSessionTable();
      counter = 0;
    }
  }
}

uint32_t K8switch::ip_to_dec(const std::string &ip) {
  // TODO: use uint8_t
  unsigned short a, b, c, d;

  std::sscanf(ip.c_str(), "%hu.%hu.%hu.%hu", &a, &b, &c, &d);
  return (a << 24) + (b << 16) + (c << 8) + d;
}

void K8switch::parse_cidr(const std::string &cidr, uint32_t *subnet,
                          uint32_t *netmask) {
  std::string net_str = cidr.substr(0, cidr.find("/"));
  std::string mask_len = cidr.substr(cidr.find("/") + 1, std::string::npos);

  *subnet = ip_to_dec(net_str);
  *netmask = 0xFFFFFFFF;
  *netmask <<= 32 - std::stoi(mask_len);
}

// services related functions
std::shared_ptr<Service> K8switch::getService(const std::string &vip,
                                              const uint16_t &vport,
                                              const ServiceProtoEnum &proto) {
  uint16_t protoNumber = Service::convertProtoToNumber(proto);

  auto key = Service::Key(vip, vport, protoNumber);

  if (service_map_.count(key) == 0) {
    logger()->error("Service {}:{}/{} does not exist", vip, vport,
                    ServiceJsonObject::ServiceProtoEnum_to_string(proto));
    throw std::runtime_error("Service does not exist");
  }

  return std::shared_ptr<Service>(&service_map_.at(key), [](Service *) {});
}

std::vector<std::shared_ptr<Service>> K8switch::getServiceList() {
  std::vector<std::shared_ptr<Service>> services_vect;

  for (auto &it : service_map_) {
    auto ptr = std::shared_ptr<Service>(&it.second, [](Service *) {});
    services_vect.push_back(ptr);
  }

  return services_vect;
}

void K8switch::addService(const std::string &vip, const uint16_t &vport,
                          const ServiceProtoEnum &proto,
                          const ServiceJsonObject &conf) {
  logger()->debug("Creating {}}:{}/{} service", vip, vport,
                  ServiceJsonObject::ServiceProtoEnum_to_string(proto));
  auto key = Service::Key(vip, vport, Service::convertProtoToNumber(proto));

  if (service_map_.count(key) != 0) {
    logger()->error("Service {}:{}/{} already exists", vip, vport,
                    ServiceJsonObject::ServiceProtoEnum_to_string(proto));
    throw std::runtime_error("Service exists");
  }

  std::unordered_map<Service::Key, Service>::iterator iter;
  bool inserted;
  std::tie(iter, inserted) =
      service_map_.emplace(std::piecewise_construct, std::forward_as_tuple(key),
                           std::forward_as_tuple(*this, conf));
  if (!inserted) {
    throw std::runtime_error("Unable to create the service instance");
  }
}

void K8switch::addServiceList(const std::vector<ServiceJsonObject> &conf) {
  for (auto &i : conf) {
    std::string vip_ = i.getVip();
    uint16_t vport_ = i.getVport();
    ServiceProtoEnum proto_ = i.getProto();
    addService(vip_, vport_, proto_, i);
  }
}

void K8switch::replaceService(const std::string &vip, const uint16_t &vport,
                              const ServiceProtoEnum &proto,
                              const ServiceJsonObject &conf) {
  delService(vip, vport, proto);
  addService(vip, vport, proto, conf);
}

void K8switch::delService(const std::string &vip, const uint16_t &vport,
                          const ServiceProtoEnum &proto) {
  logger()->debug("Removing service {}:{}/{}", vip, vport,
                  ServiceJsonObject::ServiceProtoEnum_to_string(proto));

  auto key = Service::Key(vip, vport, Service::convertProtoToNumber(proto));

  if (service_map_.count(key) == 0) {
    logger()->error("Service {}:{}/{} does not exist", vip, vport,
                    ServiceJsonObject::ServiceProtoEnum_to_string(proto));
    throw std::runtime_error("Service does not exist");
  }

  service_map_.erase(key);
}

void K8switch::delServiceList() {
  service_map_.clear();
}

std::shared_ptr<Ports> K8switch::getPorts(const std::string &name) {
  return get_port(name);
}

std::vector<std::shared_ptr<Ports>> K8switch::getPortsList() {
  return get_ports();
}

void K8switch::addPorts(const std::string &name, const PortsJsonObject &conf) {
  bool reload = false;

  try {
    switch (conf.getType()) {
    case PortsTypeEnum::NODEPORT:
      if (getNodePortPort() != nullptr) {
        logger()->warn("There is already a NODEPORT port");
        throw std::runtime_error("There is already a NODEPORT port");
      }
      reload = true;
      break;
    }
  } catch (std::runtime_error &e) {
    logger()->warn("Error when adding the port {0}", name);
    logger()->warn("Error message: {0}", e.what());
    throw;
  }

  add_port<PortsJsonObject>(name, conf);

  if (reload) {
    logger()->info("Nodeport added, reloading code");
    reloadConfig();
  }

  logger()->info("New port created with name {0}", name);
}

void K8switch::addPortsList(const std::vector<PortsJsonObject> &conf) {
  for (auto &i : conf) {
    std::string name_ = i.getName();
    addPorts(name_, i);
  }
}

void K8switch::replacePorts(const std::string &name,
                            const PortsJsonObject &conf) {
  delPorts(name);
  std::string name_ = conf.getName();
  addPorts(name_, conf);
}

void K8switch::delPorts(const std::string &name) {
  remove_port(name);
}

void K8switch::delPortsList() {
  auto ports = get_ports();
  for (auto it : ports) {
    delPorts(it->name());
  }
}

std::shared_ptr<FwdTable> K8switch::getFwdTable(const std::string &address) {
  uint32_t ip_key =
      FwdTable::get_index(address);  // takes last part of the IP address

  try {
    auto fwd_table = get_array_table<pod>("fwd_table");
    auto entry = fwd_table.get(ip_key);

    std::string mac = utils::be_uint_to_ip_string(entry.mac);
    std::string port(get_port(entry.port)->name());
    return std::make_shared<FwdTable>(FwdTable(*this, address, mac, port));
  } catch (std::exception &e) {
    logger()->error("Unable to find FWD table entry for address {0}. {1}",
                    address, e.what());
    throw std::runtime_error("FWD table entry not found");
  }
}

std::vector<std::shared_ptr<FwdTable>> K8switch::getFwdTableList() {
  logger()->debug("Getting the FWD table");

  std::vector<std::shared_ptr<FwdTable>> fwd_table_entries;

  // The FWD table is read from the data path
  try {
    auto fwd_table = get_array_table<pod>("fwd_table");
    auto entries = fwd_table.get_all();

    for (auto &entry : entries) {
      auto key = entry.first;
      auto value = entry.second;

      // TODO: key is only the index, it should be converted back to the full IP
      // address
      std::string ip = utils::be_uint_to_ip_string(key);
      std::string mac = utils::be_uint_to_mac_string(value.mac);
      std::string port(get_port(value.port)->name());

      fwd_table_entries.push_back(
          std::make_shared<FwdTable>(FwdTable(*this, ip, mac, port)));
    }
  } catch (std::exception &e) {
    logger()->error("Error while trying to get the FWD table");
    throw std::runtime_error("Unable to get the FWD table list");
  }

  return fwd_table_entries;
}

void K8switch::addFwdTable(const std::string &address,
                           const FwdTableJsonObject &conf) {
  logger()->debug("Creating fwd entry ip: {0} - mac: {1} - port: {2}", address,
                  conf.getMac(), conf.getPort());

  uint32_t ip_key =
      FwdTable::get_index(address);  // takes last part of the IP address
  auto port = get_port(conf.getPort());

  pod p{
      .mac = utils::mac_string_to_be_uint(conf.getMac()),
      .port = uint16_t(port->index()),
  };

  auto fwd_table = get_array_table<pod>("fwd_table");
  fwd_table.set(ip_key, p);
}

void K8switch::addFwdTableList(const std::vector<FwdTableJsonObject> &conf) {
  for (auto &i : conf) {
    std::string address_ = i.getAddress();
    addFwdTable(address_, i);
  }
}

void K8switch::replaceFwdTable(const std::string &address,
                               const FwdTableJsonObject &conf) {
  delFwdTable(address);
  std::string address_ = conf.getAddress();
  addFwdTable(address_, conf);
}

void K8switch::delFwdTable(const std::string &address) {
  logger()->debug("Remove the FWD table entry for address {0}", address);

  uint32_t ip_key =
      FwdTable::get_index(address);  // takes last part of the IP address

  // fwd_table is implemented as an array, deleting means writing 0s
  auto fwd_table = get_array_table<pod>("fwd_table");
  pod p{0};
  fwd_table.set(ip_key, p);
}

void K8switch::delFwdTableList() {
  throw std::runtime_error("FwdTable::remove non supported");
}