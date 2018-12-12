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


//Modify these methods with your own implementation


#include "Stats.h"
#include "Ddosmitigator.h"

using namespace polycube::service;

Stats::Stats(Ddosmitigator &parent, const StatsJsonObject &conf): parent_(parent) {
  logger()->info("Creating Stats instance");
}

Stats::~Stats() { }

void Stats::update(const StatsJsonObject &conf) {
  //This method updates all the object/parameter in Stats object specified in the conf JsonObject.
  //You can modify this implementation.
}

StatsJsonObject Stats::toJsonObject(){
  StatsJsonObject conf;

  conf.setPps(getPps());

  conf.setPkts(getPkts());

  return conf;
}


void Stats::create(Ddosmitigator &parent, const StatsJsonObject &conf){

  // throw std::runtime_error("[Stats]: Method create not supported");
}

std::shared_ptr<Stats> Stats::getEntry(Ddosmitigator &parent){
  //This method retrieves the pointer to Stats object specified by its keys.
  parent.logger()->debug("Stats getEntry");

  StatsJsonObject sjo;
  return std::shared_ptr<Stats>(new Stats(parent, sjo));
}

void Stats::removeEntry(Ddosmitigator &parent){
  //This method removes the single Stats object specified by its keys.
  //Remember to call here the remove static method for all-sub-objects of Stats.
  throw std::runtime_error("[Stats]: Method removeEntry not supported. Read Only");
}

uint64_t Stats::getPps(){
  //This method retrieves the pps value.
  int begin = getPkts();
  sleep(1);
  int end = getPkts();

  return end-begin;
  return 0;
  // throw std::runtime_error("[Stats]: Method getPps not implemented");
}

uint64_t Stats::getPkts(){
  //This method retrieves the pkts value.
  uint64_t pkts = 0;

  auto dropcnt = parent_.get_percpuarray_table<uint64_t>("dropcnt");
  auto values = dropcnt.get(0);

  pkts = std::accumulate(values.begin(), values.end(), pkts);

  logger()->debug("getting dropped packets...");
  logger()->debug("got {0} pkts", pkts);

  return pkts;
}

std::shared_ptr<spdlog::logger> Stats::logger() {
  return parent_.logger();
}
