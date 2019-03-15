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

#pragma once

#include "../interface/K8switchInterface.h"

#include "polycube/services/cube.h"
#include "polycube/services/port.h"
#include "polycube/services/utils.h"

#include <spdlog/spdlog.h>

#include "Ports.h"
#include "Service.h"
#include "hash_tuple.h"

/* definitions copied from dataplane */
struct cb_control {
  uint16_t reader;
  uint16_t writer;
  uint32_t size;
} __attribute__((packed));

struct nodeport_session_in {
  uint32_t ip_src;
  uint32_t ip_dst;
  uint16_t port_src;
  uint16_t port_dst;
  uint16_t proto;
  uint64_t timestamp;
} __attribute__((packed));

struct nodeport_session_out {
  uint64_t mac_dst;  // : 48;
  uint64_t mac_src;  // : 48;
  uint32_t ip_src;
  uint32_t ip_dst;
  uint16_t port_src;
  uint16_t port_dst;
  uint16_t proto;
  uint64_t timestamp;
} __attribute__((packed));

using namespace io::swagger::server::model;
using polycube::service::CubeType;

class K8switch : public polycube::service::Cube<Ports>,
                 public K8switchInterface {
  friend class Ports;
  friend class Service;

 public:
  K8switch(const std::string name, const K8switchJsonObject &conf);
  virtual ~K8switch();
  std::string generate_code();
  std::vector<std::string> generate_code_vector();
  void packet_in(Ports &port, polycube::service::PacketInMetadata &md,
                 const std::vector<uint8_t> &packet) override;

  void update(const K8switchJsonObject &conf) override;
  K8switchJsonObject toJsonObject() override;

  /// <summary>
  /// Services (i.e., virtual ip:port) exported to the client
  /// </summary>
  std::shared_ptr<Service> getService(const std::string &vip,
                                      const uint16_t &vport,
                                      const ServiceProtoEnum &proto) override;
  std::vector<std::shared_ptr<Service>> getServiceList() override;
  void addService(const std::string &vip, const uint16_t &vport,
                  const ServiceProtoEnum &proto,
                  const ServiceJsonObject &conf) override;
  void addServiceList(const std::vector<ServiceJsonObject> &conf) override;
  void replaceService(const std::string &vip, const uint16_t &vport,
                      const ServiceProtoEnum &proto,
                      const ServiceJsonObject &conf) override;
  void delService(const std::string &vip, const uint16_t &vport,
                  const ServiceProtoEnum &proto) override;
  void delServiceList() override;

  /// <summary>
  /// Range of VIPs where clusterIP services are exposed
  /// </summary>
  std::string getClusterIpSubnet() override;
  void setClusterIpSubnet(const std::string &value) override;
  void doSetClusterIpSubnet(const std::string &value);

  /// <summary>
  /// Range of IPs of pods in this node
  /// </summary>
  std::string getClientSubnet() override;
  void setClientSubnet(const std::string &value) override;
  void doSetClientSubnet(const std::string &value);

  /// <summary>
  /// Range where client&#39;s IPs are mapped into
  /// </summary>
  std::string getVirtualClientSubnet() override;
  void setVirtualClientSubnet(const std::string &value) override;
  void doSetVirtualClientSubnet(const std::string &value);

  /// <summary>
  /// Entry of the ports table
  /// </summary>
  std::shared_ptr<Ports> getPorts(const std::string &name) override;
  std::vector<std::shared_ptr<Ports>> getPortsList() override;
  void addPorts(const std::string &name, const PortsJsonObject &conf) override;
  void addPortsList(const std::vector<PortsJsonObject> &conf) override;
  void replacePorts(const std::string &name,
                    const PortsJsonObject &conf) override;
  void delPorts(const std::string &name) override;
  void delPortsList() override;

  /// <summary>
  /// Entry associated with the forwarding table
  /// </summary>
  std::shared_ptr<FwdTable> getFwdTable(const std::string &address) override;
  std::vector<std::shared_ptr<FwdTable>> getFwdTableList() override;
  void addFwdTable(const std::string &address,
                   const FwdTableJsonObject &conf) override;
  void addFwdTableList(const std::vector<FwdTableJsonObject> &conf) override;
  void replaceFwdTable(const std::string &address,
                       const FwdTableJsonObject &conf) override;
  void delFwdTable(const std::string &address) override;
  void delFwdTableList() override;

  void reloadConfig();
  std::shared_ptr<Ports> getNodePortPort();

 private:
  void cleanupSessionTable();
  uint32_t timestampToAge(const uint64_t timestamp);
  void tick();

  std::unique_ptr<std::thread> tick_thread;
  bool stop_;

  const uint32_t sessionTableTimeout = 300;

  uint32_t ip_to_dec(const std::string &ip);
  void parse_cidr(const std::string &cidr, uint32_t *subnet, uint32_t *netmask);

  std::unordered_map<Service::Key, Service> service_map_;

  std::string cluster_ip_cidr_;
  std::string client_cidr_;
  std::string virtual_client_cidr_;

  uint32_t client_subnet_;
  uint32_t client_mask_;

  uint32_t virtual_client_subnet_;
  uint32_t virtual_client_mask_;

  uint32_t cluster_ip_subnet_;
  uint32_t cluster_ip_mask_;

  unsigned int ncpus;
  unsigned int buf_size;
};
