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


#include "Policer.h"
#include "Policer_dp.h"


Policer::Policer(const std::string name, const PolicerJsonObject &conf)
  : TransparentCube(conf.getBase(), {policer_code}, {policer_code}), PolicerBase(name) {
  logger()->info("Creating Policer instance");

  default_contract_ =
      std::make_shared<DefaultContract>(*this, conf.getDefaultContract());
  addContractList(conf.getContract());
}

Policer::~Policer() {
  logger()->info("Destroying Policer instance");
}

void Policer::packet_in(polycube::service::Direction direction,
    polycube::service::PacketInMetadata &md,
    const std::vector<uint8_t> &packet) {
    logger()->debug("Packet received");
}

std::shared_ptr<DefaultContract> Policer::getDefaultContract() {
  return default_contract_;
}

void Policer::addDefaultContract(const DefaultContractJsonObject &value) {
  throw std::runtime_error("Cannot add a default contract");
}

void Policer::replaceDefaultContract(const DefaultContractJsonObject &conf) {
  throw std::runtime_error("Cannot replace the default contract");
}

void Policer::delDefaultContract() {
  throw std::runtime_error("Cannot remove the default contract");
}

std::shared_ptr<Contract> Policer::getContract(const uint32_t &trafficClass) {
  if (contracts_.count(trafficClass) == 0) {
    throw std::runtime_error("No contract registered for the given class");
  }

  return contracts_.at(trafficClass);
}

std::vector<std::shared_ptr<Contract>> Policer::getContractList() {
  std::vector<std::shared_ptr<Contract>> contracts_v;

  contracts_v.reserve(contracts_.size());

  for (auto const &entry : contracts_) {
    contracts_v.push_back(entry.second);
  }

  return contracts_v;
}

void Policer::addContract(const uint32_t &trafficClass,
                          const ContractJsonObject &conf) {
  if (contracts_.count(trafficClass) != 0) {
    throw std::runtime_error("Contract for the given class already registered");
  }

  if (contracts_.size() == MAX_CONTRACTS) {
    throw std::runtime_error("Maximum number of contracts reached");
  }

  contracts_[trafficClass] = std::make_shared<Contract>(*this, conf);
}

// Basic default implementation, place your extension here (if needed)
void Policer::addContractList(const std::vector<ContractJsonObject> &conf) {
  // call default implementation in base class
  PolicerBase::addContractList(conf);
}

// Basic default implementation, place your extension here (if needed)
void Policer::replaceContract(const uint32_t &trafficClass,
                              const ContractJsonObject &conf) {
  // call default implementation in base class
  PolicerBase::replaceContract(trafficClass, conf);
}

void Policer::delContract(const uint32_t &trafficClass) {
  if (contracts_.count(trafficClass) == 0) {
    throw std::runtime_error("No contract registered for the given class");
  }

  contracts_.erase(trafficClass);
}

// Basic default implementation, place your extension here (if needed)
void Policer::delContractList() {
  // call default implementation in base class
  PolicerBase::delContractList();
}