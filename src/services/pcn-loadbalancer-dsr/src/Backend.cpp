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

std::shared_ptr<spdlog::logger> Backend::logger() {
  return parent_.logger();
}

std::shared_ptr<BackendPool> Backend::getPool(const uint32_t &id) {
  std::string pool_mac = pools_[id];
  BackendPoolJsonObject conf;
  conf.setId(id);
  conf.setMac(pool_mac);
  return std::make_shared<BackendPool>(*this, conf);
}

std::vector<std::shared_ptr<BackendPool>> Backend::getPoolList() {
  std::vector<std::shared_ptr<BackendPool>> pools_vect;
  for (auto &it : pools_)
    pools_vect.push_back(getPool(it.first));

  return pools_vect;
}

void Backend::addPool(const uint32_t &id, const BackendPoolJsonObject &conf) {
  try {
    auto config_table = parent_.get_array_table<uint64_t>("config_table");

    pools_[id] = conf.getMac();

    logger()->info("n backend servers {0}", pools_.size());
    logger()->info("mac: {0}", conf.getMac());

    config_table.set(0, pools_.size());
    config_table.set(id, utils::mac_string_to_be_uint(conf.getMac()));
  } catch (std::exception &e) {
    logger()->error("[BackendPool] Error while creating the backend pool {0}",
                    id);
    throw;
  }
}

void Backend::addPoolList(const std::vector<BackendPoolJsonObject> &conf) {
  for (auto &i : conf) {
    uint32_t id_ = i.getId();
    addPool(id_, i);
  }
}

void Backend::replacePool(const uint32_t &id,
                          const BackendPoolJsonObject &conf) {
  delPool(id);
  uint32_t id_ = conf.getId();
  addPool(id_, conf);
}

void Backend::delPool(const uint32_t &id) {
  // TODO: should the ebpf map be cleanup?
  if (pools_.count(id) != 0) {
    pools_.erase(id);
  }
}

void Backend::delPoolList() {
  // TODO: would it be possible to just call claer un pools_ ?
  for (auto it = pools_.begin(); it != pools_.end();) {
    uint32_t pool_id = it->first;
    delPool(pool_id);
  }
}