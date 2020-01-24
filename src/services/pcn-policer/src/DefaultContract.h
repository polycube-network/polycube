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


#pragma once


#include "../base/DefaultContractBase.h"


class Policer;

using namespace polycube::service::model;


class DefaultContract : public DefaultContractBase {
 public:
  DefaultContract(Policer &parent, const DefaultContractJsonObject &conf);
  ~DefaultContract();
  DefaultContractJsonObject toJsonObject();

  /// <summary>
  /// Action applied to traffic of the contract: PASS &#x3D; Let all the traffic pass without limitations; LIMIT &#x3D; Apply rate and burst limits to selected traffic; DROP &#x3D; Drop all the traffic
  /// </summary>
  ActionTypeEnum getAction() override;

  /// <summary>
  /// Maximum average traffic rate (in bps)
  /// </summary>
  uint64_t getRateLimit() override;

  /// <summary>
  /// Maximum size of a burst of packets (in bits)
  /// </summary>
  uint64_t getBurstLimit() override;

  void updateData(DefaultContractUpdateDataInputJsonObject input) override;

  std::string toString();

 private:
  ActionTypeEnum action_;
  uint64_t rate_limit_;
  uint64_t burst_limit_;

  void updateDataplane();
};
