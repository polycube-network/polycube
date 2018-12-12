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

#include "BlacklistSrc.h"
#include "Ddosmitigator.h"
#include "polycube/common.h"

using namespace polycube::service;

BlacklistSrc::BlacklistSrc(Ddosmitigator &parent, const BlacklistSrcJsonObject &conf): parent_(parent) {
  logger()->debug("BlacklistSrc Constructor. ip {0} ", conf.getIp());
  this->ip_ = conf.getIp();
}

BlacklistSrc::~BlacklistSrc() {
  auto srcblacklist = parent_.get_percpuhash_table<uint32_t, uint64_t>("srcblacklist");
  logger()->debug("BlacklistSrc Destructor. ip {0} ", ip_);
  try{
    srcblacklist.remove(utils::ip_string_to_be_uint(ip_));
  } catch (...){ }
}

void BlacklistSrc::update(const BlacklistSrcJsonObject &conf) {
  //This method updates all the object/parameter in BlacklistSrc object specified in the conf JsonObject.
  //You can modify this implementation.
  logger()->error("BlacklistSrc update. This method should never be called ");
}

BlacklistSrcJsonObject BlacklistSrc::toJsonObject(){
  BlacklistSrcJsonObject conf;

  try{
    conf.setIp(getIp());
  } catch (...) {}
  logger()->debug("BlacklistSrc toJsonObject");

  conf.setDropPkts(getDropPkts());
  return conf;
}

void BlacklistSrc::create(Ddosmitigator &parent, const std::string &ip, const BlacklistSrcJsonObject &conf){
  //This method creates the actual BlacklistSrc object given thee key param.
  //Please remember to call here the create static method for all sub-objects of BlacklistSrc.
  parent.logger()->debug("BlacklistSrc create");

  try{
    parent.logger()->debug("blacklist size {0} ", parent.blacklistsrc_.size());
    // TODO check if src ip rules are already present
    // and reinject datapath with srcblacklist ps

    if (parent.blacklistsrc_.size() >= 0 ){
      parent.setSrcMatch(true);
      parent.reloadCode();
    }

    auto srcblacklist = parent.get_percpuhash_table<uint32_t, uint64_t>("srcblacklist");
    srcblacklist.set(utils::ip_string_to_be_uint(ip), 0);
  } catch (...) {
    throw std::runtime_error("unable to add element to map");
  }

  BlacklistSrcJsonObject configuration;
  configuration.setIp(ip);

  parent.blacklistsrc_.emplace(std::piecewise_construct, std::forward_as_tuple(ip),
    std::forward_as_tuple(parent, configuration));
}

std::shared_ptr<BlacklistSrc> BlacklistSrc::getEntry(Ddosmitigator &parent, const std::string &ip){
  //This method retrieves the pointer to BlacklistSrc object specified by its keys.
  parent.logger()->debug("BlacklistSrc getEntry");

  return std::shared_ptr<BlacklistSrc>(&parent.blacklistsrc_.at(ip), [](BlacklistSrc *) {});
}

void BlacklistSrc::removeEntry(Ddosmitigator &parent, const std::string &ip){
  //This method removes the single BlacklistSrc object specified by its keys.
  //Remember to call here the remove static method for all-sub-objects of BlacklistSrc.
  parent.logger()->debug("BlacklistSrc removeEntry");

  // ebpf map remove is performed in destructor
  parent.blacklistsrc_.erase(ip);

  if (parent.blacklistsrc_.size() == 0 ){
    parent.setSrcMatch(false);
    parent.reloadCode();
  }

  return;
}

std::vector<std::shared_ptr<BlacklistSrc>> BlacklistSrc::get(Ddosmitigator &parent){
  //This methods get the pointers to all the BlacklistSrc objects in Ddosmitigator.
  parent.logger()->debug("BlacklistSrc get vector");

  std::vector<std::shared_ptr<BlacklistSrc>> blacklistsrc;

  for (auto &it : parent.blacklistsrc_) {
    blacklistsrc.push_back(BlacklistSrc::getEntry(parent, it.first));
  }

  return blacklistsrc;
}

void BlacklistSrc::remove(Ddosmitigator &parent){
  //This method removes all BlacklistSrc objects in Ddosmitigator.
  //Remember to call here the remove static method for all-sub-objects of BlacklistSrc.
  parent.logger()->debug("BlacklistSrc remove");

  // ebpf maps remove performed in destructor
  parent.blacklistsrc_.clear();

  if (parent.blacklistsrc_.size() == 0 ){
    parent.setSrcMatch(false);
    parent.reloadCode();
  }

  return;
}

std::string BlacklistSrc::getIp(){
  logger()->debug("BlacklistSrc getIp {0} ", this->ip_);

  return this->ip_;
}

uint64_t BlacklistSrc::getDropPkts(){
  //This method retrieves the dropPkts value.
  uint64_t pkts = 0;

  try{
    auto srcblacklist = parent_.get_percpuhash_table<uint32_t, uint64_t>("srcblacklist");
    auto values = srcblacklist.get(utils::ip_string_to_be_uint(ip_));

    pkts = std::accumulate(values.begin(), values.end(), pkts);

    logger()->debug("getting dropped packets...");
    logger()->debug("got {0} pkts", pkts);

    return pkts;
  // TODO: what is this try-catch block for then?
  } catch (...) {
    throw;
  }
  return  pkts;
}

std::shared_ptr<spdlog::logger> BlacklistSrc::logger() {
  return parent_.logger();
}
