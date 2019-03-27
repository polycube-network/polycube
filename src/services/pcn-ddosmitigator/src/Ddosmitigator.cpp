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

#include "Ddosmitigator.h"
#include "Ddosmitigator_dp.h"

#include "polycube/services/utils.h"

using namespace polycube::service;

Ddosmitigator::Ddosmitigator(const std::string name,
                             const DdosmitigatorJsonObject &conf)
    : TransparentCube(conf.getBase(), {generate_code()}, {}) {
  logger()->info("Creating Ddosmitigator instance {0}", name);

  auto value = conf.getStats();
  addStats(conf.getStats());

  addBlacklistDstList(conf.getBlacklistDst());

  addBlacklistSrcList(conf.getBlacklistSrc());
}

Ddosmitigator::~Ddosmitigator() {}

void Ddosmitigator::update(const DdosmitigatorJsonObject &conf) {
  // This method updates all the object/parameter in Ddosmitigator object
  // specified in the conf JsonObject.
  // You can modify this implementation.

  TransparentCube::set_conf(conf.getBase());

  if (conf.statsIsSet()) {
    auto m = getStats();
    m->update(conf.getStats());
  }

  if (conf.blacklistDstIsSet()) {
    for (auto &i : conf.getBlacklistDst()) {
      auto ip = i.getIp();
      auto m = getBlacklistDst(ip);
      m->update(i);
    }
  }

  if (conf.blacklistSrcIsSet()) {
    for (auto &i : conf.getBlacklistSrc()) {
      auto ip = i.getIp();
      auto m = getBlacklistSrc(ip);
      m->update(i);
    }
  }
}

DdosmitigatorJsonObject Ddosmitigator::toJsonObject() {
  DdosmitigatorJsonObject conf;
  conf.setBase(TransparentCube::to_json());

  conf.setStats(getStats()->toJsonObject());

  for (auto &i : getBlacklistDstList()) {
    conf.addBlacklistDst(i->toJsonObject());
  }

  for (auto &i : getBlacklistSrcList()) {
    conf.addBlacklistSrc(i->toJsonObject());
  }

  return conf;
}

void Ddosmitigator::packet_in(polycube::service::Sense sense,
                              polycube::service::PacketInMetadata &md,
                              const std::vector<uint8_t> &packet) {
  logger()->info("packet in event");
}

std::string Ddosmitigator::generate_code() {
  return ddosmitigator_code;
}

std::vector<std::string> Ddosmitigator::generate_code_vector() {
  throw std::runtime_error("Method not implemented");
}

void Ddosmitigator::replaceAll(std::string &str, const std::string &from,
                               const std::string &to) {
  if (from.empty())
    return;
  size_t start_pos = 0;
  while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
    str.replace(start_pos, from.length(), to);
    start_pos += to.length();  // In case 'to' contains 'from', like replacing
                               // 'x' with 'yx'
  }
}

std::string Ddosmitigator::getCode() {
  std::string code = ddosmitigator_code;

  replaceAll(code, "_SRC_MATCH", (src_match_) ? "1" : "0");
  replaceAll(code, "_DST_MATCH", (dst_match_) ? "1" : "0");

  return code;
}

bool Ddosmitigator::reloadCode() {
  logger()->debug("reloadCode {0} ", is_code_changed_);
  if (is_code_changed_) {
    logger()->info("reloading code ...");
    reload(getCode());
    is_code_changed_ = false;
  }
  return true;
}

void Ddosmitigator::setSrcMatch(bool value) {
  logger()->debug("setSrcMatch {0} ", value);
  if (value != src_match_) {
    src_match_ = value;
    is_code_changed_ = true;
  }
}

void Ddosmitigator::setDstMatch(bool value) {
  if (value != dst_match_) {
    dst_match_ = value;
    is_code_changed_ = true;
  }
}

std::shared_ptr<Stats> Ddosmitigator::getStats() {
  logger()->debug("Stats getEntry");

  StatsJsonObject sjo;
  return std::shared_ptr<Stats>(new Stats(*this, sjo));
}

void Ddosmitigator::addStats(const StatsJsonObject &value) {}

void Ddosmitigator::replaceStats(const StatsJsonObject &conf) {
  throw std::runtime_error(
      "[Stats]: Method replaceStats not supported. Read Only");
}

void Ddosmitigator::delStats() {
  throw std::runtime_error(
      "[Stats]: Method removeEntry not supported. Read Only");
}

std::shared_ptr<BlacklistSrc> Ddosmitigator::getBlacklistSrc(
    const std::string &ip) {
  logger()->debug("BlacklistSrc getEntry");

  return std::shared_ptr<BlacklistSrc>(&blacklistsrc_.at(ip),
                                       [](BlacklistSrc *) {});
}

std::vector<std::shared_ptr<BlacklistSrc>>
Ddosmitigator::getBlacklistSrcList() {
  logger()->debug("BlacklistSrc get vector");

  std::vector<std::shared_ptr<BlacklistSrc>> blacklistsrc;

  for (auto &it : blacklistsrc_) {
    blacklistsrc.push_back(getBlacklistSrc(it.first));
  }

  return blacklistsrc;
}

