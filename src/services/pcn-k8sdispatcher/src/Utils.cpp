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

#include "Utils.h"

#include "base/K8sdispatcherBase.h"

uint8_t utils::L4ProtoEnum_to_int(L4ProtoEnum proto) {
  switch (proto) {
  case L4ProtoEnum::TCP:
    return 6;
  case L4ProtoEnum::UDP:
    return 17;
  case L4ProtoEnum::ICMP:
    return 1;
  default:
    throw std::runtime_error("Bad proto");
  }
}

L4ProtoEnum utils::int_to_L4ProtoEnum(uint8_t proto) {
  switch (proto) {
  case 6:
    return L4ProtoEnum::TCP;
  case 17:
    return L4ProtoEnum::UDP;
  case 1:
    return L4ProtoEnum::ICMP;
  default:
    throw std::runtime_error("Bad proto number");
  }
}

uint8_t utils::SessionRuleOperationEnum_to_int(
    SessionRuleOperationEnum operation) {
  switch (operation) {
  case SessionRuleOperationEnum::XLATE_SRC:
    return 0;
  case SessionRuleOperationEnum::XLATE_DST:
    return 1;
  default:
    throw std::runtime_error("Bad operation");
  }
}

SessionRuleOperationEnum utils::int_to_SessionRuleOperationEnum(
    const uint8_t operation) {
  switch (operation) {
  case 0:
    return SessionRuleOperationEnum::XLATE_SRC;
  case 1:
    return SessionRuleOperationEnum::XLATE_DST;
  default:
    throw std::runtime_error("Bad operation number");
  }
}

uint8_t utils::SessionRuleOriginatingRuleEnum_to_int(
    SessionRuleOriginatingRuleEnum originatingRule) {
  switch (originatingRule) {
  case SessionRuleOriginatingRuleEnum::POD_TO_EXT:
    return 0;
  case SessionRuleOriginatingRuleEnum::NODEPORT_CLUSTER:
    return 1;
  default:
    throw std::runtime_error("Bad originating rule");
  }
}

SessionRuleOriginatingRuleEnum utils::int_to_SessionRuleOriginatingRuleEnum(
    const uint8_t originatingRule) {
  switch (originatingRule) {
  case 0:
    return SessionRuleOriginatingRuleEnum::POD_TO_EXT;
  case 1:
    return SessionRuleOriginatingRuleEnum::NODEPORT_CLUSTER;
  default:
    throw std::runtime_error("Bad originating rule number");
  }
}

uint8_t utils::NodeportRuleExternalTrafficPolicyEnum_to_int(
    NodeportRuleExternalTrafficPolicyEnum externalTrafficPolicy) {
  switch (externalTrafficPolicy) {
  case NodeportRuleExternalTrafficPolicyEnum::LOCAL:
    return 0;
  case NodeportRuleExternalTrafficPolicyEnum::CLUSTER:
    return 1;
  default:
    throw std::runtime_error("Bad external traffic policy");
  }
}

NodeportRuleExternalTrafficPolicyEnum
utils::int_to_NodeportRuleExternalTrafficPolicyEnum(
    const uint8_t externalTrafficPolicy) {
  switch (externalTrafficPolicy) {
  case 0:
    return NodeportRuleExternalTrafficPolicyEnum::LOCAL;
  case 1:
    return NodeportRuleExternalTrafficPolicyEnum::CLUSTER;
  default:
    throw std::runtime_error("Bad external traffic policy number");
  }
}

bool utils::is_valid_ipv4_str(std::string const &ip) {
  try {
    // if the provided IPv4 address is not valid, the following call
    // will throw and exception
    polycube::service::utils::ip_string_to_nbo_uint(ip);
    return true;
  } catch (...) {
    return false;
  }
}