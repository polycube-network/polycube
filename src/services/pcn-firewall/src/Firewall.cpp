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

#include "Firewall.h"
#include "Firewall_dp.h"

Firewall::Firewall(const std::string name, const FirewallJsonObject &conf)
    : TransparentCube(conf.getBase(), {firewall_code}, {firewall_code}), FirewallBase(name) {
  logger()->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [Firewall] [%n] [%l] %v");
  logger()->info("Creating Firewall instance");

  update(conf);

  /*Creating default INGRESS and EGRESS chains.*/
  ChainJsonObject chain;
  addChain(ChainNameEnum::INGRESS, chain);
  addChain(ChainNameEnum::EGRESS, chain);
  logger()->debug("Ingress and Egress chain added");

  ingress_programs.resize(ModulesConstants::NR_INITIAL_MODULES + ModulesConstants::NR_MODULES, nullptr);
  egress_programs.resize(ModulesConstants::NR_INITIAL_MODULES + ModulesConstants::NR_MODULES, nullptr);

  /* Initialize ingress programs */
  ingress_programs[ModulesConstants::PARSER] =
    new Firewall::Parser(ModulesConstants::PARSER, ChainNameEnum::INGRESS, *this);
  ingress_programs[ModulesConstants::CONNTRACKLABEL] =
    new Firewall::ConntrackLabel(ModulesConstants::CONNTRACKLABEL, ChainNameEnum::INGRESS, *this);
  ingress_programs[ModulesConstants::CHAINFORWARDER] =
    new Firewall::ChainForwarder(ModulesConstants::CHAINFORWARDER, ChainNameEnum::INGRESS, *this);

  egress_programs[ModulesConstants::PARSER] =
    new Firewall::Parser(ModulesConstants::PARSER, ChainNameEnum::EGRESS, *this);
  egress_programs[ModulesConstants::CONNTRACKLABEL] =
    new Firewall::ConntrackLabel(ModulesConstants::CONNTRACKLABEL, ChainNameEnum::EGRESS, *this);
  egress_programs[ModulesConstants::CHAINFORWARDER] =
    new Firewall::ChainForwarder(ModulesConstants::CHAINFORWARDER, ChainNameEnum::EGRESS, *this);

  /*
   * 3 modules in the beginning
   * NR_MODULES ingress chain
   * NR_MODULES egress chain
   * NR_MODULES second ingress chain
   * NR_MODULES second egress chain
   * Default Action
   * Conntrack Label
   */

  ingress_programs[ModulesConstants::DEFAULTACTION] =
    new Firewall::DefaultAction(ModulesConstants::DEFAULTACTION,
                                ChainNameEnum::INGRESS,*this);

  ingress_programs[ModulesConstants::CONNTRACKTABLEUPDATE] =
    new Firewall::ConntrackTableUpdate(ModulesConstants::CONNTRACKTABLEUPDATE,
                                       ChainNameEnum::INGRESS, *this);

  egress_programs[ModulesConstants::DEFAULTACTION] =
    new Firewall::DefaultAction(ModulesConstants::DEFAULTACTION,
                                ChainNameEnum::EGRESS, *this);

  egress_programs[ModulesConstants::CONNTRACKTABLEUPDATE] =
    new Firewall::ConntrackTableUpdate(ModulesConstants::CONNTRACKTABLEUPDATE,
                                       ChainNameEnum::EGRESS, *this);
}

Firewall::~Firewall() {
  logger()->info("[{0}] Destroying firewall...", get_name());
  chains_.clear();

  // Delete all eBPF programs
  for (auto &i : ingress_programs) {
    if (i) {
      delete i;
    }
  }

  for (auto &i : egress_programs) {
    if (i) {
      delete i;
    }
  }

  TransparentCube::dismount();
}

void Firewall::packet_in(polycube::service::Direction direction,
                                      polycube::service::PacketInMetadata &md,
                                      const std::vector<uint8_t> &packet) {
  switch (direction) {
  case polycube::service::Direction::INGRESS:
    logger()->info("packet in event from ingress program");
    break;
  case polycube::service::Direction::EGRESS:
    logger()->info("packet in event from egress program");
    break;
  }
}

