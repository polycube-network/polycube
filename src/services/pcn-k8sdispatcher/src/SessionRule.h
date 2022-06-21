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

#include "../base/SessionRuleBase.h"

struct st_k {
  uint32_t src_ip;
  uint32_t dst_ip;
  uint16_t src_port;
  uint16_t dst_port;
  uint8_t proto;
} __attribute__((packed));
struct st_v {
  uint32_t new_ip;
  uint16_t new_port;
  uint8_t operation;
  uint8_t originating_rule;
} __attribute__((packed));

class K8sdispatcher;

using namespace polycube::service::model;

class SessionRule : public SessionRuleBase {
 public:
  SessionRule(K8sdispatcher &parent, const SessionRuleJsonObject &conf);
  SessionRule(K8sdispatcher &parent, SessionRuleDirectionEnum direction,
              std::string srcIp, std::string dstIp, uint16_t srcPort,
              uint16_t dstPort, L4ProtoEnum proto, std::string newIp,
              uint16_t newPort, SessionRuleOperationEnum operation,
              SessionRuleOriginatingRuleEnum originatingRule);
  virtual ~SessionRule();

  /// <summary>
  /// Session entry direction (e.g. INGRESS or EGRESS)
  /// </summary>
  SessionRuleDirectionEnum getDirection() override;

  /// <summary>
  /// Session entry source IP address
  /// </summary>
  std::string getSrcIp() override;

  /// <summary>
  /// Session entry destination IP address
  /// </summary>
  std::string getDstIp() override;

  /// <summary>
  /// Session entry source L4 port number
  /// </summary>
  uint16_t getSrcPort() override;

  /// <summary>
  /// Session entry destination L4 port number
  /// </summary>
  uint16_t getDstPort() override;

  /// <summary>
  /// Session entry L4 protocol
  /// </summary>
  L4ProtoEnum getProto() override;

  /// <summary>
  /// Translated IP address
  /// </summary>
  std::string getNewIp() override;

  /// <summary>
  /// Translated L4 port number
  /// </summary>
  uint16_t getNewPort() override;

  /// <summary>
  /// Operation applied on the original packet
  /// </summary>
  SessionRuleOperationEnum getOperation() override;

  /// <summary>
  /// Rule originating the session entry
  /// </summary>
  SessionRuleOriginatingRuleEnum getOriginatingRule() override;

 private:
  SessionRuleDirectionEnum direction_;
  std::string srcIp_;
  std::string dstIp_;
  uint16_t srcPort_;
  uint16_t dstPort_;
  L4ProtoEnum proto_;

  std::string newIp_;
  uint16_t newPort_;
  SessionRuleOperationEnum operation_;
  SessionRuleOriginatingRuleEnum originatingRule_;
};
