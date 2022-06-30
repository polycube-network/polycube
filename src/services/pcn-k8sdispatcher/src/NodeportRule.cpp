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

#include "NodeportRule.h"

#include "K8sdispatcher.h"
#include "Utils.h"

NodeportRule::NodeportRule(K8sdispatcher &parent,
                           const NodeportRuleJsonObject &conf)
    : NodeportRuleBase(parent),
      parent_{parent},
      nodeportPort_{conf.nodeportPortIsSet()
                        ? conf.getNodeportPort()
                        : throw std::runtime_error{"NodePort rule port"
                                                   "must be provided"}},
      proto_{
          conf.protoIsSet()
              ? conf.getProto()
              : throw std::
                    runtime_error{"NodePort rule protocol must be provided"}},
      externalTrafficPolicy_{
          conf.externalTrafficPolicyIsSet()
              ? conf.getExternalTrafficPolicy()
              : NodeportRuleExternalTrafficPolicyEnum::CLUSTER},
      ruleName_{conf.getRuleName()} {
  logger()->info("Creating NodeportRule instance");
  if (this->proto_ == L4ProtoEnum::ICMP) {
    throw std::runtime_error{"NodePort rule protocol cannot be ICMP"};
  }
}

NodeportRule::~NodeportRule() {
  logger()->info("Destroying NodeportRule instance");
}

uint16_t NodeportRule::getNodeportPort() {
  return this->nodeportPort_;
}

L4ProtoEnum NodeportRule::getProto() {
  return this->proto_;
}

NodeportRuleExternalTrafficPolicyEnum NodeportRule::getExternalTrafficPolicy() {
  return this->externalTrafficPolicy_;
}

void NodeportRule::setExternalTrafficPolicy(
    const NodeportRuleExternalTrafficPolicyEnum &value) {
  logger()->trace(
      "Received a request to update NodePort rule external traffic policy");
  try {
    logger()->trace("Retrieving NodePort rules kernel map");
    auto npr_table = this->parent_.get_hash_table<struct nt_k, struct nt_v>(
        K8sdispatcher::EBPF_NPR_TABLE_MAP);
    logger()->trace("Retrieved NodePort rules kernel map");

    struct nt_k npr_key {
      .port = htons(this->nodeportPort_),
      .proto = utils::L4ProtoEnum_to_int(this->proto_),
    };
    struct nt_v npr_value {
      .external_traffic_policy =
          utils::NodeportRuleExternalTrafficPolicyEnum_to_int(value)
    };

    logger()->trace(
        "Updating NodePort rule external traffic policy in NodePort rules "
        "kernel map");
    npr_table.set(npr_key, npr_value);
    this->externalTrafficPolicy_ = value;
    logger()->trace(
        "Updated NodePort rule external traffic policy in NodePort rules "
        "kernel map");
  } catch (std::exception &ex) {
    logger()->error("Failed to update NodePort rule in kernel map: {}",
                    ex.what());
    throw std::runtime_error("Failed to update NodePort rule in kernel map");
  }
  logger()->trace("Updated NodePort rule external traffic policy");
}

std::string NodeportRule::getRuleName() {
  return this->ruleName_;
}