bool Firewall::getInteractive() {
  return interactive_;
}

void Firewall::setInteractive(const bool &value) {
  interactive_ = value;
}

FirewallAcceptEstablishedEnum Firewall::getAcceptEstablished() {
  if (this->conntrackMode == ConntrackModes::AUTOMATIC) {
    return FirewallAcceptEstablishedEnum::ON;
  }
  return FirewallAcceptEstablishedEnum::OFF;
}

void Firewall::setAcceptEstablished(
    const FirewallAcceptEstablishedEnum &value) {
  if (conntrackMode == ConntrackModes::DISABLED) {
    throw std::runtime_error("Please enable conntrack first.");
  }

  if (value == FirewallAcceptEstablishedEnum::ON &&
      conntrackMode != ConntrackModes::AUTOMATIC) {
    conntrackMode = ConntrackModes::AUTOMATIC;

    ingress_programs[ModulesConstants::CONNTRACKLABEL]->reload();
    egress_programs[ModulesConstants::CONNTRACKLABEL]->reload();
    return;
  }

  if (value == FirewallAcceptEstablishedEnum::OFF &&
      conntrackMode != ConntrackModes::MANUAL) {
    conntrackMode = ConntrackModes::MANUAL;

    ingress_programs[ModulesConstants::CONNTRACKLABEL]->reload();
    egress_programs[ModulesConstants::CONNTRACKLABEL]->reload();
    return;
  }
}

FirewallConntrackEnum Firewall::getConntrack() {
  if (this->conntrackMode == ConntrackModes::DISABLED) {
    return FirewallConntrackEnum::OFF;
  }
  return FirewallConntrackEnum::ON;
}

void Firewall::setConntrack(const FirewallConntrackEnum &value) {
  if (value == FirewallConntrackEnum::OFF &&
      this->conntrackMode != ConntrackModes::DISABLED) {
    this->conntrackMode = ConntrackModes::DISABLED;

    // The parser has to be reloaded to skip the conntrack
    ingress_programs[ModulesConstants::CONNTRACKTABLEUPDATE]->reload();
    egress_programs[ModulesConstants::CONNTRACKTABLEUPDATE]->reload();

    ingress_programs[ModulesConstants::PARSER]->reload();
    egress_programs[ModulesConstants::PARSER]->reload();

    if (ingress_programs[ModulesConstants::CONNTRACKLABEL]) {
      delete ingress_programs[ModulesConstants::CONNTRACKLABEL];
      ingress_programs[ModulesConstants::CONNTRACKLABEL] = nullptr;
    }

    if (egress_programs[ModulesConstants::CONNTRACKLABEL]) {
      delete egress_programs[ModulesConstants::CONNTRACKLABEL];
      egress_programs[ModulesConstants::CONNTRACKLABEL] = nullptr;
    }

    return;
  }

  if (value == FirewallConntrackEnum::ON &&
      this->conntrackMode == ConntrackModes::DISABLED) {
    this->conntrackMode = ConntrackModes::MANUAL;
    ingress_programs[ModulesConstants::CONNTRACKLABEL] =
      new Firewall::ConntrackLabel(1, ChainNameEnum::INGRESS, *this);
    egress_programs[ModulesConstants::CONNTRACKLABEL] =
      new Firewall::ConntrackLabel(1, ChainNameEnum::EGRESS, *this);

    ingress_programs[ModulesConstants::CONNTRACKTABLEUPDATE]->reload();
    egress_programs[ModulesConstants::CONNTRACKTABLEUPDATE]->reload();

    ingress_programs[ModulesConstants::PARSER]->reload();
    egress_programs[ModulesConstants::PARSER]->reload();
    return;
  }
}

void Firewall::reload_chain(ChainNameEnum chain) {
  if (chain == ChainNameEnum::INGRESS) {
    for (auto &i : ingress_programs) {
      i->reload();
    }
  } else if (chain == ChainNameEnum::EGRESS) {
    for (auto &i : egress_programs) {
      i->reload();
    }
  }
}

void Firewall::reload_all() {
  reload_chain(ChainNameEnum::INGRESS);
  reload_chain(ChainNameEnum::EGRESS);
}

