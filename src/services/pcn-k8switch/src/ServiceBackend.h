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

#include "../interface/ServiceBackendInterface.h"

#include <spdlog/spdlog.h>

class Service;

using namespace io::swagger::server::model;

class ServiceBackend : public ServiceBackendInterface {
 public:
  ServiceBackend(Service &parent, const ServiceBackendJsonObject &conf);
  virtual ~ServiceBackend();

  nlohmann::fifo_map<std::string, std::string> getKeys();
  std::shared_ptr<spdlog::logger> logger();
  void update(const ServiceBackendJsonObject &conf) override;
  ServiceBackendJsonObject toJsonObject() override;

  /// <summary>
  /// IP address of the backend server of the pool
  /// </summary>
  std::string getIp() override;

  /// <summary>
  /// name
  /// </summary>
  std::string getName() override;
  void setName(const std::string &value) override;

  /// <summary>
  /// Weight of the backend in the pool
  /// </summary>
  uint16_t getWeight() override;
  void setWeight(const uint16_t &value) override;

  /// <summary>
  /// Port where the actual server listen to
  /// </summary>
  uint16_t getPort() override;

  typedef std::pair<std::string, uint16_t> Key;

 private:
  Service &parent_;
  uint16_t weight_;
  uint16_t port_;
  std::string ip_;
  std::string name_;
};
