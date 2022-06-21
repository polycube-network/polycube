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

#include "K8sdispatcher.h"

#include <tins/ethernetII.h>
#include <tins/tins.h>

#include "K8sdispatcher_dp.h"
#include "Utils.h"

using namespace Tins;
namespace poly_utils = polycube::service::utils;

const std::string K8sdispatcher::EBPF_EGRESS_SESSION_TABLE =
    "egress_session_table";
const std::string K8sdispatcher::EBPF_INGRESS_SESSION_TABLE =
    "ingress_session_table";
const std::string K8sdispatcher::EBPF_NPR_TABLE_MAP = "npr_table";

K8sdispatcher::K8sdispatcher(const std::string name,
                             const K8sdispatcherJsonObject &conf)
    : Cube(conf.getBase(), {k8sdispatcher_code}, {}),
      K8sdispatcherBase(name),
      internalSrcIp_{conf.getInternalSrcIp()},
      internalSrcIpNboInt_{poly_utils::ip_string_to_nbo_uint(internalSrcIp_)},
      nodeportRange_{"30000-32767"},
      nodeportRangeTuple_{30000, 32767},
      nodeIpNboInt_{0} {
  logger()->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [K8sDispatcher] [%n] [%l] %v");
  logger()->info("Creating K8sDispatcher instance");

  if (conf.nodeportRangeIsSet()) {
    this->setNodeportRange(conf.getNodeportRange());
  }
  this->addNodeportRuleList(conf.getNodeportRule());
  this->addPortsList(conf.getPorts());

  logger()->trace("Created K8sDispatcher instance");
}

K8sdispatcher::~K8sdispatcher() {
  logger()->info("Destroying K8sDispatcher instance");
}

void K8sdispatcher::packet_in(Ports &port,
                              polycube::service::PacketInMetadata &md,
                              const std::vector<uint8_t> &packet) {
  try {
    switch (static_cast<SlowPathReason>(md.reason)) {
    case SlowPathReason::ARP_REPLY: {
      EthernetII pkt(&packet[0], packet.size());
      auto backendPort = this->getBackendPort();
      if (backendPort != nullptr) {
        backendPort->send_packet_out(pkt);
      }
      break;
    }
    default: {
      logger()->error("Not valid slow path reason {} received", md.reason);
    }
    }
  } catch (const std::exception &e) {
    logger()->error("Exception during slow path packet processing: {}",
                    e.what());
  }
}

std::shared_ptr<Ports> K8sdispatcher::getPorts(const std::string &name) {
  return this->get_port(name);
}

std::vector<std::shared_ptr<Ports>> K8sdispatcher::getPortsList() {
  return this->get_ports();
}

void K8sdispatcher::addPorts(const std::string &name,
                             const PortsJsonObject &conf) {
  try {
    PortsTypeEnum type = conf.getType();

    // consistency check
    if (get_ports().size() == 2) {
      throw std::runtime_error("Reached maximum number of ports");
    }
    if (type == PortsTypeEnum::FRONTEND) {
      if (this->getFrontendPort() != nullptr) {
        throw std::runtime_error("There is already a FRONTEND port");
      }
    } else {
      if (this->getBackendPort() != nullptr) {
        throw std::runtime_error("There is already a BACKEND port");
      }
    }

    add_port<PortsJsonObject>(name, conf);

    // save node IP if port_type == FRONTEND
    if (type == PortsTypeEnum::FRONTEND) {
      this->nodeIpNboInt_ = poly_utils::ip_string_to_nbo_uint(conf.getIp());
    }
    // reload service dataplane if the configuration is complete
    if (get_ports().size() == 2) {
      this->reloadConfig();
    }
  } catch (std::runtime_error &ex) {
    logger()->error("Failed to add port {0}: {1}", name, ex.what());
    throw std::runtime_error("Failed to add port");
  }
}

void K8sdispatcher::addPortsList(const std::vector<PortsJsonObject> &conf) {
  for (auto &i : conf) {
    std::string name_ = i.getName();
    this->addPorts(name_, i);
  }
}

void K8sdispatcher::replacePorts(const std::string &name,
                                 const PortsJsonObject &conf) {
  logger()->error("K8sdispatcher::delPorts: Method not allowed");
  throw std::runtime_error{"Method not allowed"};
}

void K8sdispatcher::delPorts(const std::string &name) {
  logger()->error("K8sdispatcher::delPorts: Method not allowed");
  throw std::runtime_error{"Method not allowed"};
}

void K8sdispatcher::delPortsList() {
  logger()->error("K8sdispatcher::delPorts: Method not allowed");
  throw std::runtime_error{"Method not allowed"};
}

std::string K8sdispatcher::getInternalSrcIp() {
  return this->internalSrcIp_;
}

