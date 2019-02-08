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

#include "Backend.h"
#include "Lbdsr.h"

Backend::Backend(Lbdsr &parent, const BackendJsonObject &conf)
    : parent_(parent) {
  logger()->info("Creating Backend instance");

  if (conf.poolIsSet()) {
    Backend::addPoolList(conf.getPool());
  }
}

Backend::Backend(Lbdsr &parent) : parent_(parent) {}

Backend::~Backend() {}

void Backend::update(const BackendJsonObject &conf) {
  // This method updates all the object/parameter in Backend object specified in
  // the conf JsonObject.
  // You can modify this implementation.

  if (conf.poolIsSet()) {
    for (auto &i : conf.getPool()) {
      auto id = i.getId();
      auto m = getPool(id);
      m->update(i);
    }
  }
}

BackendJsonObject Backend::toJsonObject() {
  BackendJsonObject conf;

  // Remove comments when you implement all sub-methods
  for (auto &i : getPoolList()) {
    conf.addBackendPool(i->toJsonObject());
  }

  return conf;
}

void Backend::create(Lbdsr &parent, const BackendJsonObject &conf) {
  // This method creates the actual Backend object given thee key param.
  // Please remember to call here the create static method for all sub-objects
  // of Backend.
  parent.logger()->debug("[Backend] Received request to create new Backend");
  parent.backend_ = std::make_shared<Backend>(parent, conf);
}

std::shared_ptr<Backend> Backend::getEntry(Lbdsr &parent) {
  // This method retrieves the pointer to Backend object specified by its keys.
  if (parent.backend_ != nullptr) {
    return parent.backend_;
  } else {
    return std::make_shared<Backend>(parent);
  }
}

void Backend::removeEntry(Lbdsr &parent) {
  // This method removes the single Backend object specified by its keys.
  // Remember to call here the remove static method for all-sub-objects of
  // Backend.
  if (parent.backend_ != nullptr) {
    parent.backend_->delPoolList();
    parent.backend_ = nullptr;
  } else {
    throw std::runtime_error("There is no backend in this LBDSR");
  }
}

std::shared_ptr<spdlog::logger> Backend::logger() {
  return parent_.logger();
}
