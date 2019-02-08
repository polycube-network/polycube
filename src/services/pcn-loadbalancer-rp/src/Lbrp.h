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

#include "../interface/LbrpInterface.h"

#include "polycube/services/cube.h"
#include "polycube/services/port.h"
#include "polycube/services/utils.h"

#include <spdlog/spdlog.h>

#include "Ports.h"
#include "Service.h"
#include "SrcIpRewrite.h"
#include "hash_tuple.h"

using namespace io::swagger::server::model;
using polycube::service::CubeType;

class Lbrp : public polycube::service::Cube<Ports>, public LbrpInterface {
  friend class Ports;
  friend class Service;
  friend class SrcIpRewrite;

 public:
  Lbrp(const std::string name, const LbrpJsonObject &conf,
       CubeType type = CubeType::TC);
  virtual ~Lbrp();
  std::string generate_code();
  std::vector<std::string> generate_code_vector();
  void packet_in(Ports &port, polycube::service::PacketInMetadata &md,
                 const std::vector<uint8_t> &packet) override;

  void update(const LbrpJsonObject &conf) override;
  LbrpJsonObject toJsonObject() override;

  /// <summary>
  /// Name of the lbrp service
  /// </summary>
  std::string getName() override;

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
  /// Defines the logging level of a service instance, from none (OFF) to the
  /// most verbose (TRACE). Default: OFF
  /// </summary>
  LbrpLoglevelEnum getLoglevel() override;
  void setLoglevel(const LbrpLoglevelEnum &value) override;

  /// <summary>
  ///
  /// </summary>
  std::shared_ptr<SrcIpRewrite> getSrcIpRewrite() override;
  void addSrcIpRewrite(const SrcIpRewriteJsonObject &value) override;
  void replaceSrcIpRewrite(const SrcIpRewriteJsonObject &conf) override;
  void delSrcIpRewrite() override;

  /// <summary>
  /// Type of the Cube (TC, XDP_SKB, XDP_DRV)
  /// </summary>
  CubeType getType() override;

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
  /// UUID of the Cube
  /// </summary>
  std::string getUuid() override;

  void reloadCodeWithNewPorts();
  std::shared_ptr<Ports> getFrontendPort();
  std::shared_ptr<Ports> getBackendPort();

 private:
  std::unordered_map<Service::ServiceKey, Service> service_map_;
  std::shared_ptr<SrcIpRewrite> src_ip_rewrite_;
};