bool Firewall::isContrackActive() {
  return (conntrackMode == ConntrackModes::AUTOMATIC ||
          conntrackMode == ConntrackModes::MANUAL);
}

std::shared_ptr<SessionTable> Firewall::getSessionTable(
    const std::string &src, const std::string &dst, const std::string &l4proto,
    const uint16_t &sport, const uint16_t &dport) {
  throw std::runtime_error("[SessionTable]: Method getEntry not allowed");
}

std::vector<std::shared_ptr<SessionTable>> Firewall::getSessionTableList() {
  std::vector<std::pair<ct_k, ct_v>> connections =
      dynamic_cast<Firewall::ConntrackLabel *>(
          ingress_programs[ModulesConstants::CONNTRACKLABEL])
          ->getMap();

  std::vector<std::shared_ptr<SessionTable>> sessionTable;
  SessionTableJsonObject conf;

  for (auto &connection : connections) {
    auto key = connection.first;
    auto value = connection.second;

    conf.setSrc(utils::nbo_uint_to_ip_string(key.srcIp));
    conf.setDst(utils::nbo_uint_to_ip_string(key.dstIp));
    conf.setL4proto(ChainRule::protocol_from_int_to_string(key.l4proto));
    conf.setSport(ntohs(key.srcPort));
    conf.setDport(ntohs(key.dstPort));
    conf.setState(SessionTable::state_from_number_to_string(value.state));
    conf.setEta(
        SessionTable::from_ttl_to_eta(value.ttl, value.state, key.l4proto));

    sessionTable.push_back(
        std::shared_ptr<SessionTable>(new SessionTable(*this, conf)));
  }
  return sessionTable;
}

void Firewall::addSessionTable(const std::string &src, const std::string &dst,
                               const std::string &l4proto,
                               const uint16_t &sport, const uint16_t &dport,
                               const SessionTableJsonObject &conf) {
  throw std::runtime_error("[SessionTable]: Method create not allowed");
}

void Firewall::addSessionTableList(
    const std::vector<SessionTableJsonObject> &conf) {
  throw std::runtime_error("[SessionTable]: Method create not allowed");
}

void Firewall::replaceSessionTable(const std::string &src,
                                   const std::string &dst,
                                   const std::string &l4proto,
                                   const uint16_t &sport, const uint16_t &dport,
                                   const SessionTableJsonObject &conf) {
  throw std::runtime_error("[SessionTable]: Method replace not allowed");
}

void Firewall::delSessionTable(const std::string &src, const std::string &dst,
                               const std::string &l4proto,
                               const uint16_t &sport, const uint16_t &dport) {
  throw std::runtime_error("[SessionTable]: Method remove not allowed");
}

void Firewall::delSessionTableList() {
  throw std::runtime_error("[SessionTable]: Method remove not allowed");
}

std::shared_ptr<Chain> Firewall::getChain(const ChainNameEnum &name) {
  // This method retrieves the pointer to Chain object specified by its keys.
  if (chains_.count(name) == 0) {
    throw std::runtime_error("There is no chain " +
                             ChainJsonObject::ChainNameEnum_to_string(name));
  }

  return std::shared_ptr<Chain>(&chains_.at(name), [](Chain *) {});
}

std::vector<std::shared_ptr<Chain>> Firewall::getChainList() {
  std::vector<std::shared_ptr<Chain>> chains;

  for (auto &it : chains_) {
    chains.push_back(getChain(it.first));
  }

  return chains;
}

void Firewall::addChain(const ChainNameEnum &name,
                        const ChainJsonObject &conf) {
  // This method creates the actual Chain object given thee key param.
  ChainJsonObject namedChain = conf;
  namedChain.setName(name);
  if (chains_.count(name) != 0) {
    throw std::runtime_error("There is already a chain " +
                             ChainJsonObject::ChainNameEnum_to_string(name));
  }

  chains_.emplace(std::piecewise_construct, std::forward_as_tuple(name),
                  std::forward_as_tuple(*this, namedChain));
}

void Firewall::delChain(const ChainNameEnum &name) {
  throw std::runtime_error("Method not supported.");
}

void Firewall::delChainList() {
  throw std::runtime_error("Method not supported.");
}
