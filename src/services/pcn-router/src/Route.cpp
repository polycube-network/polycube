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

// TODO: Modify these methods with your own implementation

#include "Route.h"
#include "Router.h"

Route::Route(Router &parent, const RouteJsonObject &conf)
    : RouteBase(parent) {
  logger()->info("Creating Route instance");

  network_ = conf.getNetwork();
  nexthop_ = conf.getNexthop();

  pathCostIsSet_ = conf.pathcostIsSet();

  pathcost_ = conf.getPathcost();

  parent.logger()->debug(
      "Adding route [network: {0} - nexthop: {1} - path cost: {2}]",
      network_, nexthop_, pathcost_);

  // identify the interface of the router to be used to reach the nexthop
  if (!conf.interfaceIsSet()) {
    interface_ = parent.search_interface_from_nexthop(nexthop_);
  } else {
    interface_ = conf.getInterface();
  }

  parent.logger()->info(
      "Adding route [network: {0} - nexthop: {1} - interface: {2} "
      "- path cost: {3}]", network_, nexthop_, interface_, pathcost_);

  int port_id = parent.get_port(interface_)->index();

  parent.add_active_nexthop_in_ebpf_map(network_, nexthop_,
                                        conf.getPathcost(), port_id);
}

Route::Route(Router &parent, std::string network, const std::string &nexthop,
             const std::string &interface, const uint32_t pathcost)
    : RouteBase(parent) {
  network_ = network;
  nexthop_ = nexthop;
  interface_ = interface;
  pathcost_ = pathcost;
  pathCostIsSet_ = true;
}

Route::~Route() {}

std::string Route::getNetwork() {
  // This method retrieves the network value.
  return network_;
}

std::string Route::getNexthop() {
  // This method retrieves the nexthop value.
  return nexthop_;
}

std::string Route::getInterface() {
  // This method retrieves the interface value.
  return interface_;
}

uint32_t Route::getPathcost() {
  // This method retrieves the pathcost value.
  return pathcost_;
}

void Route::setPathcost(const uint32_t &value) {
  throw std::runtime_error("Route::setPathcost: Method not implemented");
}

bool Route::pathcostIsSet() {
  return pathCostIsSet_;
}
