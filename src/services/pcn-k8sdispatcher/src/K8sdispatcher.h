/*
 * Copyright 2022 The Polycube Authors
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

#include "base/K8sdispatcherBase.h"
#include "SessionRule.h"
#include "NodeportRule.h"
#include "Ports.h"
#include "HashTuple.h"

/* definitions copied from datapath */
struct nt_k {
  uint16_t port;
  uint8_t proto;
} __attribute__((packed));
struct nt_v {
  uint16_t external_traffic_policy;
} __attribute__((packed));

enum class SlowPathReason { ARP_REPLY = 0 };

using namespace polycube::service::model;

class K8sdispatcher : public K8sdispatcherBase {
 public:
  K8sdispatcher(const std::string name, const K8sdispatcherJsonObject &conf);
  virtual ~K8sdispatcher();

  void packet_in(Ports &port, polycube::service::PacketInMetadata &md,
                 const std::vector<uint8_t> &packet) override;

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
  /// Internal source IP address used for natting incoming packets directed to
  /// Kubernetes Services with a CLUSTER external traffic policy
  /// </summary>
  std::string getInternalSrcIp() override;

  /// <summary>
  /// Port range used for NodePort Services
  /// </summary>
  std::string getNodeportRange() override;
  void setNodeportRange(const std::string &value) override;

  /// <summary>
  /// Session entry related to a specific traffic direction
  /// </summary>
  std::shared_ptr<SessionRule> getSessionRule(
      const SessionRuleDirectionEnum &direction, const std::string &srcIp,
      const std::string &dstIp, const uint16_t &srcPort,
      const uint16_t &dstPort, const L4ProtoEnum &proto) override;
  std::vector<std::shared_ptr<SessionRule>> getSessionRuleList() override;
  void addSessionRule(const SessionRuleDirectionEnum &direction,
                      const std::string &srcIp, const std::string &dstIp,
                      const uint16_t &srcPort, const uint16_t &dstPort,
                      const L4ProtoEnum &proto,
                      const SessionRuleJsonObject &conf) override;
  void addSessionRuleList(
      const std::vector<SessionRuleJsonObject> &conf) override;
  void replaceSessionRule(const SessionRuleDirectionEnum &direction,
                          const std::string &srcIp, const std::string &dstIp,
                          const uint16_t &srcPort, const uint16_t &dstPort,
                          const L4ProtoEnum &proto,
                          const SessionRuleJsonObject &conf) override;
  void delSessionRule(const SessionRuleDirectionEnum &direction,
                      const std::string &srcIp, const std::string &dstIp,
                      const uint16_t &srcPort, const uint16_t &dstPort,
                      const L4ProtoEnum &proto) override;
  void delSessionRuleList() override;

  /// <summary>
  /// NodePort rule associated with a Kubernetes NodePort Service
  /// </summary>
  std::shared_ptr<NodeportRule> getNodeportRule(
      const uint16_t &nodeportPort, const L4ProtoEnum &proto) override;
  std::vector<std::shared_ptr<NodeportRule>> getNodeportRuleList() override;
  void addNodeportRule(const uint16_t &nodeportPort, const L4ProtoEnum &proto,
                       const NodeportRuleJsonObject &conf) override;
  void addNodeportRuleList(
      const std::vector<NodeportRuleJsonObject> &conf) override;
  void updateNodeportRuleList(
      const std::vector<NodeportRuleJsonObject> &conf) override;
  void replaceNodeportRule(const uint16_t &nodeportPort,
                           const L4ProtoEnum &proto,
                           const NodeportRuleJsonObject &conf) override;
  virtual void replaceNodeportRuleList(
      const std::vector<NodeportRuleJsonObject> &conf) override;
  void delNodeportRule(const uint16_t &nodeportPort,
                       const L4ProtoEnum &proto) override;
  void delNodeportRuleList() override;

  static const std::string EBPF_EGRESS_SESSION_TABLE;
  static const std::string EBPF_INGRESS_SESSION_TABLE;
  static const std::string EBPF_NPR_TABLE_MAP;

  typedef std::tuple<uint16_t, L4ProtoEnum> NodeportKey;

 private:
  std::string internalSrcIp_;
  uint32_t internalSrcIpNboInt_;
  std::string nodeportRange_;
  std::pair<uint16_t, uint16_t> nodeportRangeTuple_;

  uint32_t nodeIpNboInt_;

  std::unordered_map<NodeportKey, std::shared_ptr<NodeportRule>>
      nodePortRuleMap_;

  void reloadConfig();

  std::shared_ptr<Ports> getFrontendPort();

  std::shared_ptr<Ports> getBackendPort();
};
