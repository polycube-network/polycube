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


#include "TrafficClass.h"
#include "Classifier.h"


TrafficClass::TrafficClass(Classifier &parent,
                           const TrafficClassJsonObject &conf)
    : TrafficClassBase(parent) {
  id_ = conf.getId();
  priority_ = conf.getPriority();
  direction_ = conf.getDirection();

  smac_set_ = conf.smacIsSet();
  if (smac_set_) {
    smac_ = conf.getSmac();
  }

  dmac_set_ = conf.dmacIsSet();
  if (dmac_set_) {
    dmac_ = conf.getDmac();
  }

  ethtype_set_ = conf.ethtypeIsSet();
  if (ethtype_set_) {
    ethtype_ = conf.getEthtype();
  }

  srcip_set_ = conf.srcipIsSet();
  if (srcip_set_) {
    srcip_ = conf.getSrcip();
  }

  dstip_set_ = conf.dstipIsSet();
  if (dstip_set_) {
    dstip_ = conf.getDstip();
  }

  l4proto_set_ = conf.l4protoIsSet();
  if (l4proto_set_) {
    l4proto_ = conf.getL4proto();
  }

  sport_set_ = conf.sportIsSet();
  if (sport_set_) {
    sport_ = conf.getSport();
  }

  dport_set_ = conf.dportIsSet();
  if (dport_set_) {
    dport_ = conf.getDport();
  }

  if (!isValid()) {
    throw std::runtime_error(std::string("Invalid traffic class ") +
                             toString());
  }
}

TrafficClass::~TrafficClass() {}

TrafficClassJsonObject TrafficClass::toJsonObject() {
  TrafficClassJsonObject conf;

  conf.setId(getId());
  conf.setPriority(getPriority());
  conf.setDirection(getDirection());
  if (smac_set_) {
    conf.setSmac(smac_);
  }
  if (dmac_set_) {
    conf.setDmac(dmac_);
  }
  if (ethtype_set_) {
    conf.setEthtype(ethtype_);
  }
  if (srcip_set_) {
    conf.setSrcip(srcip_);
  }
  if (dstip_set_) {
    conf.setDstip(dstip_);
  }
  if (l4proto_set_) {
    conf.setL4proto(l4proto_);
  }
  if (sport_set_) {
    conf.setSport(sport_);
  }
  if (dport_set_) {
    conf.setDport(dport_);
  }

  return conf;
}

uint32_t TrafficClass::getId() {
  return id_;
}

uint32_t TrafficClass::getPriority() {
  return priority_;
}

void TrafficClass::setPriority(const uint32_t &value) {
  if (priority_ == value) {
    return;
  }

  priority_ = value;

  parent_.updateProgram(getDirection());

  logger()->info("Traffic class updated {0}", toString());
}

TrafficClassDirectionEnum TrafficClass::getDirection() {
  return direction_;
}

void TrafficClass::setDirection(const TrafficClassDirectionEnum &value) {
  if (direction_ == value) {
    return;
  }

  direction_ = value;

  parent_.updateProgram(TrafficClassDirectionEnum::BOTH);

  logger()->info("Traffic class updated {0}", toString());
}

std::string TrafficClass::getSmac() {
  if (!smac_set_) {
    throw std::runtime_error("Field not set");
  }

  return smac_;
}

void TrafficClass::setSmac(const std::string &value) {
  if (smac_set_ && smac_ == value) {
    return;
  }

  bool old_set_ = smac_set_;
  auto old_val_ = smac_;

  smac_ = value;
  smac_set_ = true;

  if (!isValid()) {
    smac_set_ = old_set_;
    smac_ = old_val_;

    throw std::runtime_error(std::string("Invalid traffic class ") +
                             toString());
  }

  parent_.updateProgram(getDirection());

  logger()->info("Traffic class updated {0}", toString());
}

bool TrafficClass::smacIsSet() {
  return smac_set_;
}

std::string TrafficClass::getDmac() {
  if (!dmac_set_) {
    throw std::runtime_error("Field not set");
  }

  return dmac_;
}

void TrafficClass::setDmac(const std::string &value) {
  if (dmac_set_ && dmac_ == value) {
    return;
  }

  bool old_set_ = dmac_set_;
  auto old_val_ = dmac_;

  dmac_ = value;
  dmac_set_ = true;

  if (!isValid()) {
    dmac_set_ = old_set_;
    dmac_ = old_val_;

    throw std::runtime_error(std::string("Invalid traffic class ") +
                             toString());
  }

  parent_.updateProgram(getDirection());

  logger()->info("Traffic class updated {0}", toString());
}

bool TrafficClass::dmacIsSet() {
  return dmac_set_;
}

TrafficClassEthtypeEnum TrafficClass::getEthtype() {
  if (!ethtype_set_) {
    throw std::runtime_error("Field not set");
  }

  return ethtype_;
}

