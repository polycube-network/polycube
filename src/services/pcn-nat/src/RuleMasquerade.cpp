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

// Modify these methods with your own implementation

#include "RuleMasquerade.h"
#include "Nat.h"

using namespace polycube::service;

RuleMasquerade::RuleMasquerade(Rule &parent) : parent_(parent) {
  enabled = false;
}

RuleMasquerade::RuleMasquerade(Rule &parent,
                               const RuleMasqueradeJsonObject &conf)
    : parent_(parent) {
  update(conf);
}

RuleMasquerade::~RuleMasquerade() {}

void RuleMasquerade::update(const RuleMasqueradeJsonObject &conf) {
  // This method updates all the object/parameter in RuleMasquerade object
  // specified in the conf JsonObject.
  // You can modify this implementation.
  if (conf.enabledIsSet()) {
    setEnabled(conf.getEnabled());
  } else {
    setEnabled(false);
  }
}

RuleMasqueradeJsonObject RuleMasquerade::toJsonObject() {
  RuleMasqueradeJsonObject conf;
  conf.setEnabled(getEnabled());
  return conf;
}

bool RuleMasquerade::inject(uint32_t ip) {
 try {
    // Inject rule in the datapath table
    auto sm_rules = parent_.getParent().get_hash_table<sm_k, sm_v>(
        "sm_rules", 0, ProgramType::EGRESS);
    sm_k key{
        .internal_netmask_len = 0, .internal_ip = 0,
    };

    sm_v value{
        .external_ip = ip,
        .entry_type = (uint8_t)NattingTableOriginatingRuleEnum::MASQUERADE,
    };
    sm_rules.set(key, value);
  } catch (std::exception &e) {
    logger()->error("Error injecting masquerate rule " + std::string(e.what()));
    return false;
  }

  return true;
}

RuleMasqueradeEnableOutputJsonObject RuleMasquerade::enable() {
  RuleMasqueradeEnableOutputJsonObject output;
  if (enabled) {
    // Already enabled
    output.setResult(true);
    return output;
  }

  uint32_t ip = 0;

  try {
    ip = utils::ip_string_to_nbo_uint(
                       parent_.getParent().getExternalIpString());
  } catch(...) {
    output.setResult(false);
    return output;
  }

  bool result = inject(ip);
  output.setResult(result);
  if (result) {
    enabled = true;
    logger()->info("Enabled masquerade: 0.0.0.0 -> {0}",
                   parent_.getParent().getExternalIpString());
  }

  return output;
}

RuleMasqueradeDisableOutputJsonObject RuleMasquerade::disable() {
  RuleMasqueradeDisableOutputJsonObject output;
  if (!enabled) {
    // Already disabled
    output.setResult(true);
    return output;
  }

  bool result = inject(0);
  output.setResult(result);
  if (result) {
    enabled = false;
    logger()->info("Disabled masquerade");
  }

  return output;
}

void RuleMasquerade::setEnabled(const bool &value) {
  if (value) {
    enable();
  } else {
    disable();
  }

  enabled = value;
}

bool RuleMasquerade::getEnabled() {
  return enabled;
}

std::shared_ptr<spdlog::logger> RuleMasquerade::logger() {
  return parent_.logger();
}
