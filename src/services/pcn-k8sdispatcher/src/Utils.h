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

#ifndef POLYCUBE_UTILS_H
#define POLYCUBE_UTILS_H

#include "base/NodeportRuleBase.h"
#include "base/SessionRuleBase.h"

namespace utils {
// conversion functions
uint8_t L4ProtoEnum_to_int(L4ProtoEnum proto);
L4ProtoEnum int_to_L4ProtoEnum(const uint8_t proto);

uint8_t SessionRuleOperationEnum_to_int(SessionRuleOperationEnum operation);
SessionRuleOperationEnum int_to_SessionRuleOperationEnum(
    const uint8_t operation);

uint8_t SessionRuleOriginatingRuleEnum_to_int(
    SessionRuleOriginatingRuleEnum originatingRule);
SessionRuleOriginatingRuleEnum int_to_SessionRuleOriginatingRuleEnum(
    const uint8_t originatingRule);

uint8_t NodeportRuleExternalTrafficPolicyEnum_to_int(
    NodeportRuleExternalTrafficPolicyEnum externalTrafficPolicy);
NodeportRuleExternalTrafficPolicyEnum
int_to_NodeportRuleExternalTrafficPolicyEnum(
    const uint8_t externalTrafficPolicy);

// helper functions
bool is_valid_ipv4_str(std::string const &ip);

}  // namespace utils

#endif  // POLYCUBE_UTILS_H
