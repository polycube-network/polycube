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

#include "../base/NodeportRuleBase.h"

class K8sdispatcher;

using namespace polycube::service::model;

class NodeportRule : public NodeportRuleBase {
 public:
  NodeportRule(K8sdispatcher &parent, const NodeportRuleJsonObject &conf);
  virtual ~NodeportRule();

  /// <summary>
  /// NodePort rule nodeport port number
  /// </summary>
  uint16_t getNodeportPort() override;

  /// <summary>
  /// NodePort rule L4 protocol
  /// </summary>
  L4ProtoEnum getProto() override;

  /// <summary>
  /// The external traffic policy of the Kubernetes NodePort Service
  /// </summary>
  NodeportRuleExternalTrafficPolicyEnum getExternalTrafficPolicy() override;
  void setExternalTrafficPolicy(const NodeportRuleExternalTrafficPolicyEnum &value) override;

  /// <summary>
  /// An optional name for the NodePort rule
  /// </summary>
  std::string getRuleName() override;

 private:
  K8sdispatcher& parent_;

  uint16_t nodeportPort_;
  L4ProtoEnum proto_;

  NodeportRuleExternalTrafficPolicyEnum externalTrafficPolicy_;
  std::string ruleName_;
};
