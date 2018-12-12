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

#include "Iptables.h"
#include "../../../polycubed/src/netlink.h"
#include "./../../../polycubed/src/utils.h"
#include "Iptables_dp.h"

Iptables::Iptables(const std::string name, const IptablesJsonObject &conf,
                   CubeType type)
    : Cube(name, {iptables_code_ingress}, {iptables_code_egress}, type,
           conf.getPolycubeLoglevel()) {
  logger()->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [Iptables] [%n] [%l] %v");
  logger()->info("Creating Iptables instance");

  netlink_notification_index_ = netlink_instance_iptables_.registerObserver(
      polycube::polycubed::Netlink::Event::ALL,
      std::bind(&Iptables::netlinkNotificationCallbackIptables, this));
  update(conf);

  /*Creating default INPUT FORWARD OUTPUT chains.*/
  ChainJsonObject chain;
  chain.setDefault(ActionEnum::ACCEPT);

  addChain(ChainNameEnum::INPUT, chain);
  addChain(ChainNameEnum::FORWARD, chain);
  addChain(ChainNameEnum::OUTPUT, chain);

  logger()->debug("INPUT, FORWARD, OUTPUT chains added");

  /*Initialize program*/
  // Insert parser, first program of the chain

  // PARSER INGRESS
  programs_.insert(
      // pair <ModulesConstants TYPE, ChainName>
      // Iptables::Parser
      std::pair<std::pair<uint8_t, ChainNameEnum>, Iptables::Program *>(
          std::make_pair(ModulesConstants::PARSER_INGRESS,
                         ChainNameEnum::INVALID_INGRESS),
          new Iptables::Parser(ModulesConstants::PARSER_INGRESS, *this)));

  // PARSER EGRESS
  programs_.insert(
      // pair <ModulesConstants TYPE, ChainName>
      // Iptables::Parser
      std::pair<std::pair<uint8_t, ChainNameEnum>, Iptables::Program *>(
          std::make_pair(ModulesConstants::PARSER_EGRESS,
                         ChainNameEnum::INVALID_EGRESS),
          new Iptables::Parser(ModulesConstants::PARSER_EGRESS, *this,
                               ProgramType::EGRESS)));

  // CHAINSELECTOR INGRESS
  programs_.insert(
      std::pair<std::pair<uint8_t, ChainNameEnum>, Iptables::Program *>(
          std::make_pair(ModulesConstants::CHAINSELECTOR_INGRESS,
                         ChainNameEnum::INVALID_INGRESS),
          new Iptables::ChainSelector(ModulesConstants::CHAINSELECTOR_INGRESS,
                                      *this)));

  // CHAINSELECTOR EGRESS
  programs_.insert(
      std::pair<std::pair<uint8_t, ChainNameEnum>, Iptables::Program *>(
          std::make_pair(ModulesConstants::CHAINSELECTOR_EGRESS,
                         ChainNameEnum::INVALID_EGRESS),
          new Iptables::ChainSelector(ModulesConstants::CHAINSELECTOR_EGRESS,
                                      *this, ProgramType::EGRESS)));

  // CONNTRACK INGRESS
  programs_.insert(
      std::pair<std::pair<uint8_t, ChainNameEnum>, Iptables::Program *>(
          std::make_pair(ModulesConstants::CONNTRACKLABEL_INGRESS,
                         ChainNameEnum::INVALID_INGRESS),
          new Iptables::ConntrackLabel(ModulesConstants::CONNTRACKLABEL_INGRESS,
                                       *this)));

  // CONNTRACK EGRESS
  programs_.insert(
      std::pair<std::pair<uint8_t, ChainNameEnum>, Iptables::Program *>(
          std::make_pair(ModulesConstants::CONNTRACKLABEL_EGRESS,
                         ChainNameEnum::INVALID_EGRESS),
          new Iptables::ConntrackLabel(ModulesConstants::CONNTRACKLABEL_EGRESS,
                                       *this, ProgramType::EGRESS)));

  // CHAINFORWARDER INGRESS
  programs_.insert(
      std::pair<std::pair<uint8_t, ChainNameEnum>, Iptables::Program *>(
          std::make_pair(ModulesConstants::CHAINFORWARDER_INGRESS,
                         ChainNameEnum::INVALID_INGRESS),
          new Iptables::ChainForwarder(ModulesConstants::CHAINFORWARDER_INGRESS,
                                       *this)));

  // CHAINFORWARDER EGRESS
  programs_.insert(
      std::pair<std::pair<uint8_t, ChainNameEnum>, Iptables::Program *>(
          std::make_pair(ModulesConstants::CHAINFORWARDER_EGRESS,
                         ChainNameEnum::INVALID_EGRESS),
          new Iptables::ChainForwarder(ModulesConstants::CHAINFORWARDER_EGRESS,
                                       *this, ProgramType::EGRESS)));

  // CONNTRACK TABLE UPDATE INGRESS
  programs_.insert(
      std::pair<std::pair<uint8_t, ChainNameEnum>, Iptables::Program *>(
          std::make_pair(ModulesConstants::CONNTRACKTABLEUPDATE_INGRESS,
                         ChainNameEnum::INVALID_INGRESS),
          new Iptables::ConntrackTableUpdate(
              ModulesConstants::CONNTRACKTABLEUPDATE_INGRESS, *this)));

  // CONNTRACK TABLE UPDATE EGRESS
  programs_.insert(
      std::pair<std::pair<uint8_t, ChainNameEnum>, Iptables::Program *>(
          std::make_pair(ModulesConstants::CONNTRACKTABLEUPDATE_EGRESS,
                         ChainNameEnum::INVALID_EGRESS),
          new Iptables::ConntrackTableUpdate(
              ModulesConstants::CONNTRACKTABLEUPDATE_EGRESS, *this,
              ProgramType::EGRESS)));

  reloadAll();

  logger()->debug("Automatically Attaching to network interfaces");
  attachInterfaces();
}