std::string K8sdispatcher::getNodeportRange() {
  return this->nodeportRange_;
}

void K8sdispatcher::setNodeportRange(const std::string &value) {
  // Return immediately if the provided NodePort port range is the same of the
  // already configured one
  if (this->nodeportRange_ == value) {
    return;
  }

  // Parsing lower and upper port numbers
  uint16_t low;
  uint16_t high;
  int res = std::sscanf(value.c_str(), "%hu-%hu", &low, &high);
  if (res != 2) {
    logger()->error("Failed to parse {} NodePort port range", value);
    throw std::runtime_error("Failed to parse NodePort port range");
  }

  if (low >= high) {
    logger()->error("Invalid {} NodePort port range", value);
    throw std::runtime_error("Invalid NodePort port range");
  }

  try {
    auto npr_table = get_hash_table<struct nt_k, struct nt_v>(
        K8sdispatcher::EBPF_NPR_TABLE_MAP);

    for (auto const &rule : this->getNodeportRuleList()) {
      uint16_t port = rule->getNodeportPort();
      if (port < low || port > high) {
        L4ProtoEnum proto = rule->getProto();
        nt_k npr_key{.port = htons(port),
                     .proto = utils::L4ProtoEnum_to_int(proto)};
        npr_table.remove(npr_key);

        if (this->nodePortRuleMap_.erase(NodeportKey(port, proto)) != 1) {
          std::runtime_error{"Failed to delete NodePort rule from user map"};
        }
      }
    }
  } catch (std::runtime_error &ex) {
    logger()->error("Failed to flush out-of-range NodePort rules: {}",
                    ex.what());
    throw std::runtime_error("Failed to flush out-of-range NodePort rules");
  }

  this->nodeportRangeTuple_ = std::make_pair(low, high);
  this->nodeportRange_ = value;
}

std::shared_ptr<SessionRule> K8sdispatcher::getSessionRule(
    const SessionRuleDirectionEnum &direction, const std::string &srcIp,
    const std::string &dstIp, const uint16_t &srcPort, const uint16_t &dstPort,
    const L4ProtoEnum &proto) {
  try {
    auto table = get_hash_table<st_k, st_v>(
        direction == SessionRuleDirectionEnum::EGRESS
            ? K8sdispatcher::EBPF_EGRESS_SESSION_TABLE
            : K8sdispatcher::EBPF_INGRESS_SESSION_TABLE);
    st_k map_key{.src_ip = poly_utils::ip_string_to_nbo_uint(srcIp),
                 .dst_ip = poly_utils::ip_string_to_nbo_uint(dstIp),
                 .src_port = htons(srcPort),
                 .dst_port = htons(dstPort),
                 .proto = utils::L4ProtoEnum_to_int(proto)};

    st_v value = table.get(map_key);

    std::string newIp = poly_utils::nbo_uint_to_ip_string(value.new_ip);
    uint16_t newPort = ntohs(value.new_port);
    SessionRuleOperationEnum operation =
        utils::int_to_SessionRuleOperationEnum(value.operation);
    SessionRuleOriginatingRuleEnum originatingRule =
        utils::int_to_SessionRuleOriginatingRuleEnum(value.originating_rule);

    return std::make_shared<SessionRule>(*this, direction, srcIp, dstIp,
                                         srcPort, dstPort, proto, newIp,
                                         newPort, operation, originatingRule);
  } catch (std::exception &ex) {
    logger()->error("Failed to get session rule: {}", ex.what());
    throw std::runtime_error("Failed to get session rule");
  }
}

std::vector<std::shared_ptr<SessionRule>> K8sdispatcher::getSessionRuleList() {
  std::vector<std::shared_ptr<SessionRule>> rules;
  std::vector<std::string> tableNames{
      K8sdispatcher::EBPF_EGRESS_SESSION_TABLE,
      K8sdispatcher::EBPF_INGRESS_SESSION_TABLE};
  try {
    for (auto &tableName : tableNames) {
      auto table = get_hash_table<struct st_k, struct st_v>(tableName);
      auto direction = tableName == K8sdispatcher::EBPF_INGRESS_SESSION_TABLE
                           ? SessionRuleDirectionEnum::INGRESS
                           : SessionRuleDirectionEnum::EGRESS;
      for (auto &entry : table.get_all()) {
        auto key = entry.first;
        auto value = entry.second;

        auto rule = std::make_shared<SessionRule>(
            *this, direction, poly_utils::nbo_uint_to_ip_string(key.src_ip),
            poly_utils::nbo_uint_to_ip_string(key.dst_ip), ntohs(key.src_port),
            ntohs(key.dst_port), utils::int_to_L4ProtoEnum(key.proto),
            poly_utils::nbo_uint_to_ip_string(value.new_ip),
            ntohs(value.new_port),
            utils::int_to_SessionRuleOperationEnum(value.operation),
            utils::int_to_SessionRuleOriginatingRuleEnum(
                value.originating_rule));

        rules.push_back(rule);
      }
    }
  } catch (std::exception &ex) {
    logger()->error("Failed to get session rules list: {0}", ex.what());
    throw std::runtime_error("Failed to get session rules list");
  }
  return rules;
}