void TrafficClass::setEthtype(const TrafficClassEthtypeEnum &value) {
  if (ethtype_set_ && ethtype_ == value) {
    return;
  }

  bool old_set_ = ethtype_set_;
  auto old_val_ = ethtype_;

  ethtype_ = value;
  ethtype_set_ = true;

  if (!isValid()) {
    ethtype_set_ = old_set_;
    ethtype_ = old_val_;

    throw std::runtime_error(std::string("Invalid traffic class ") +
                             toString());
  }

  parent_.updateProgram(getDirection());

  logger()->info("Traffic class updated {0}", toString());
}

bool TrafficClass::ethtypeIsSet() {
  return ethtype_set_;
}

std::string TrafficClass::getSrcip() {
  if (!srcip_set_) {
    throw std::runtime_error("Field not set");
  }

  return srcip_;
}

void TrafficClass::setSrcip(const std::string &value) {
  if (srcip_set_ && srcip_ == value) {
    return;
  }

  bool old_set_ = srcip_set_;
  auto old_val_ = srcip_;

  srcip_ = value;
  srcip_set_ = true;

  if (!isValid()) {
    srcip_set_ = old_set_;
    srcip_ = old_val_;

    throw std::runtime_error(std::string("Invalid traffic class ") +
                             toString());
  }

  parent_.updateProgram(getDirection());

  logger()->info("Traffic class updated {0}", toString());
}

bool TrafficClass::srcipIsSet() {
  return srcip_set_;
}

std::string TrafficClass::getDstip() {
  if (!dstip_set_) {
    throw std::runtime_error("Field not set");
  }

  return dstip_;
}

void TrafficClass::setDstip(const std::string &value) {
  if (dstip_set_ && dstip_ == value) {
    return;
  }

  bool old_set_ = dstip_set_;
  auto old_val_ = dstip_;

  dstip_ = value;
  dstip_set_ = true;

  if (!isValid()) {
    dstip_set_ = old_set_;
    dstip_ = old_val_;

    throw std::runtime_error(std::string("Invalid traffic class ") +
                             toString());
  }

  parent_.updateProgram(getDirection());

  logger()->info("Traffic class updated {0}", toString());
}

bool TrafficClass::dstipIsSet() {
  return dstip_set_;
}

TrafficClassL4protoEnum TrafficClass::getL4proto() {
  if (!l4proto_set_) {
    throw std::runtime_error("Field not set");
  }

  return l4proto_;
}

void TrafficClass::setL4proto(const TrafficClassL4protoEnum &value) {
  if (l4proto_set_ && l4proto_ == value) {
    return;
  }

  bool old_set_ = l4proto_set_;
  auto old_val_ = l4proto_;

  l4proto_ = value;
  l4proto_set_ = true;

  if (!isValid()) {
    l4proto_set_ = old_set_;
    l4proto_ = old_val_;

    throw std::runtime_error(std::string("Invalid traffic class ") +
                             toString());
  }

  parent_.updateProgram(getDirection());

  logger()->info("Traffic class updated {0}", toString());
}

bool TrafficClass::l4protoIsSet() {
  return l4proto_set_;
}

uint16_t TrafficClass::getSport() {
  if (!sport_set_) {
    throw std::runtime_error("Field not set");
  }

  return sport_;
}

void TrafficClass::setSport(const uint16_t &value) {
  if (sport_set_ && sport_ == value) {
    return;
  }

  bool old_set_ = sport_set_;
  auto old_val_ = sport_;

  sport_ = value;
  sport_set_ = true;

  if (!isValid()) {
    sport_set_ = old_set_;
    sport_ = old_val_;

    throw std::runtime_error(std::string("Invalid traffic class ") +
                             toString());
  }

  parent_.updateProgram(getDirection());

  logger()->info("Traffic class updated {0}", toString());
}

bool TrafficClass::sportIsSet() {
  return sport_set_;
}

uint16_t TrafficClass::getDport() {
  if (!dport_set_) {
    throw std::runtime_error("Field not set");
  }

  return dport_;
}

void TrafficClass::setDport(const uint16_t &value) {
  if (dport_set_ && dport_ == value) {
    return;
  }

  bool old_set_ = dport_set_;
  auto old_val_ = dport_;

  dport_ = value;
  dport_set_ = true;

  if (!isValid()) {
    dport_set_ = old_set_;
    dport_ = old_val_;

    throw std::runtime_error(std::string("Invalid traffic class ") +
                             toString());
  }

  parent_.updateProgram(getDirection());

  logger()->info("Traffic class updated {0}", toString());
}

bool TrafficClass::dportIsSet() {
  return dport_set_;
}

bool TrafficClass::operator<=(const TrafficClass &other) {
  return this->priority_ <= other.priority_;
}

std::string TrafficClass::toString() {
  return toJsonObject().toJson().dump();
}

bool TrafficClass::isValid() {
  if (ethtype_set_ && ethtype_ != TrafficClassEthtypeEnum::IP) {
    if (srcip_set_ || dstip_set_ || l4proto_set_ || sport_set_ || dport_set_) {
      return false;
    }
  }

  if (l4proto_set_ && l4proto_ != TrafficClassL4protoEnum::TCP &&
      l4proto_ != TrafficClassL4protoEnum::UDP) {
    if (sport_set_ || dport_set_) {
      return false;
    }
  }

  return true;
}
