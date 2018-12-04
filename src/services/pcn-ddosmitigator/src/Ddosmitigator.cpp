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

Ddosmitigator::Ddosmitigator(const std::string name,
                             const DdosmitigatorJsonObject &conf, CubeType type)
    : TransparentCube(name, {generate_code()}, {}, type,
                      conf.getPolycubeLoglevel()) {
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

  if (conf.statsIsSet()) {
    auto m = getStats();
    m->update(conf.getStats());
  }

  if (conf.loglevelIsSet()) {
    setLoglevel(conf.getLoglevel());
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

  conf.setStats(getStats()->toJsonObject());
  conf.setName(getName());
  conf.setLoglevel(getLoglevel());

  for (auto &i : getBlacklistDstList()) {
    conf.addBlacklistDst(i->toJsonObject());
  }

  conf.setUuid(getUuid());

  conf.setType(getType());

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
