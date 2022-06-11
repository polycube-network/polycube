/*
 * Copyright 2021 Leonardo Di Giovanna
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


#include "../base/K8slbrpBase.h"

#include "Ports.h"
#include "Service.h"
#include "SrcIpRewrite.h"
#include "hash_tuple.h"
#include <set>
#include <mutex>


using namespace polycube::service::model;

enum class SlowPathReason { ARP_REPLY = 0, FLOODING = 1 };

class K8slbrp : public K8slbrpBase {
    friend class Ports;
    friend class Service;
    friend class SrcIpRewrite;

 public:
  static std::string buildK8sLbrpCode(std::string const& k8slbrp_code, K8slbrpPortModeEnum port_mode);
  K8slbrp(const std::string name, const K8slbrpJsonObject &conf);
  virtual ~K8slbrp();

  void flood_packet(Ports &port,
                    polycube::service::PacketInMetadata &md,
                    const std::vector<uint8_t> &packet);
  void packet_in(Ports &port,
                 polycube::service::PacketInMetadata &md,
                 const std::vector<uint8_t> &packet) override;

  /// <summary>
  /// Entry of the ports table
  /// </summary>
  std::shared_ptr<Ports> getPorts(const std::string &name) override;
  std::vector<std::shared_ptr<Ports>> getPortsList() override;
  void addPortsInSinglePortMode(const std::string &name, const PortsJsonObject &conf);
  void addPortsInMultiPortMode(const std::string &name, const PortsJsonObject &conf);
  void addPorts(const std::string &name, const PortsJsonObject &conf) override;
  void addPortsList(const std::vector<PortsJsonObject> &conf) override;
  void replacePorts(const std::string &name, const PortsJsonObject &conf) override;
  void delPorts(const std::string &name) override;
  void delPortsList() override;

  /// <summary>
  /// K8s lbrp mode of operation. 'MULTI' allows to manage multiple FRONTEND port. 'SINGLE' is optimized for working with a single FRONTEND port
  /// </summary>
  K8slbrpPortModeEnum getPortMode() override;
  void setPortMode(const K8slbrpPortModeEnum &value) override;

  /// <summary>
  /// If configured, when a client request arrives to the LB, the source IP address is replaced with another IP address from the &#39;new&#39; range
  /// </summary>
  std::shared_ptr<SrcIpRewrite> getSrcIpRewrite() override;
  void addSrcIpRewrite(const SrcIpRewriteJsonObject &value) override;
  void replaceSrcIpRewrite(const SrcIpRewriteJsonObject &conf) override;
  void delSrcIpRewrite() override;

  /// <summary>
  /// Services (i.e., virtual ip:protocol:port) exported to the client
  /// </summary>
  std::shared_ptr<Service> getService(const std::string &vip, const uint16_t &vport, const ServiceProtoEnum &proto) override;
  std::vector<std::shared_ptr<Service>> getServiceList() override;
  void addService(const std::string &vip, const uint16_t &vport, const ServiceProtoEnum &proto, const ServiceJsonObject &conf) override;
  void addServiceList(const std::vector<ServiceJsonObject> &conf) override;
  void replaceService(const std::string &vip, const uint16_t &vport, const ServiceProtoEnum &proto, const ServiceJsonObject &conf) override;
  void delService(const std::string &vip, const uint16_t &vport, const ServiceProtoEnum &proto) override;
  void delServiceList() override;

  void reloadCodeWithNewPorts();
  void reloadCodeWithNewBackendPort(uint16_t backend_port_index);
  std::vector<std::shared_ptr<Ports>> getFrontendPorts();
  std::shared_ptr<Ports> getBackendPort();
private:
  static const std::string EBPF_IP_TO_FRONTEND_PORT_MAP;
  std::set<std::string> frontend_ip_set_;
  std::unordered_map<Service::ServiceKey, Service> service_map_;
  std::shared_ptr<SrcIpRewrite> src_ip_rewrite_;
  K8slbrpPortModeEnum port_mode_;
  std::string k8slbrp_code_;
};
