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

// Modify these methods with your own implementation

#include "Frontend.h"
#include "Lbdsr.h"

Frontend::Frontend(Lbdsr &parent, const FrontendJsonObject &conf)
    : parent_(parent) {
  logger()->info("Creating Frontend instance");

  if (conf.vipIsSet()) {
    vip_ = conf.getVip();
  }

  if (conf.macIsSet()) {
    mac_ = conf.getMac();
  }
}

Frontend::Frontend(Lbdsr &parent) : parent_(parent) {}

Frontend::~Frontend() {}

void Frontend::update(const FrontendJsonObject &conf) {
  // This method updates all the object/parameter in Frontend object specified
  // in the conf JsonObject.
  // You can modify this implementation.

  if (conf.vipIsSet()) {
    setVip(conf.getVip());
  }

  if (conf.macIsSet()) {
    setMac(conf.getMac());
  }
}

FrontendJsonObject Frontend::toJsonObject() {
  FrontendJsonObject conf;

  conf.setVip(getVip());

  conf.setMac(getMac());

  return conf;
}

void Frontend::create(Lbdsr &parent, const FrontendJsonObject &conf) {
  // This method creates the actual Frontend object given thee key param.
  // Please remember to call here the create static method for all sub-objects
  // of Frontend.
  parent.logger()->debug("[Frontend] Received request to create new Frontend");
  parent.frontend_ = std::make_shared<Frontend>(parent, conf);
}

std::shared_ptr<Frontend> Frontend::getEntry(Lbdsr &parent) {
  // This method retrieves the pointer to Frontend object specified by its keys.
  if (parent.frontend_ != nullptr) {
    return parent.frontend_;
  } else {
    return std::make_shared<Frontend>(parent);
  }
}

void Frontend::removeEntry(Lbdsr &parent) {
  // This method removes the single Frontend object specified by its keys.
  // Remember to call here the remove static method for all-sub-objects of
  // Frontend.
  if (parent.frontend_ != nullptr) {
    parent.frontend_ = nullptr;
  } else {
    throw std::runtime_error("There is no frontend in this LBDSR");
  }
}

std::string Frontend::getVip() {
  // This method retrieves the vip value.
  return this->vip_;
}

void Frontend::setVip(const std::string &value) {
  // This method set the vip value.

  // convert value to hexbe string ip
  logger()->info("Set Vip request str: {0} ", value);
  std::string vip_hexbe = utils::ip_string_to_hexbe_string(value);
  logger()->trace("Set Vip request str: {0} hexbe: {1} ", value, vip_hexbe);

  // if success set vip and vip_hexbe_string in parent
  parent_.setVipHexbe(vip_hexbe);
  this->vip_ = value;
}

std::string Frontend::getMac() {
  // This method retrieves the mac value.
  return this->mac_;
}

void Frontend::setMac(const std::string &value) {
  // This method set the mac value.

  try {
    // convert value to hexbe string ip
    logger()->info("Set Mac request str: {0} ", value);
    std::string mac_hexbe = utils::mac_string_to_hexbe_string(value);
    logger()->trace("Set Mac request str: {0} hexbe: {1} ", value, mac_hexbe);

    // if success set vip and mac_hexbe_string in parent
    parent_.setMacHexbe(mac_hexbe);
    this->mac_ = value;
  } catch (...) {
    throw std::runtime_error("no valid mac");
  }
}

std::shared_ptr<spdlog::logger> Frontend::logger() {
  return parent_.logger();
}
