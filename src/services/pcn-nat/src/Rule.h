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

#include "../interface/RuleInterface.h"

#include <spdlog/spdlog.h>

#include "RuleDnat.h"
#include "RuleMasquerade.h"
#include "RulePortForwarding.h"
#include "RuleSnat.h"

class Nat;

using namespace io::swagger::server::model;

class Rule : public RuleInterface {
  friend class RuleSnat;
  friend class RuleDnat;
  friend class RulePortForwarding;
  friend class RuleMasquerade;

 public:
  Rule(Nat &parent, const RuleJsonObject &conf);
  virtual ~Rule();

  std::shared_ptr<spdlog::logger> logger();
  void update(const RuleJsonObject &conf) override;
  RuleJsonObject toJsonObject() override;

  /// <summary>
  ///
  /// </summary>
  std::shared_ptr<RuleSnat> getSnat() override;
  void addSnat(const RuleSnatJsonObject &value) override;
  void replaceSnat(const RuleSnatJsonObject &conf) override;
  void delSnat() override;

  /// <summary>
  ///
  /// </summary>
  std::shared_ptr<RuleMasquerade> getMasquerade() override;
  void addMasquerade(const RuleMasqueradeJsonObject &value) override;
  void replaceMasquerade(const RuleMasqueradeJsonObject &conf) override;
  void delMasquerade() override;

  /// <summary>
  ///
  /// </summary>
  std::shared_ptr<RuleDnat> getDnat() override;
  void addDnat(const RuleDnatJsonObject &value) override;
  void replaceDnat(const RuleDnatJsonObject &conf) override;
  void delDnat() override;

  /// <summary>
  ///
  /// </summary>
  std::shared_ptr<RulePortForwarding> getPortForwarding() override;
  void addPortForwarding(const RulePortForwardingJsonObject &value) override;
  void replacePortForwarding(const RulePortForwardingJsonObject &conf) override;
  void delPortForwarding() override;

  Nat &getParent();

 private:
  Nat &parent_;
  std::shared_ptr<RuleSnat> snat_;
  std::shared_ptr<RuleDnat> dnat_;
  std::shared_ptr<RulePortForwarding> portforwarding_;
  std::shared_ptr<RuleMasquerade> masquerade_;
};
