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


#include "../interface/NatInterface.h"

#include "polycube/services/cube.h"
#include "polycube/services/port.h"
#include "polycube/services/utils.h"

#include <spdlog/spdlog.h>

#include "NattingTable.h"
#include "Ports.h"
#include "Rule.h"

using namespace io::swagger::server::model;
using polycube::service::CubeType;

/* definitions copied from datapath */
struct dp_k {
  uint32_t mask;
  uint32_t external_ip;
  uint16_t external_port;
  uint8_t proto;
} __attribute__((packed));

struct dp_v {
  uint32_t internal_ip;
  uint16_t internal_port;
  uint8_t entry_type;
} __attribute__((packed));

struct sm_k {
  uint32_t internal_netmask_len;
  uint32_t internal_ip;
} __attribute__((packed));

struct sm_v {
  uint32_t external_ip;
  uint8_t entry_type;
} __attribute__((packed));

class Nat : public polycube::service::Cube<Ports>, public NatInterface {
  friend class Ports;
  friend class Rule;
 public:
  Nat(const std::string name, const NatJsonObject &conf, CubeType type = CubeType::TC);
  virtual ~Nat();
  std::string generate_code();
  std::vector<std::string> generate_code_vector();
  void packet_in(Ports &port, polycube::service::PacketInMetadata &md, const std::vector<uint8_t> &packet) override;

  void update(const NatJsonObject &conf) override;
  NatJsonObject toJsonObject() override;

  /// <summary>
  /// Name of the nat service
  /// </summary>
  std::string getName() override;

  /// <summary>
  /// UUID of the Cube
  /// </summary>
  std::string getUuid() override;

  /// <summary>
  /// Type of the Cube (TC, XDP_SKB, XDP_DRV)
  /// </summary>
  CubeType getType() override;

  /// <summary>
  /// Defines the logging level of a service instance, from none (OFF) to the most verbose (TRACE)
  /// </summary>
  NatLoglevelEnum getLoglevel() override;
  void setLoglevel(const NatLoglevelEnum &value) override;

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
  std::shared_ptr<Rule> getRule() override;
  void addRule(const RuleJsonObject &value) override;
  void replaceRule(const RuleJsonObject &conf) override;
  void delRule() override;

  /// <summary>
  ///
  /// </summary>
  std::shared_ptr<NattingTable> getNattingTable(const std::string &internalSrc, const std::string &internalDst, const uint16_t &internalSport, const uint16_t &internalDport, const std::string &proto) override;
  std::vector<std::shared_ptr<NattingTable>> getNattingTableList() override;
  void addNattingTable(const std::string &internalSrc, const std::string &internalDst, const uint16_t &internalSport, const uint16_t &internalDport, const std::string &proto, const NattingTableJsonObject &conf) override;
  void addNattingTableList(const std::vector<NattingTableJsonObject> &conf) override;
  void replaceNattingTable(const std::string &internalSrc, const std::string &internalDst, const uint16_t &internalSport, const uint16_t &internalDport, const std::string &proto, const NattingTableJsonObject &conf) override;
  void delNattingTable(const std::string &internalSrc, const std::string &internalDst, const uint16_t &internalSport, const uint16_t &internalDport, const std::string &proto) override;
  void delNattingTableList() override;

  std::shared_ptr<Ports> getExternalPort();
  std::shared_ptr<Ports> getInternalPort();

  std::string getExternalIpString();

  std::string proto_from_int_to_string(const uint8_t proto);
  uint8_t proto_from_string_to_int(const std::string &proto);

private:
  std::shared_ptr<Rule> rule_;

  uint16_t internal_port_index_;
  uint16_t external_port_index_;

  std::string external_ip_;

  void reloadCode();
};

