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


#include "../interface/ServiceInterface.h"


#include <spdlog/spdlog.h>
#include "ServiceBackend.h"

class Lbrp;

using namespace io::swagger::server::model;

/* definitions copied from datapath */
struct vip {
  uint32_t ip;
  uint16_t port;
  uint16_t proto;
  uint16_t index;  //refers to backend's index for that Service(vip)
} __attribute__((packed));

struct backend {
  uint32_t ip;
  uint16_t port;
  uint16_t proto;
} __attribute__((packed));

class Service : public ServiceInterface {
  friend class ServiceBackend;
public:
  Service(Lbrp &parent, const ServiceJsonObject &conf);
  virtual ~Service();

  static void create(Lbrp &parent, const std::string &vip, const uint16_t &vport, const ServiceProtoEnum &proto, const ServiceJsonObject &conf);
  static std::shared_ptr<Service> getEntry(Lbrp &parent, const std::string &vip, const uint16_t &vport, const ServiceProtoEnum &proto);
  static void removeEntry(Lbrp &parent, const std::string &vip, const uint16_t &vport, const ServiceProtoEnum &proto);
  static std::vector<std::shared_ptr<Service>> get(Lbrp &parent);
  static void remove(Lbrp &parent);
  nlohmann::fifo_map<std::string, std::string> getKeys();
  std::shared_ptr<spdlog::logger> logger();
  void update(const ServiceJsonObject &conf) override;
  ServiceJsonObject toJsonObject() override;

  /// <summary>
  /// Virtual IP (vip) of the service (where clients connect to)
  /// </summary>
  std::string getVip() override;

  /// <summary>
  /// Upper-layer protocol associated with a loadbalancing service instance
  /// </summary>
  ServiceProtoEnum getProto() override;

  /// <summary>
  /// Service name related to the backend server of the pool is connected to
  /// </summary>
  std::string getName() override;
  void setName(const std::string &value) override;

  /// <summary>
  /// Port of the virtual server (where clients connect to)
  /// </summary>
  uint16_t getVport() override;

  /// <summary>
  /// Pool of backend servers that actually serve requests
  /// </summary>
  std::shared_ptr<ServiceBackend> getBackend(const std::string &ip) override;
  std::vector<std::shared_ptr<ServiceBackend>> getBackendList() override;
  void addBackend(const std::string &ip, const ServiceBackendJsonObject &conf) override;
  void addBackendList(const std::vector<ServiceBackendJsonObject> &conf) override;
  void replaceBackend(const std::string &ip, const ServiceBackendJsonObject &conf) override;
  void delBackend(const std::string &ip) override;
  void delBackendList() override;

  typedef std::tuple<std::string, uint16_t, uint8_t> ServiceKey;

private:
  Lbrp &parent_;
  std::unordered_map<std::string, ServiceBackend> service_backends_;
  std::map<std::string, std::vector<int>> backend_matrix_;

  static const uint INITIAL_BACKEND_SIZE;
  static const uint BACKEND_REPLICAS;
  static const std::string EBPF_SERVICE_MAP;
  static const std::string EBPF_BACKEND_TO_SERVICE_MAP;
  static const uint16_t ICMP_EBPF_PORT;

  std::string service_name_;
  std::string vip_;
  uint16_t vport_;
  ServiceProtoEnum proto_;
  uint backend_size_;

  void updateConsistentHashMap();
  std::vector<std::string> getConsistentArray();
  void updateKernelServiceMap(const std::vector<std::string> consistent_array);
  void addBackendToServiceMatrix(std::string backend_ip);
  void removeBackendFromServiceMatrix(std::string backend_ip);
  static ServiceProtoEnum convertNumberToProto(const uint8_t proto);
  static uint8_t convertProtoToNumber(const ServiceProtoEnum &proto);
  std::vector<int> getRandomIntVector(int vect_size);
  std::vector<int> getEmptyIntVector(int vect_size);
  void removeServiceFromKernelMap();
  std::map<std::string,int> get_weight_backend();
};