void K8sdispatcher::addSessionRule(
    const SessionRuleDirectionEnum &direction, const std::string &srcIp,
    const std::string &dstIp, const uint16_t &srcPort, const uint16_t &dstPort,
    const L4ProtoEnum &proto, const SessionRuleJsonObject &conf) {
  logger()->error("K8sdispatcher::addSessionRule: Method not allowed");
  throw std::runtime_error{"Method not allowed"};
}

void K8sdispatcher::addSessionRuleList(
    const std::vector<SessionRuleJsonObject> &conf) {
  logger()->error("K8sdispatcher::addSessionRuleList: Method not allowed");
  throw std::runtime_error{"Method not allowed"};
}

void K8sdispatcher::replaceSessionRule(
    const SessionRuleDirectionEnum &direction, const std::string &srcIp,
    const std::string &dstIp, const uint16_t &srcPort, const uint16_t &dstPort,
    const L4ProtoEnum &proto, const SessionRuleJsonObject &conf) {
  logger()->error("K8sdispatcher::replaceSessionRule: Method not allowed");
  throw std::runtime_error{"Method not allowed"};
}

void K8sdispatcher::delSessionRule(const SessionRuleDirectionEnum &direction,
                                   const std::string &srcIp,
                                   const std::string &dstIp,
                                   const uint16_t &srcPort,
                                   const uint16_t &dstPort,
                                   const L4ProtoEnum &proto) {
  logger()->error("K8sdispatcher::delSessionRule: Method not allowed");
  throw std::runtime_error{"Method not allowed"};
}

void K8sdispatcher::delSessionRuleList() {
  logger()->error("K8sdispatcher::delSessionRuleList: Method not allowed");
  throw std::runtime_error{"Method not allowed"};
}

std::shared_ptr<NodeportRule> K8sdispatcher::getNodeportRule(
    const uint16_t &nodeportPort, const L4ProtoEnum &proto) {
  NodeportKey key = NodeportKey(nodeportPort, proto);
  auto it = nodePortRuleMap_.find(key);
  if (it == nodePortRuleMap_.end()) {
    logger()->info("No NodePort rule associated with key ({0}, {1})",
                   nodeportPort,
                   NodeportRuleJsonObject::L4ProtoEnum_to_string(proto));
    std::runtime_error{"No NodePort rule associated with the provided key"};
  }
  return it->second;
}

std::vector<std::shared_ptr<NodeportRule>>
K8sdispatcher::getNodeportRuleList() {
  std::vector<std::shared_ptr<NodeportRule>> rules;
  for (auto &entry : nodePortRuleMap_) {
    rules.push_back(entry.second);
  }
  return rules;
}

void K8sdispatcher::addNodeportRule(const uint16_t &nodeportPort,
                                    const L4ProtoEnum &proto,
                                    const NodeportRuleJsonObject &conf) {
  logger()->trace("Received a request to add a NodePort rule");
  NodeportKey key = NodeportKey(nodeportPort, proto);

  if (nodeportPort < this->nodeportRangeTuple_.first ||
      nodeportPort > this->nodeportRangeTuple_.second) {
    logger()->error(
        "The NodePort rule port is not contained in the valid range");
    throw std::runtime_error(
        "The NodePort rule port is not contained in the valid range");
  }

  if (this->nodePortRuleMap_.count(key)) {
    logger()->error("The NodePort rule already exists");
    throw std::runtime_error("The NodePort rule already exists");
  }

  auto rule = std::make_shared<NodeportRule>(*this, conf);
  if (!nodePortRuleMap_.insert(std::make_pair(key, rule)).second) {
    logger()->error("Failed to add the NodePort rule");
    throw std::runtime_error("Failed to add the NodePort rule");
  }

  try {
    logger()->trace("Retrieving NodePort rules kernel map");
    auto npr_table =
        get_hash_table<nt_k, nt_v>(K8sdispatcher::EBPF_NPR_TABLE_MAP);
    logger()->trace("Retrieved NodePort rules kernel map");
    nt_k npr_key{
        .port = htons(nodeportPort),
        .proto = utils::L4ProtoEnum_to_int(proto),
    };
    nt_v npr_value{.external_traffic_policy =
                       utils::NodeportRuleExternalTrafficPolicyEnum_to_int(
                           conf.getExternalTrafficPolicy())};

    logger()->trace("Storing NodePort rule in NodePort rules kernel map");
    npr_table.set(npr_key, npr_value);
    logger()->trace("Stored NodePort rule in NodePort rules kernel map");
  } catch (std::exception &ex) {
    logger()->warn("Failed to store NodePort rule in kernel map: {}",
                   ex.what());
    if (this->nodePortRuleMap_.erase(key) != 1) {
      logger()->error("Broken user space NodePort rule map");
    };
    throw std::runtime_error("Failed to store NodePort rule in kernel map");
  }
  logger()->info("Added NodePort rule");
}

