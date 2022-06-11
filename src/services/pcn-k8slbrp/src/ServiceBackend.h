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


#include "../base/ServiceBackendBase.h"


class Service;

using namespace polycube::service::model;

class ServiceBackend : public ServiceBackendBase {
 public:
  ServiceBackend(Service &parent, const ServiceBackendJsonObject &conf);
  virtual ~ServiceBackend();

    void update(const ServiceBackendJsonObject &conf) override;

  /// <summary>
  /// name
  /// </summary>
  std::string getName() override;
  void setName(const std::string &value) override;

  /// <summary>
  /// IP address of the backend server of the pool
  /// </summary>
  std::string getIp() override;

  /// <summary>
  /// Port where the server listen to (this value is ignored in case of ICMP)
  /// </summary>
  uint16_t getPort() override;
  void setPort(const uint16_t &value) override;

  /// <summary>
  /// Weight of the backend in the pool
  /// </summary>
  uint16_t getWeight() override;
  void setWeight(const uint16_t &value) override;


private:
    uint16_t weight_;
    uint16_t port_;
    std::string ip_;
    std::string name_;
};
