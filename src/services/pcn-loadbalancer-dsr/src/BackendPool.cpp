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

#include "BackendPool.h"
#include "Lbdsr.h"

BackendPool::BackendPool(Backend &parent, const BackendPoolJsonObject &conf)
    : parent_(parent) {
  logger()->info("Creating BackendPool instance");

  id_ = conf.getId();
  mac_ = conf.getMac();
  parent.addPool(id_, conf);
}

BackendPool::~BackendPool() {}

void BackendPool::update(const BackendPoolJsonObject &conf) {
  // This method updates all the object/parameter in BackendPool object
  // specified in the conf JsonObject.
  // You can modify this implementation.

  id_ = conf.getId();
  mac_ = conf.getMac();
  parent_.replacePool(id_, conf);
}

BackendPoolJsonObject BackendPool::toJsonObject() {
  BackendPoolJsonObject conf;

  conf.setMac(getMac());

  conf.setId(getId());

  return conf;
}

std::string BackendPool::getMac() {
  // This method retrieves the mac value.
  return this->mac_;
}

uint32_t BackendPool::getId() {
  // This method retrieves the id value.
  return this->id_;
}

std::shared_ptr<spdlog::logger> BackendPool::logger() {
  return parent_.logger();
}
