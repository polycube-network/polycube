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

#include "../interface/RuleMasqueradeInterface.h"

#include <spdlog/spdlog.h>

class Rule;

using namespace io::swagger::server::model;

class RuleMasquerade : public RuleMasqueradeInterface {
  friend class Nat;
 public:
  RuleMasquerade(Rule &parent);
  RuleMasquerade(Rule &parent, const RuleMasqueradeJsonObject &conf);
  virtual ~RuleMasquerade();

  std::shared_ptr<spdlog::logger> logger();
  void update(const RuleMasqueradeJsonObject &conf) override;
  RuleMasqueradeJsonObject toJsonObject() override;

  RuleMasqueradeEnableOutputJsonObject enable() override;
  RuleMasqueradeDisableOutputJsonObject disable() override;

  bool getEnabled() override;
  void setEnabled(const bool &value) override;

 private:
  bool inject(uint32_t ip); // injects the rule in datapath
  Rule &parent_;
  bool enabled;
};
