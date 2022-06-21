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

#include "SessionRule.h"
#include "K8sdispatcher.h"
#include "Utils.h"

SessionRule::SessionRule(K8sdispatcher &parent,
                         const SessionRuleJsonObject &conf)
    : SessionRule{parent,
                  conf.getDirection(),
                  conf.getSrcIp(),
                  conf.getDstIp(),
                  conf.getSrcPort(),
                  conf.getDstPort(),
                  conf.getProto(),
                  conf.getNewIp(),
                  conf.getNewPort(),
                  conf.getOperation(),
                  conf.getOriginatingRule()} {}

SessionRule::SessionRule(K8sdispatcher &parent,
                         SessionRuleDirectionEnum direction, std::string srcIp,
                         std::string dstIp, uint16_t srcPort, uint16_t dstPort,
                         L4ProtoEnum proto, std::string newIp, uint16_t newPort,
                         SessionRuleOperationEnum operation,
                         SessionRuleOriginatingRuleEnum originatingRule)
    : SessionRuleBase(parent),
      direction_{direction},
      srcIp_{srcIp},
      dstIp_{dstIp},
      srcPort_{srcPort},
      dstPort_{dstPort},
      proto_{proto},
      newIp_{newIp},
      newPort_{newPort},
      operation_{operation},
      originatingRule_{originatingRule} {
  logger()->info("Creating SessionRule instance");
  if (!utils::is_valid_ipv4_str(srcIp)) {
    throw std::runtime_error{"Invalid source IPv4 address"};
  }
  if (!utils::is_valid_ipv4_str(dstIp)) {
    throw std::runtime_error{"Invalid destination IPv4 address"};
  }
  if (!utils::is_valid_ipv4_str(newIp)) {
    throw std::runtime_error{"Invalid new IPv4 address"};
  }
}

SessionRule::~SessionRule() {
  logger()->info("Destroying SessionRule instance");
}

SessionRuleDirectionEnum SessionRule::getDirection() {
  return this->direction_;
}

std::string SessionRule::getSrcIp() {
  return this->srcIp_;
}

std::string SessionRule::getDstIp() {
  return this->dstIp_;
}

uint16_t SessionRule::getSrcPort() {
  return this->srcPort_;
}

uint16_t SessionRule::getDstPort() {
  return this->dstPort_;
}

L4ProtoEnum SessionRule::getProto() {
  return this->proto_;
}

std::string SessionRule::getNewIp() {
  return this->newIp_;
}

uint16_t SessionRule::getNewPort() {
  return this->newPort_;
}

SessionRuleOperationEnum SessionRule::getOperation() {
  return this->operation_;
}

SessionRuleOriginatingRuleEnum SessionRule::getOriginatingRule() {
  return this->originatingRule_;
}