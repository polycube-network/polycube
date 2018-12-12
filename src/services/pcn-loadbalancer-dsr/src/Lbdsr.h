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


#include "../interface/LbdsrInterface.h"

#include "polycube/services/cube.h"
#include "polycube/services/port.h"
#include "polycube/services/utils.h"

#include <spdlog/spdlog.h>

#include "Backend.h"
#include "Frontend.h"
#include "Ports.h"


using namespace io::swagger::server::model;
using polycube::service::CubeType;

class Lbdsr : public polycube::service::Cube<Ports>, public LbdsrInterface {
  friend class Ports;
  friend class Frontend;
  friend class Backend;
public:
  Lbdsr(const std::string name, const LbdsrJsonObject &conf, CubeType type = CubeType::TC);
  virtual ~Lbdsr();
  std::string generate_code(bool first);
  std::vector<std::string> generate_code_vector();
  void packet_in(Ports &port, polycube::service::PacketInMetadata &md, const std::vector<uint8_t> &packet) override;

  void update(const LbdsrJsonObject &conf) override;
  LbdsrJsonObject toJsonObject() override;

  /// <summary>
  ///
  /// </summary>
  std::shared_ptr<Frontend> getFrontend() override;
  void addFrontend(const FrontendJsonObject &value) override;
  void replaceFrontend(const FrontendJsonObject &conf) override;
  void delFrontend() override;

  /// <summary>
  /// Name of the lbdsr service
  /// </summary>
  std::string getName() override;

  /// <summary>
  /// Defines the algorithm which LB use to direct requests to the node of the pool (Random, RoundRobin, ..)
  /// </summary>
  std::string getAlgorithm() override;
  void setAlgorithm(const std::string &value) override;

  /// <summary>
  /// Defines the logging level of a service instance, from none (OFF) to the most verbose (TRACE). Default: INFO
  /// </summary>
  LbdsrLoglevelEnum getLoglevel() override;
  void setLoglevel(const LbdsrLoglevelEnum &value) override;

  /// <summary>
  /// UUID of the Cube
  /// </summary>
  std::string getUuid() override;

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
  void replacePorts(const std::string &name, const PortsJsonObject &conf) override;
  void delPorts(const std::string &name) override;
  void delPortsList() override;

  /// <summary>
  ///
  /// </summary>
  std::shared_ptr<Backend> getBackend() override;
  void addBackend(const BackendJsonObject &value) override;
  void replaceBackend(const BackendJsonObject &conf) override;
  void delBackend() override;


void replaceAll(std::string &str, const std::string &from, const std::string &to);
std::string getCode(bool first);
bool reloadCode();

void rmPort(std::string portName);
void setFrontendPort(std::string portName);
void setBackendPort(std::string portName);
void rmFrontendPort(std::string portName);
void rmBackendPort(std::string portName);

void setVipHexbe(std::string &value);
void setMacHexbe(std::string &value);

private:
  std::shared_ptr<Backend> backend_ = nullptr;
  std::shared_ptr<Frontend> frontend_ = nullptr;
  std::string algorithm_;

  uint32_t frontend_port_ = -1;
  uint32_t backend_port_ = -1;

  std::string frontend_port_str_ = "";
  std::string backend_port_str_ = "";

  std::string vip_hexbe_str_ = "0x6400000a";
  std::string mac_hexbe_str_ = "0xccbbaa010101";

  std::string vip_hexbe_str_default_ = "0x6400000a";
  std::string mac_hexbe_str_default_ = "0xccbbaa010101";

  // when code is reloaded it will be set to false
  bool is_code_changed_ = false;
};
