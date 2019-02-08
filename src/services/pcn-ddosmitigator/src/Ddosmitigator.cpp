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
    : Cube(name, {generate_code()}, {}, type, conf.getPolycubeLoglevel()) {
  logger()->info("Creating Ddosmitigator instance {0}", name);

  auto value = conf.getStats();
  addStats(conf.getStats());

  addBlacklistDstList(conf.getBlacklistDst());
  addPortsList(conf.getPorts());
  if (conf.activePortIsSet()) {
    setActivePort(conf.getActivePort());
  }

  addBlacklistSrcList(conf.getBlacklistSrc());
  if (conf.redirectPortIsSet()) {
    setRedirectPort(conf.getRedirectPort());
  }
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

  if (conf.portsIsSet()) {
    for (auto &i : conf.getPorts()) {
      auto name = i.getName();
      auto m = getPorts(name);
      m->update(i);
    }
  }

  if (conf.activePortIsSet()) {
    setActivePort(conf.getActivePort());
  }

  if (conf.blacklistSrcIsSet()) {
    for (auto &i : conf.getBlacklistSrc()) {
      auto ip = i.getIp();
      auto m = getBlacklistSrc(ip);
      m->update(i);
    }
  }

  if (conf.redirectPortIsSet()) {
    setRedirectPort(conf.getRedirectPort());
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

  for (auto &i : getPortsList()) {
    conf.addPorts(i->toJsonObject());
  }

  conf.setActivePort(getActivePort());

  conf.setType(getType());

  for (auto &i : getBlacklistSrcList()) {
    conf.addBlacklistSrc(i->toJsonObject());
  }

  conf.setRedirectPort(getRedirectPort());
  return conf;
}

std::string Ddosmitigator::generate_code() {
  return ddosmitigator_code;
}

std::vector<std::string> Ddosmitigator::generate_code_vector() {
  throw std::runtime_error("Method not implemented");
}

void Ddosmitigator::packet_in(Ports &port,
                              polycube::service::PacketInMetadata &md,
                              const std::vector<uint8_t> &packet) {
  logger()->info("Ddosmitigator {0}: Packet received from port {1}",
                 Cube::get_name(), port.name());
}

std::string Ddosmitigator::getActivePort() {
  // This method retrieves the activePort value.
  return in_port_str_;
}

void Ddosmitigator::setActivePort(const std::string &value) {
  // This method set the activePort value.
  setInPort(value);
  reloadCode();
}

std::string Ddosmitigator::getRedirectPort() {
  // This method retrieves the redirectPort value.
  return out_port_str_;
}

void Ddosmitigator::setRedirectPort(const std::string &value) {
  // This method set the redirectPort value.
  setOutPort(value);
  reloadCode();
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
  replaceAll(code, "_REDIRECT", (redirect_) ? "1" : "0");

  replaceAll(code, "_IN_PORT", std::to_string(in_port_));
  replaceAll(code, "_OUT_PORT", std::to_string(out_port_));

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

void Ddosmitigator::setInPort(std::string portName) {
  // todo check
  auto p = get_port(portName);

  int index = p->index();

  in_port_ = index;
  in_port_str_ = portName;

  is_code_changed_ = true;
}

void Ddosmitigator::setOutPort(std::string portName) {
  if (portName == "" && out_port_is_set_ == true) {
    out_port_ = 1;
    out_port_str_ = "";
    out_port_is_set_ = false;
    redirect_ = false;
    is_code_changed_ = true;
    return;
  }

  auto p = get_port(portName);

  int index = p->index();

  out_port_ = index;
  out_port_str_ = portName;
  out_port_is_set_ = true;
  redirect_ = true;
  is_code_changed_ = true;
}

void Ddosmitigator::addPort(std::string name) {
  if (get_ports().size() == 1) {
    setInPort(name);
    reloadCode();
  }
}

void Ddosmitigator::rmPort(std::string name) {
  if (get_ports().size() < 2) {
    setOutPort("");
    // todo lookup first port name
    // parent.setInPort();
    reloadCode();
  }
}