Iptables::~Iptables() {
  Iptables::Program *pr =
      programs_[std::make_pair(ModulesConstants::CONNTRACKTABLEUPDATE_INGRESS,
                              ChainNameEnum::INVALID_INGRESS)];
  Iptables::ConntrackTableUpdate *ctu =
      static_cast<Iptables::ConntrackTableUpdate *>(pr);
  ctu->quitAndJoin();

  netlink_instance_iptables_.unregisterObserver(polycube::polycubed::Netlink::Event::ALL,
                                      netlink_notification_index_);

  logger()->info("[{0}] Destroying iptables...", get_name());
  // cleanup chains struct contained in control plane
  chains_.clear();

  // Delete all eBPF programs
  for (auto it = programs_.begin(); it != programs_.end(); ++it) {
    delete it->second;
  }
}

void Iptables::netlinkNotificationCallbackIptables() {
  logger()->debug("Iptables - Netlink notification received");
  attachInterfaces();
}

void Iptables::update(const IptablesJsonObject &conf) {
  // This method updates all the object/parameter in Iptables object specified
  // in the conf JsonObject.
  // You can modify this implementation.

  if (conf.chainIsSet()) {
    for (auto &i : conf.getChain()) {
      auto name = i.getName();
      auto m = getChain(name);
      m->update(i);
    }
  }

  if (conf.loglevelIsSet()) {
    setLoglevel(conf.getLoglevel());
  }

  if (conf.interactiveIsSet()) {
    setInteractive(conf.getInteractive());
  }

  if (conf.horusIsSet()) {
    setHorus(conf.getHorus());
  }

  if (conf.portsIsSet()) {
    for (auto &i : conf.getPorts()) {
      auto name = i.getName();
      auto m = getPorts(name);
      m->update(i);
    }
  }
}

IptablesJsonObject Iptables::toJsonObject() {
  IptablesJsonObject conf;

  conf.setName(getName());

  // Remove comments when you implement all sub-methods
  // for(auto &i : getChainList()){
  //  conf.addChain(i->toJsonObject());
  //}

  conf.setLoglevel(getLoglevel());
  conf.setConntrack(getConntrack());
  conf.setHorus(getHorus());
  conf.setInteractive(getInteractive());
  conf.setType(getType());

  for (auto &i : getPortsList()) {
    conf.addPorts(i->toJsonObject());
  }

  conf.setUuid(getUuid());

  return conf;
}

void Iptables::packet_in(Ports &port, polycube::service::PacketInMetadata &md,
                         const std::vector<uint8_t> &packet) {
  logger()->info("Packet received from port {0}", port.name());
}

bool Iptables::getInteractive() {
  // This method retrieves the interactive value.
  return interactive_;
}

void Iptables::setInteractive(const bool &value) {
  // This method set the interactive value.
  interactive_ = value;
}

