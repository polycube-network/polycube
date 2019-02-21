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

void RuleMasquerade::create(Rule &parent,
                            const RuleMasqueradeJsonObject &conf) {
  // This method creates the actual RuleMasquerade object given thee key param.
  // Please remember to call here the create static method for all sub-objects
  // of RuleMasquerade.
  parent.masquerade_ = std::make_shared<RuleMasquerade>(parent, conf);
}

std::shared_ptr<RuleMasquerade> RuleMasquerade::getEntry(Rule &parent) {
  // This method retrieves the pointer to RuleMasquerade object specified by its
  // keys.
  return parent.masquerade_;
}

void RuleMasquerade::removeEntry(Rule &parent) {
  // This method removes the single RuleMasquerade object specified by its keys.
  // Remember to call here the remove static method for all-sub-objects of
  // RuleMasquerade.

  // Treat deletion as disabling
  parent.masquerade_->disable();
}

RuleMasqueradeEnableOutputJsonObject RuleMasquerade::enable() {
  RuleMasqueradeEnableOutputJsonObject output;
  if (enabled) {
    // Already enabled
    output.setResult(true);
    return output;
  }
  try {
    // Inject rule in the datapath table
    auto sm_rules = parent_.getParent().get_hash_table<sm_k, sm_v>(
        "sm_rules", 0, ProgramType::EGRESS);
    sm_k key{
        .internal_netmask_len = 0, .internal_ip = 0,
    };

    sm_v value{
        .external_ip = utils::ip_string_to_be_uint(
            parent_.getParent().getExternalIpString()),
        .entry_type = (uint8_t)NattingTableOriginatingRuleEnum::MASQUERADE,
    };
    sm_rules.set(key, value);
    enabled = true;
    output.setResult(true);
    logger()->info("Enabled masquerade: 0.0.0.0 -> {0}",
                   parent_.getParent().getExternalIpString());
  } catch (std::exception &e) {
    logger()->info("Could not enable masquerade: " + std::string(e.what()));
    output.setResult(false);
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
  try {
    auto sm_rules = parent_.getParent().get_hash_table<sm_k, sm_v>(
        "sm_rules", 0, ProgramType::EGRESS);
    sm_k key{
        .internal_netmask_len = 0, .internal_ip = 0,
    };

    sm_rules.remove(key);

    enabled = false;
    output.setResult(true);
    logger()->info("Disabled masquerade");
  } catch (std::exception &e) {
    logger()->info("Could not disable masquerade: " + std::string(e.what()));
    output.setResult(false);
  }
  return output;
}

void RuleMasquerade::setEnabled(const bool &value) {
  enabled = value;
}
bool RuleMasquerade::getEnabled() {
  return enabled;
}

std::shared_ptr<spdlog::logger> RuleMasquerade::logger() {
  return parent_.logger();
}
