/*
 * Copyright 2020 The Polycube Authors
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

#include "Contract.h"
#include "Policer.h"

Contract::Contract(Policer &parent, const ContractJsonObject &conf)
    : ContractBase(parent) {
  traffic_class_ = conf.getTrafficClass();
  action_ = conf.getAction();

  if (action_ == ActionTypeEnum::LIMIT) {
    if (!conf.rateLimitIsSet() || ! conf.burstLimitIsSet()) {
      throw std::runtime_error("Action LIMIT requires rate and burst limits");
    }

    rate_limit_ = conf.getRateLimit();
    burst_limit_ = conf.getBurstLimit();
  
  } else {
    if (conf.rateLimitIsSet() || conf.burstLimitIsSet()) {
      throw std::runtime_error(
          "Rate and burst limits can only be set with action LIMIT");
    }

    rate_limit_ = 0;
    burst_limit_ = 0;
  }

  updateDataplane();
}

Contract::~Contract() {
  parent_.get_hash_table<uint32_t, struct contract>("contracts")
         .remove(traffic_class_);
}

ContractJsonObject Contract::toJsonObject() {
  ContractJsonObject conf;

  conf.setTrafficClass(traffic_class_);
  conf.setAction(action_);
  if (action_ == ActionTypeEnum::LIMIT) {
    conf.setRateLimit(rate_limit_);
    conf.setBurstLimit(burst_limit_);
  }

  return conf;
}

uint32_t Contract::getTrafficClass() {
  return traffic_class_;
}

ActionTypeEnum Contract::getAction() {
  return action_;
}

uint64_t Contract::getRateLimit() {
  return rate_limit_;
}

uint64_t Contract::getBurstLimit() {
  return burst_limit_;
}

void Contract::updateData(ContractUpdateDataInputJsonObject input) {
  if (input.actionIsSet()) {
    if (input.getAction() == ActionTypeEnum::LIMIT) {
      if (!input.rateLimitIsSet() || !input.burstLimitIsSet()) {
        throw std::runtime_error("Action LIMIT requires rate and burst limits");
      }

      rate_limit_ = input.getRateLimit();
      burst_limit_ = input.getBurstLimit();

    } else {
      if (input.rateLimitIsSet() || input.burstLimitIsSet()) {
        throw std::runtime_error(
            "Rate and burst limits can only be set with action LIMIT");
      }

      rate_limit_ = 0;
      burst_limit_ = 0;
    }

    action_ = input.getAction();
  
  } else {
    if (action_ != ActionTypeEnum::LIMIT) {
      throw std::runtime_error(
          "Rate and burst limits can only be set with action LIMIT");
    }

    if (input.rateLimitIsSet()) {
      rate_limit_ = input.getRateLimit();
    }

    if (input.burstLimitIsSet()) {
      burst_limit_ = input.getBurstLimit();
    }
  }

  updateDataplane();

  logger()->info("Contract updated {0}", toString());
}

void Contract::updateDataplane() {
  struct bucket bucket = {
    .tokens = burst_limit_ * 1000000,
    .refill_rate = rate_limit_,
    .capacity = burst_limit_ * 1000000,
    .last_update = 0
  };

  struct contract contract = {
    .action = static_cast<uint8_t>(action_),
    .bucket = bucket
  };

  parent_.get_hash_table<uint32_t, struct contract>("contracts")
         .set(traffic_class_, contract);
}

std::string Contract::toString() {
  return toJsonObject().toJson().dump();
}