void K8sdispatcher::addNodeportRuleList(
    const std::vector<NodeportRuleJsonObject> &conf) {
  for (auto &i : conf) {
    this->addNodeportRule(i.getNodeportPort(), i.getProto(), i);
  }
}

void K8sdispatcher::updateNodeportRuleList(
    const std::vector<NodeportRuleJsonObject> &conf) {
  logger()->trace("Received request for NodePort rules updating");
  try {
    for (auto &i : conf) {
      auto nodeportRule = getNodeportRule(i.getNodeportPort(), i.getProto());
      nodeportRule->update(i);
    }
  } catch (std::exception &ex) {
    logger()->error("Failed update NodePort rule: {}", ex.what());
    throw std::runtime_error("Failed to update NodePort rule");
  }
  logger()->info("Updated NodePort rules");
}

void K8sdispatcher::replaceNodeportRule(const uint16_t &nodeportPort,
                                        const L4ProtoEnum &proto,
                                        const NodeportRuleJsonObject &conf) {
  this->delNodeportRule(nodeportPort, proto);
  this->addNodeportRule(nodeportPort, proto, conf);
}

void K8sdispatcher::replaceNodeportRuleList(
    const std::vector<NodeportRuleJsonObject> &conf) {
  this->delNodeportRuleList();
  this->addNodeportRuleList(conf);
}

void K8sdispatcher::delNodeportRule(const uint16_t &nodeportPort,
                                    const L4ProtoEnum &proto) {
  logger()->trace("Received a request to delete a NodePort rule");
  NodeportKey key = NodeportKey(nodeportPort, proto);

  if (!this->nodePortRuleMap_.count(key)) {
    logger()->info("No NodePort rule associated with key ({0}, {1})",
                   nodeportPort,
                   NodeportRuleJsonObject::L4ProtoEnum_to_string(proto));
    std::runtime_error{"No NodePort rule associated with the provided key"};
  }

  try {
    logger()->trace("Retrieving NodePort rules kernel map");
    auto npr_table =
        get_hash_table<nt_k, nt_v>(K8sdispatcher::EBPF_NPR_TABLE_MAP);
    logger()->trace("Retrieved NodePort rules kernel map");
    nt_k npr_key{.port = htons(nodeportPort),
                 .proto = utils::L4ProtoEnum_to_int(proto)};

    npr_table.remove(npr_key);

    if (this->nodePortRuleMap_.erase(key) == 0) {
      std::runtime_error{"No NodePort rule associated with the provided key"};
    }
  } catch (std::exception &ex) {
    logger()->error("Failed to delete NodePort rule: {}", ex.what());
    throw std::runtime_error("Failed to delete NodePort rule");
  }
  logger()->info("Deleted NodePort rule");
}

void K8sdispatcher::delNodeportRuleList() {
  for (auto &it : nodePortRuleMap_) {
    NodeportKey key = it.first;
    this->delNodeportRule(std::get<0>(key), std::get<1>(key));
  }
}

void K8sdispatcher::reloadConfig() {
  std::string flags;

  uint16_t frontend = 0;
  uint16_t backend = 0;
  for (auto &it : get_ports()) {
    if (it->getType() == PortsTypeEnum::FRONTEND)
      frontend = it->index();
    else
      backend = it->index();
  }
  flags += "#define FRONTEND_PORT " + std::to_string(frontend) + "\n";
  flags += "#define BACKEND_PORT " + std::to_string(backend) + "\n";
  flags += "#define NODE_IP " + std::to_string(this->nodeIpNboInt_) + "\n";
  flags +=
      "#define INTERNAL_SRC_IP " + std::to_string(this->internalSrcIpNboInt_);

  logger()->trace("Reloading code with the following flags:\n{}", flags);

  reload(flags + '\n' + k8sdispatcher_code);

  logger()->trace("Reloaded K8sDispatcher code");
}

std::shared_ptr<Ports> K8sdispatcher::getFrontendPort() {
  for (auto &it : get_ports()) {
    if (it->getType() == PortsTypeEnum::FRONTEND) {
      return it;
    }
  }
  return nullptr;
}

std::shared_ptr<Ports> K8sdispatcher::getBackendPort() {
  for (auto &it : get_ports()) {
    if (it->getType() == PortsTypeEnum::BACKEND) {
      return it;
    }
  }
  return nullptr;
}