// IptablesConntrackEnum Iptables::getConntrack(){
//   //This method retrieves the conntrack value.
//   throw std::runtime_error("[Iptables]: Method getConntrack not
//   implemented");
// }

// void Iptables::setConntrack(const IptablesConntrackEnum &value){
//   //This method set the conntrack value.
//   throw std::runtime_error("[Iptables]: Method setConntrack not
//   implemented");
// }

// reload is not bpf::reload, but reload function managed by pcn-iptables on
// each prog of the chain
void Iptables::reloadChain(ChainNameEnum chain) {
  for (auto it = programs_.begin(); it != programs_.end(); ++it) {
    if (it->first.second == chain) {
      it->second->reload();
    }
  }
}

void Iptables::reloadAll() {
  for (auto it = programs_.begin(); it != programs_.end(); ++it) {
    it->second->reload();
  }
}

void Iptables::attachInterfaces() {
  std::lock_guard<std::mutex> guard(mutex_iptables_);

  // logic to retrieve interfaces:
  // This function is invoked with Iptables constructor
  // but can be invoked also in other points

  // Algorithm:
  // Maintain vector of ports
  // Look at new ports
  // modify (add/rm) diff between old and new vectors

  // we have to ignore some interfaces:
  std::unordered_map<std::string, std::string> ignored_interfaces;

  ignored_interfaces.insert({":host", ":host"});
  ignored_interfaces.insert({"pcn_tc_cp", "pcn_tc_cp"});
  ignored_interfaces.insert({"pcn_xdp_cp", "pcn_xdp_cp"});

  std::unordered_map<std::string, std::string> connected_ports_new;

  auto ifaces =
      polycube::polycubed::Netlink::getInstance().get_available_ifaces();
  for (auto &it : ifaces) {
    auto name = it.second.get_name();
    logger()->trace("+Interface {0} ", name);
    if (ignored_interfaces.find(name) == ignored_interfaces.end()) {
      // logger()->info("+ ATTACH Interface {0} ", name);
      connected_ports_new.insert({name, name});
    }
  }

  for (auto &new_port : connected_ports_new) {
    if (connected_ports_.find(new_port.first) == connected_ports_.end()) {
      connected_ports_.insert({new_port.first, new_port.first});
      logger()->info("port: {0} was not present. ++ ADDING", new_port.first);
      try {
        connectPort(new_port.first);
      } catch (...) {
      }
    }
    // else element already present
  }

  for (auto old_port = connected_ports_.begin();
       old_port != connected_ports_.end();) {
    if (connected_ports_new.find((*old_port).first) ==
        connected_ports_new.end()) {
      try {
        disconnectPort((*old_port).first);
        logger()->info("port: {0} is no longer present. -- REMOVING",
                       (*old_port).first);
        old_port = connected_ports_.erase(old_port);
      } catch (...) {
        logger()->info("port: catching exception in disconnect");
      }
    } else {
      ++old_port;
    }
  }

  logger()->debug("Updating ports attached to pcn-iptables");
}

bool Iptables::connectPort(const std::string &name) {
  try {
    PortsJsonObject conf;
    conf.setName(name);
    conf.setPeer(name);

    Ports::create(*this, name, conf);
  } catch (...) {
    return false;
  }

  return true;
}

bool Iptables::disconnectPort(const std::string &name) {
  try {
    Ports::removeEntry(*this, name);
  } catch (...) {
    return false;
  }

  return true;
}

IptablesConntrackEnum Iptables::getConntrack() {
  if (this->conntrack_mode_ == ConntrackModes::DISABLED) {
    return IptablesConntrackEnum::OFF;
  }
  return IptablesConntrackEnum::ON;
}

