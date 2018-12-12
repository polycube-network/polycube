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


#include "../interface/PortsInterface.h"

#include "polycube/services/cube.h"
#include "polycube/services/port.h"
#include "polycube/services/utils.h"

#include <spdlog/spdlog.h>


class K8switch;

using namespace io::swagger::server::model;

class Ports : public polycube::service::Port, public PortsInterface {
public:
  Ports(polycube::service::Cube<Ports> &parent,
        std::shared_ptr<polycube::service::PortIface> port,
        const PortsJsonObject &conf);
  virtual ~Ports();

  static void create(K8switch &parent, const std::string &name, const PortsJsonObject &conf);
  static std::shared_ptr<Ports> getEntry(K8switch &parent, const std::string &name);
  static void removeEntry(K8switch &parent, const std::string &name);
  static std::vector<std::shared_ptr<Ports>> get(K8switch &parent);
  static void remove(K8switch &parent);
  nlohmann::fifo_map<std::string, std::string> getKeys();
  std::shared_ptr<spdlog::logger> logger();
  void update(const PortsJsonObject &conf) override;
  PortsJsonObject toJsonObject() override;

  /// <summary>
  /// Status of the port (UP or DOWN)
  /// </summary>
  PortsStatusEnum getStatus() override;

  /// <summary>
  /// Peer name, such as a network interfaces (e.g., &#39;veth0&#39;) or another cube (e.g., &#39;br1:port2&#39;)
  /// </summary>
  std::string getPeer() override;
  void setPeer(const std::string &value) override;

  /// <summary>
  /// Type of the LB port (e.g. FRONTEND or BACKEND)
  /// </summary>
  PortsTypeEnum getType() override;
  void setType(const PortsTypeEnum &value) override;

  /// <summary>
  /// Port Name
  /// </summary>
  std::string getName() override;

  /// <summary>
  /// UUID of the port
  /// </summary>
  std::string getUuid() override;

private:
  K8switch &parent_;
  PortsTypeEnum port_type_;

};