void Ddosmitigator::addBlacklistSrc(const std::string &ip,
                                    const BlacklistSrcJsonObject &conf) {
  logger()->debug("BlacklistSrc create");

  try {
    logger()->debug("blacklist size {0} ", blacklistsrc_.size());
    // TODO check if src ip rules are already present
    // and reinject datapath with srcblacklist ps

    if (blacklistsrc_.size() >= 0) {
      setSrcMatch(true);
      reloadCode();
    }

    auto srcblacklist =
        get_percpuhash_table<uint32_t, uint64_t>("srcblacklist");
    srcblacklist.set(utils::ip_string_to_be_uint(ip), 0);
  } catch (...) {
    throw std::runtime_error("unable to add element to map");
  }

  BlacklistSrcJsonObject configuration;
  configuration.setIp(ip);

  blacklistsrc_.emplace(std::piecewise_construct, std::forward_as_tuple(ip),
                        std::forward_as_tuple(*this, configuration));
}

void Ddosmitigator::addBlacklistSrcList(
    const std::vector<BlacklistSrcJsonObject> &conf) {
  for (auto &i : conf) {
    std::string ip_ = i.getIp();
    addBlacklistSrc(ip_, i);
  }
}

void Ddosmitigator::replaceBlacklistSrc(const std::string &ip,
                                        const BlacklistSrcJsonObject &conf) {
  delBlacklistSrc(ip);
  std::string ip_ = conf.getIp();
  addBlacklistSrc(ip_, conf);
}

void Ddosmitigator::delBlacklistSrc(const std::string &ip) {
  logger()->debug("BlacklistSrc removeEntry");

  // ebpf map remove is performed in destructor
  blacklistsrc_.erase(ip);

  if (blacklistsrc_.size() == 0) {
    setSrcMatch(false);
    reloadCode();
  }
}

void Ddosmitigator::delBlacklistSrcList() {
  logger()->debug("BlacklistSrc remove");

  // ebpf maps remove performed in destructor
  blacklistsrc_.clear();

  if (blacklistsrc_.size() == 0) {
    setSrcMatch(false);
    reloadCode();
  }
}

std::shared_ptr<BlacklistDst> Ddosmitigator::getBlacklistDst(
    const std::string &ip) {
  logger()->debug("BlacklistDst getEntry");

  return std::shared_ptr<BlacklistDst>(&blacklistdst_.at(ip),
                                       [](BlacklistDst *) {});
}

std::vector<std::shared_ptr<BlacklistDst>>
Ddosmitigator::getBlacklistDstList() {
  logger()->debug("BlacklistDst get vector");

  std::vector<std::shared_ptr<BlacklistDst>> blacklistdst;

  for (auto &it : blacklistdst_) {
    blacklistdst.push_back(getBlacklistDst(it.first));
  }

  return blacklistdst;
}

void Ddosmitigator::addBlacklistDst(const std::string &ip,
                                    const BlacklistDstJsonObject &conf) {
  logger()->debug("BlacklistDst create");

  try {
    // TODO check if dst ip rules are already present
    // and reinject datapath with dstblacklist ps

    if (blacklistdst_.size() >= 0) {
      setDstMatch(true);
      reloadCode();
    }

    auto dstblacklist =
        get_percpuhash_table<uint32_t, uint64_t>("dstblacklist");
    dstblacklist.set(utils::ip_string_to_be_uint(ip), 0);
  } catch (...) {
    throw std::runtime_error("unable to add element to map");
  }

  BlacklistDstJsonObject configuration;
  configuration.setIp(ip);

  blacklistdst_.emplace(std::piecewise_construct, std::forward_as_tuple(ip),
                        std::forward_as_tuple(*this, configuration));
}

void Ddosmitigator::addBlacklistDstList(
    const std::vector<BlacklistDstJsonObject> &conf) {
  for (auto &i : conf) {
    std::string ip_ = i.getIp();
    delBlacklistDst(ip_);
  }
}

void Ddosmitigator::replaceBlacklistDst(const std::string &ip,
                                        const BlacklistDstJsonObject &conf) {
  delBlacklistDst(ip);
  std::string ip_ = conf.getIp();
  addBlacklistDst(ip_, conf);
}

void Ddosmitigator::delBlacklistDst(const std::string &ip) {
  logger()->debug("BlacklistDst removeEntry");

  // ebpf map remove is performed in destructor
  blacklistdst_.erase(ip);

  if (blacklistdst_.size() == 0) {
    setDstMatch(false);
    reloadCode();
  }
}

void Ddosmitigator::delBlacklistDstList() {
  logger()->debug("BlacklistDst remove");

  // ebpf maps remove performed in destructor
  blacklistdst_.clear();

  if (blacklistdst_.size() == 0) {
    setDstMatch(false);
    reloadCode();
  }
}
