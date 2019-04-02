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

#include "BlacklistDst.h"
#include "Ddosmitigator.h"
#include "polycube/common.h"

using namespace polycube::service;

BlacklistDst::BlacklistDst(Ddosmitigator &parent,
                           const BlacklistDstJsonObject &conf)
    : parent_(parent) {
  logger()->debug("BlacklistDst Constructor. ip {0} ", conf.getIp());
  this->ip_ = conf.getIp();
}

BlacklistDst::~BlacklistDst() {
  auto dstblacklist =
      parent_.get_percpuhash_table<uint32_t, uint64_t>("dstblacklist");
  logger()->debug("BlacklistDst Destructor. ip {0} ", ip_);
  try {
    dstblacklist.remove(utils::ip_string_to_be_uint(ip_));
  } catch (...) {
  }
}

void BlacklistDst::update(const BlacklistDstJsonObject &conf) {
  // This method updates all the object/parameter in BlacklistDst object
  // specified in the conf JsonObject.
  // You can modify this implementation.
  logger()->error("BlacklistDst update. This method should never be called ");
}

BlacklistDstJsonObject BlacklistDst::toJsonObject() {
  BlacklistDstJsonObject conf;

  try {
    conf.setIp(getIp());
  } catch (...) {
  }
  logger()->debug("BlacklistDst toJsonObject");

  conf.setDropPkts(getDropPkts());

  return conf;
}

std::string BlacklistDst::getIp() {
  // This method retrieves the ip value.
  logger()->debug("BlacklistDst getIp {0} ", this->ip_);

  return this->ip_;
}

uint64_t BlacklistDst::getDropPkts() {
  // This method retrieves the dropPkts value.
  uint64_t pkts = 0;
  try {
    auto dstblacklist =
        parent_.get_percpuhash_table<uint32_t, uint64_t>("dstblacklist");
    auto values = dstblacklist.get(utils::ip_string_to_be_uint(ip_));

    pkts = std::accumulate(values.begin(), values.end(), pkts);

    logger()->debug("getting dropped packets...");
    logger()->debug("got {0} pkts", pkts);
    // TODO: what is this try-catch block for then?
  } catch (...) {
    throw;
  }
  return pkts;
}

std::shared_ptr<spdlog::logger> BlacklistDst::logger() {
  return parent_.logger();
}
