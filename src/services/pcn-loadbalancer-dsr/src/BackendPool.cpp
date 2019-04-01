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

void BackendPool::create(Backend &parent, const uint32_t &id,
                         const BackendPoolJsonObject &conf) {
  // This method creates the actual BackendPool object given thee key param.
  // Please remember to call here the create static method for all sub-objects
  // of BackendPool.
  try {
    auto config_table =
        parent.parent_.get_array_table<uint64_t>("config_table");

    parent.pools_[id] = conf.getMac();

    parent.logger()->info("n backend servers {0}", parent.pools_.size());
    parent.logger()->info("mac: {0}", conf.getMac());

    config_table.set(0, parent.pools_.size());
    config_table.set(id, utils::mac_string_to_be_uint(conf.getMac()));
  } catch (std::exception &e) {
    parent.logger()->error(
        "[BackendPool] Error while creating the backend pool {0}", id);
    throw;
  }
}

std::shared_ptr<BackendPool> BackendPool::getEntry(Backend &parent,
                                                   const uint32_t &id) {
  // This method retrieves the pointer to BackendPool object specified by its
  // keys.
  std::string pool_mac = parent.pools_[id];
  BackendPoolJsonObject conf;
  conf.setId(id);
  conf.setMac(pool_mac);
  return std::make_shared<BackendPool>(parent, conf);
}

void BackendPool::removeEntry(Backend &parent, const uint32_t &id) {
  // This method removes the single BackendPool object specified by its keys.
  // Remember to call here the remove static method for all-sub-objects of
  // BackendPool.
  if (parent.pools_.count(id) != 0) {
    parent.pools_.erase(id);
    parent.delPool(id);
  }
}

std::vector<std::shared_ptr<BackendPool>> BackendPool::get(Backend &parent) {
  // This methods get the pointers to all the BackendPool objects in Backend.
  std::vector<std::shared_ptr<BackendPool>> pools_vect;
  for (auto &it : parent.pools_)
    pools_vect.push_back(BackendPool::getEntry(parent, it.first));

  return pools_vect;
}

void BackendPool::remove(Backend &parent) {
  // This method removes all BackendPool objects in Backend.
  // Remember to call here the remove static method for all-sub-objects of
  // BackendPool.
  for (auto it = parent.pools_.begin(); it != parent.pools_.end();) {
    uint32_t pool_id = it->first;
    BackendPool::removeEntry(parent, pool_id);
  }
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