void Iptables::enableAcceptEstablished(Chain &chain) {
  switch (chain.getName()) {
  case ChainNameEnum::INPUT:
    if (accept_established_enabled_input_) {
      logger()->debug("INPUT: accept ESTABLISHED optimization already enabled");
    } else {
      logger()->info("INPUT: Enabling ESTABLISHED optimization ...");
      accept_established_enabled_input_ = true;
      conntrack_mode_input_ = ConntrackModes::ON;
      programs_[std::make_pair(ModulesConstants::CONNTRACKLABEL_INGRESS,
                              ChainNameEnum::INVALID_INGRESS)]
          ->reload();
    }
    break;
  case ChainNameEnum::FORWARD:
    if (accept_established_enabled_forward_) {
      logger()->debug(
          "FORWARD: accept ESTABLISHED optimization already enabled");
    } else {
      logger()->info("FORWARD: Enabling ESTABLISHED optimization ...");
      accept_established_enabled_forward_ = true;
      conntrack_mode_forward_ = ConntrackModes::ON;
      programs_[std::make_pair(ModulesConstants::CONNTRACKLABEL_INGRESS,
                              ChainNameEnum::INVALID_INGRESS)]
          ->reload();
    }
    break;
  case ChainNameEnum::OUTPUT:
    if (accept_established_enabled_output_) {
      logger()->debug(
          "OUTPUT: accept ESTABLISHED optimization already enabled");
    } else {
      logger()->info("OUTPUT: Enabling ESTABLISHED optimization ...");
      accept_established_enabled_output_ = true;
      conntrack_mode_output_ = ConntrackModes::ON;
      programs_[std::make_pair(ModulesConstants::CONNTRACKLABEL_EGRESS,
                              ChainNameEnum::INVALID_EGRESS)]
          ->reload();
    }
  }
}

IptablesHorusEnum Iptables::getHorus() {
  if (this->horus_enabled) {
    return IptablesHorusEnum::ON;
  }
  return IptablesHorusEnum::OFF;
}

void Iptables::setHorus(const IptablesHorusEnum &value) {
  if (value == IptablesHorusEnum::ON) {
    this->horus_enabled = true;
  } else {
    this->horus_enabled = false;
  }
}



void Iptables::disableAcceptEstablished(Chain &chain) {
  switch (chain.getName()) {
  case ChainNameEnum::INPUT:
    if (!accept_established_enabled_input_) {
      logger()->debug(
          "INPUT: accept ESTABLISHED optimization already disabled");
    } else {
      logger()->info("INPUT: Disabling ESTABLISHED optimization ...");
      accept_established_enabled_input_ = false;
      conntrack_mode_input_ = ConntrackModes::OFF;
      programs_[std::make_pair(ModulesConstants::CONNTRACKLABEL_INGRESS,
                              ChainNameEnum::INVALID_INGRESS)]
          ->reload();
    }
  case ChainNameEnum::FORWARD:
    if (!accept_established_enabled_forward_) {
      logger()->debug(
          "FORWARD: accept ESTABLISHED optimization already disabled");
    } else {
      logger()->info("FORWARD: Disabling ESTABLISHED optimization ...");
      accept_established_enabled_forward_ = false;
      conntrack_mode_forward_ = ConntrackModes::OFF;
      programs_[std::make_pair(ModulesConstants::CONNTRACKLABEL_INGRESS,
                              ChainNameEnum::INVALID_INGRESS)]
          ->reload();
    }
  case ChainNameEnum::OUTPUT:
    if (!accept_established_enabled_output_) {
      logger()->debug(
          "OUTPUT: accept ESTABLISHED optimization already disabled");
    } else {
      logger()->info("OUTPUT: Disabling ESTABLISHED optimization ...");
      accept_established_enabled_output_ = false;
      conntrack_mode_output_ = ConntrackModes::OFF;
      programs_[std::make_pair(ModulesConstants::CONNTRACKLABEL_EGRESS,
                              ChainNameEnum::INVALID_INGRESS)]
          ->reload();
    }
  }
}

void Iptables::setConntrack(const IptablesConntrackEnum &value) {}

bool Iptables::isContrackActive() {
  return (conntrack_mode_ == ConntrackModes::ON ||
          conntrack_mode_ == ConntrackModes::OFF);
}

bool Iptables::fibLookupEnabled() {
  if (!fib_lookup_set_) {
    fib_lookup_enabled_ = true;
    if (getType() == CubeType::TC) {
      fib_lookup_enabled_ = false;
    }

    if (!polycube::polycubed::utils::check_kernel_version(
            REQUIRED_FIB_LOOKUP_KERNEL)) {
      logger()->info("kernel {0} is required for FIB_LOOKUP helper",
                     REQUIRED_FIB_LOOKUP_KERNEL);
      fib_lookup_enabled_ = false;
    }

    if (fib_lookup_enabled_)
      logger()->info("kernel {0} requirement satisfied for FIB_LOOKUP helper",
                     REQUIRED_FIB_LOOKUP_KERNEL);

    fib_lookup_set_ = true;
  }

  return fib_lookup_enabled_;
}