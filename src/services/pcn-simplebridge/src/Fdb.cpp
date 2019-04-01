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

#include "Fdb.h"
#include "Simplebridge.h"

Fdb::Fdb(Simplebridge &parent, const FdbJsonObject &conf) : parent_(parent) {
  logger()->info("Creating Fdb instance");

  if (conf.agingTimeIsSet()) {
    setAgingTime(conf.getAgingTime());
  }

  if (conf.entryIsSet()) {
    // FIXME: What if this method doesn't succeed
    Fdb::addEntryList(conf.getEntry());
  }
}

Fdb::Fdb(Simplebridge &parent) : parent_(parent), agingTime_(0) {}

Fdb::~Fdb() {}

void Fdb::update(const FdbJsonObject &conf) {
  // This method updates all the object/parameter in Fdb object specified in the
  // conf JsonObject.
  // You can modify this implementation.
  if (conf.entryIsSet()) {
    for (auto &i : conf.getEntry()) {
      auto address = i.getAddress();
      auto m = getEntry(address);
      m->update(i);
    }
  }

  if (conf.agingTimeIsSet()) {
    setAgingTime(conf.getAgingTime());
  }
}

FdbJsonObject Fdb::toJsonObject() {
  FdbJsonObject conf;

  for (auto &i : getEntryList()) {
    conf.addFdbEntry(i->toJsonObject());
  }

  conf.setAgingTime(getAgingTime());

  return conf;
}

void Fdb::create(Simplebridge &parent, const FdbJsonObject &conf) {
  // This method creates the actual Fdb object given thee key param.
  // Please remember to call here the create static method for all sub-objects
  // of Fdb.
  parent.fdb_ = std::make_shared<Fdb>(parent, conf);
}

std::shared_ptr<Fdb> Fdb::getEntry(Simplebridge &parent) {
  // This method retrieves the pointer to Fdb object specified by its keys.
  if (parent.fdb_ != nullptr) {
    return parent.fdb_;
  } else {
    return std::make_shared<Fdb>(parent);
  }
}

void Fdb::removeEntry(Simplebridge &parent) {
  // This method removes the single Fdb object specified by its keys.
  // Remember to call here the remove static method for all-sub-objects of Fdb.
  if (parent.fdb_ != nullptr) {
    parent.fdb_->delEntryList();

    // I don't want to delete the Filtering database. This is very strange
    // I'll only reset the agingTime, if needed
    parent.fdb_->setAgingTime(300);
    // parent.fdb_.reset();
    // parent.fdb_ = nullptr;
  } else {
    // This should never happen
    throw std::runtime_error("There is not filtering database in the bridge");
  }
}

uint32_t Fdb::getAgingTime() {
  // This method retrieves the agingTime value.
  return agingTime_;
}

void Fdb::setAgingTime(const uint32_t &value) {
  // This method set the agingTime value.
  if (agingTime_ != value) {
    try {
      parent_.reloadCodeWithAgingtime(value);
    } catch (std::exception &e) {
      logger()->error("Failed to reload the code with the new aging time {0}",
                      value);
    }
    agingTime_ = value;
  } else {
    logger()->debug(
        "[Fdb] Request to set aging time but the value is the same as before");
  }
}

FdbFlushOutputJsonObject Fdb::flush() {
  FdbFlushOutputJsonObject result;
  try {
    Fdb::removeEntry(parent_);
  } catch (std::exception &e) {
    logger()->error("Error while flushing the filtering database. Reason: {0}",
                    e.what());
    result.setFlushed(false);
    return result;
  }
  result.setFlushed(true);
  return result;
}

std::shared_ptr<spdlog::logger> Fdb::logger() {
  return parent_.logger();
}
