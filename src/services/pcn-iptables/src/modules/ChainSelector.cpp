/*
 * Copyright 2017 The Polycube Authors
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

#include "../../../../polycubed/src/netlink.h"
#include "../Iptables.h"
#include "datapaths/Iptables_ChainSelector_dp.h"

Iptables::ChainSelector::ChainSelector(const int &index, Iptables &outer,
                                       const ProgramType t)
    : Iptables::Program(iptables_code_chainselector, index,
                        (t == ProgramType::INGRESS)
                            ? ChainNameEnum::INVALID_INGRESS
                            : ChainNameEnum::INVALID_EGRESS,
                        outer, t) {
  load();

  updateLocalIps();

  netlink_notification_index_chainselector_ =
      netlink_instance_chainselector_.registerObserver(
          polycube::polycubed::Netlink::Event::ALL,
          std::bind(&ChainSelector::netlinkNotificationCallbackChainSelector,
                    this));
}

Iptables::ChainSelector::~ChainSelector() {
  netlink_instance_chainselector_.unregisterObserver(
      polycube::polycubed::Netlink::Event::ALL,
      netlink_notification_index_chainselector_);
}

void Iptables::ChainSelector::netlinkNotificationCallbackChainSelector() {
  // logger()->debug("ChainSelector - Netlink notification received");
  updateLocalIps();
}

// If default action is drop, drop the packet
// else, if default action is accept, forward it to ConntrackTableUpdate

std::string Iptables::ChainSelector::defaultActionString(
    ChainNameEnum direction) {
  try {
    auto chain = iptables_.getChain(direction);
    if (chain->getDefault() == ActionEnum::DROP) {
      return "pcn_log(ctx, LOG_TRACE, \"[_HOOK] [ChainSelector] "
             "DROP_NO_LABELING\"); \n "
             "return RX_DROP;";
    } else if (chain->getDefault() == ActionEnum::ACCEPT) {
      std::string ret =
          "pcn_log(ctx, LOG_TRACE, \"[_HOOK] [ChainSelector] PASS_LABELING\"); "
          "\n "
          "updateForwardingDecision(PASS_LABELING);";
      return ret;
    }
  } catch (...) {
    return "return RX_DROP;";
  }
}

void Iptables::ChainSelector::updateLocalIps() {
  std::lock_guard<std::mutex> guard(program_mutex_);

  // logic to retrieve ips and interfaces:
  // This function is invoked with Parser constructor
  // but can be invoked also in other points

  // Algorithm:
  // Maintain vector of ip addresses
  // Look at new ip addresses
  // modify (add/rm) diff between old and new vectors

  std::unordered_map<std::string, std::string> localip_new;

  auto ifaces =
      polycube::polycubed::Netlink::getInstance().get_available_ifaces();
  for (auto &it : ifaces) {
    // auto name = it.second.get_name();
    auto addrs = it.second.get_addresses();
    for (auto &ip : addrs) {
      iptables_.logger()->trace("++new_IP: {0} ", ip);
      localip_new.insert({ip, ip});
    }
  }

  for (auto &new_ip : localip_new) {
    if (local_ips_.find(new_ip.first) == local_ips_.end()) {
      local_ips_.insert({new_ip.first, new_ip.first});
      iptables_.logger()->info("ip: {0} was not present. ++ ADDING",
                               new_ip.first);
      try {
        uint32_t ip_be = polycube::service::utils::ip_string_to_be_uint(
            removeNetFromIp(new_ip.first));
        auto localip_table =
            iptables_.get_hash_table<uint32_t, int>("localip", getIndex());
        localip_table.set(ip_be, 0);
      } catch (...) {
        // std::cout << "EXCEPTION" << std::endl;
      }
    }
    // else element already present
  }

  for (auto old_ip = local_ips_.begin(); old_ip != local_ips_.end();) {
    if (localip_new.find((*old_ip).first) == localip_new.end()) {
      iptables_.logger()->info("ip: {0} is not present. -- REMOVING",
                               (*old_ip).first);
      try {
        uint32_t ip_be = polycube::service::utils::ip_string_to_be_uint(
            removeNetFromIp((*old_ip).first));
        auto localipTable =
            iptables_.get_hash_table<uint32_t, int>("localip", getIndex());
        localipTable.remove(ip_be);
      } catch (...) {
        // std::cout << "EXCEPTION" << std::endl;
      }
      old_ip = local_ips_.erase(old_ip);
    } else {
      ++old_ip;
    }
    // else element already present
  }

  iptables_.logger()->trace("updating localip used in Additional Logic");
}

uint64_t Iptables::ChainSelector::getDefaultPktsCount(ChainNameEnum chain) {
  std::string table_name = "pkts_default_";

  if (chain == ChainNameEnum::INPUT)
    table_name += "Input";
  else if (chain == ChainNameEnum::FORWARD)
    table_name += "Forward";
  else if (chain == ChainNameEnum::OUTPUT)
    table_name += "Output";

  try {
    std::lock_guard<std::mutex> guard(program_mutex_);
    uint64_t pkts = 0;

    auto pkts_table = iptables_.get_percpuarray_table<uint64_t>(
        table_name, index_, program_type_);
    auto values = pkts_table.get(0);

    return std::accumulate(values.begin(), values.end(), pkts);
  } catch (...) {
    throw std::runtime_error("Counter not available.");
  }
}

uint64_t Iptables::ChainSelector::getDefaultBytesCount(ChainNameEnum chain) {
  std::string table_name = "bytes_default_";

  if (chain == ChainNameEnum::INPUT)
    table_name += "Input";
  else if (chain == ChainNameEnum::FORWARD)
    table_name += "Forward";
  else if (chain == ChainNameEnum::OUTPUT)
    table_name += "Output";

  try {
    std::lock_guard<std::mutex> guard(program_mutex_);
    auto bytes_table = iptables_.get_percpuarray_table<uint64_t>(
        table_name, index_, program_type_);
    auto values = bytes_table.get(0);
    uint64_t bytes = 0;
    return std::accumulate(values.begin(), values.end(), bytes);
  } catch (...) {
    throw std::runtime_error("Counter not available.");
  }
}

std::string Iptables::ChainSelector::getCode() {
  std::string no_macro_code = code_;

  /*Ingress or Egress logic*/
  if (program_type_ == ProgramType::INGRESS) {
    // if this is a parser for INPUT hook:
    replaceAll(no_macro_code, "_INGRESS_LOGIC", std::to_string(1));
    replaceAll(no_macro_code, "_EGRESS_LOGIC", std::to_string(0));

    auto chain_input = iptables_.getChain(ChainNameEnum::INPUT);
    auto chain_forward = iptables_.getChain(ChainNameEnum::FORWARD);
    if (chain_input->getDefault() == ActionEnum::ACCEPT &&
        chain_forward->getDefault() == ActionEnum::ACCEPT
        // && no rules
        && chain_input->getNrRules() == 0 && chain_forward->getNrRules() == 0) {
      replaceAll(no_macro_code, "_INGRESS_ALLOWLOGIC", std::to_string(1));
    } else {
      replaceAll(no_macro_code, "_INGRESS_ALLOWLOGIC", std::to_string(0));
    }

  } else if (program_type_ == ProgramType::EGRESS) {
    // if this is a parser for OUTPUT hook:
    replaceAll(no_macro_code, "_INGRESS_LOGIC", std::to_string(0));
    replaceAll(no_macro_code, "_EGRESS_LOGIC", std::to_string(1));
  }

  /* Replacing next hops_*/
  replaceAll(no_macro_code, "_CONNTRACK_LABEL_INGRESS",
             std::to_string(ModulesConstants::CONNTRACKLABEL_INGRESS));
  replaceAll(no_macro_code, "_CONNTRACK_LABEL_EGRESS",
             std::to_string(ModulesConstants::CONNTRACKLABEL_EGRESS));

  /* Replacing next hops_*/
  replaceAll(no_macro_code, "_ACTIONCACHE_INGRESS",
             std::to_string(ModulesConstants::ACTIONCACHE_INGRESS));
  replaceAll(no_macro_code, "_ACTIONCACHE_EGRESS",
             std::to_string(ModulesConstants::ACTIONCACHE_EGRESS));

  /*Replacing the maximum number of rules*/
  replaceAll(no_macro_code, "_MAXRULES",
             std::to_string(FROM_NRULES_TO_NELEMENTS(iptables_.max_rules_)));

  /*Replacing the default action*/
  replaceAll(no_macro_code, "_DEFAULTACTION_INPUT",
             this->defaultActionString(ChainNameEnum::INPUT));
  replaceAll(no_macro_code, "_DEFAULTACTION_FORWARD",
             this->defaultActionString(ChainNameEnum::FORWARD));
  replaceAll(no_macro_code, "_DEFAULTACTION_OUTPUT",
             this->defaultActionString(ChainNameEnum::OUTPUT));

  /*Replacing nrElements*/
  replaceAll(no_macro_code, "_NR_ELEMENTS_INPUT",
             std::to_string(FROM_NRULES_TO_NELEMENTS(
                 iptables_.getChain(ChainNameEnum::INPUT)->getNrRules())));

  try {
    replaceAll(no_macro_code, "_NR_ELEMENTS_FORWARD",
               std::to_string(FROM_NRULES_TO_NELEMENTS(
                   iptables_.getChain(ChainNameEnum::FORWARD)->getNrRules())));
  } catch (...) {
    // Egress chain not active.
    replaceAll(no_macro_code, "_NR_ELEMENTS_FORWARD",
               std::to_string(FROM_NRULES_TO_NELEMENTS(0)));
  }

  try {
    replaceAll(no_macro_code, "_NR_ELEMENTS_OUTPUT",
               std::to_string(FROM_NRULES_TO_NELEMENTS(
                   iptables_.getChain(ChainNameEnum::OUTPUT)->getNrRules())));
  } catch (...) {
    // Egress chain not active.
    replaceAll(no_macro_code, "_NR_ELEMENTS_OUTPUT",
               std::to_string(FROM_NRULES_TO_NELEMENTS(0)));
  }

  if (program_type_ == ProgramType::INGRESS) {
    replaceAll(no_macro_code, "call_bpf_program", "call_ingress_program");
  } else if (program_type_ == ProgramType::EGRESS) {
    replaceAll(no_macro_code, "call_bpf_program", "call_egress_program");
  }

  /*Ingress or Egress logic*/
  if (program_type_ == ProgramType::INGRESS) {
    replaceAll(no_macro_code, "_HOOK", "INGRESS");
  } else if (program_type_ == ProgramType::EGRESS) {
    replaceAll(no_macro_code, "_HOOK", "EGRESS");
  }

  return no_macro_code;
}

std::string Iptables::ChainSelector::removeNetFromIp(std::string ip) {
  std::string ip_without_net = ip.substr(0, ip.find("/"));
  // std::cout << "Converting  " << ip << " -> " << ip_without_net << std::endl;
  return ip_without_net;
}
