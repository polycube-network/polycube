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

#include "Route.h"
#include "Router.h"

Route::Route(Router &parent, const RouteJsonObject &conf) : parent_(parent) {
  logger()->info("Creating Route instance");

  // TODO: check the validity of the netmask

  netmask_ = conf.getNetmask();
  network_ = conf.getNetwork();
  nexthop_ = conf.getNexthop();

  pathCostIsSet_ = conf.pathcostIsSet();

  pathcost_ = conf.getPathcost();

  parent.logger()->debug(
      "Adding route [network: {0} - netmask: {1} - nexthop: {2} - path cost: "
      "{3}]",
      network_, netmask_, nexthop_, pathcost_);

  // identify the interface of the router to be used to reach the nexthop
  if (!conf.interfaceIsSet()) {
    interface_ = parent.search_interface_from_nexthop(nexthop_);
  } else {
    interface_ = conf.getInterface();
  }

  parent.logger()->info(
      "Adding route [network: {0} - netmask: {1} - nexthop: {2} - interface: "
      "{3} - path cost: {4}]",
      network_, netmask_, nexthop_, interface_, pathcost_);

  int port_id = parent.get_port(interface_)->index();

  parent.add_active_nexthop_in_ebpf_map(network_, netmask_, nexthop_,
                                        conf.getPathcost(), port_id);
}

Route::Route(Router &parent, std::string network, const std::string &netmask,
             const std::string &nexthop, const std::string &interface,
             const uint32_t pathcost)
    : parent_(parent) {
  netmask_ = netmask;
  network_ = network;
  nexthop_ = nexthop;
  interface_ = interface;
  pathcost_ = pathcost;
  pathCostIsSet_ = true;
}

Route::~Route() {}

void Route::update(const RouteJsonObject &conf) {
  // This method updates all the object/parameter in Route object specified in
  // the conf JsonObject.
  // You can modify this implementation.

  if (conf.pathcostIsSet()) {
    setPathcost(conf.getPathcost());
  }
}

RouteJsonObject Route::toJsonObject() {
  RouteJsonObject conf;

  conf.setInterface(getInterface());

  conf.setNetmask(getNetmask());

  conf.setNexthop(getNexthop());

  conf.setNetwork(getNetwork());

  conf.setPathcost(getPathcost());

  return conf;
}

void Route::create(Router &parent, const std::string &network,
                   const std::string &netmask, const std::string &nexthop,
                   const RouteJsonObject &conf) {
  // This method creates the actual Route object given thee key param.
  // Please remember to call here the create static method for all sub-objects
  // of Route.

  parent.logger()->debug(
      "Trying to add route [network: {0} - netmask: {1} - nexthop: {2}]",
      network, netmask, nexthop);

  std::tuple<string, string, string> key(network, netmask, nexthop);

  if (parent.routes_.count(key) != 0)
    throw std::runtime_error("Route already exists");

  parent.routes_.emplace(std::piecewise_construct, std::forward_as_tuple(key),
                         std::forward_as_tuple(parent, conf));
}

std::shared_ptr<Route> Route::getEntry(Router &parent,
                                       const std::string &network,
                                       const std::string &netmask,
                                       const std::string &nexthop) {
  // This method retrieves the pointer to Route object specified by its keys.

  std::tuple<string, string, string> key(network, netmask, nexthop);

  return std::shared_ptr<Route>(&parent.routes_.at(key), [](Route *) {});
}

void Route::removeEntry(Router &parent, const std::string &network,
                        const std::string &netmask,
                        const std::string &nexthop) {
  // This method removes the single Route object specified by its keys.
  // Remember to call here the remove static method for all-sub-objects of
  // Route.

  parent.logger()->debug(
      "Trying to remove route [network: {0} - netmask: {1} - nexthop: {2}]",
      network, netmask, nexthop);

  // FIXME: is this good? cannot users delete "local" routes.
  // It is possible in linux
  if (nexthop == "local")  // FIXME: use constants for these two values
    throw std::runtime_error("Users can not delete a local route");

  parent.remove_route(network, netmask, nexthop);
}

std::vector<std::shared_ptr<Route>> Route::get(Router &parent) {
  // This methods get the pointers to all the Route objects in Router.
  parent.logger()->debug("Returning all the routes ({0})",
                         parent.routes_.size());

  std::vector<std::shared_ptr<Route>> routes_vect;
  for (auto &it : parent.routes_) {
    routes_vect.push_back(Route::getEntry(parent, it.second.getNetwork(),
                                          it.second.getNetmask(),
                                          it.second.getNexthop()));
    parent.logger()->debug(
        "\t'route [network: {0} - netmask: {1} - nexthop: {2}]",
        it.second.getNetwork(), it.second.getNetmask(), it.second.getNexthop());
  }

  return routes_vect;
}

void Route::remove(Router &parent) {
  // This method removes all Route objects in Router.
  // Remember to call here the remove static method for all-sub-objects of
  // Route.

  parent.logger()->debug("Removing all the entries in the routing table");
  parent.remove_all_routes();
}

std::string Route::getInterface() {
  // This method retrieves the interface value.
  return interface_;
}

std::string Route::getNetmask() {
  // This method retrieves the netmask value.
  return netmask_;
}

std::string Route::getNexthop() {
  // This method retrieves the nexthop value.
  return nexthop_;
}

std::string Route::getNetwork() {
  // This method retrieves the network value.
  return network_;
}

uint32_t Route::getPathcost() {
  // This method retrieves the pathcost value.
  return pathcost_;
}

void Route::setPathcost(const uint32_t &value) {
  // This method set the pathcost value.
  throw std::runtime_error("[Route]: Method setPathcost not implemented");
}

std::shared_ptr<spdlog::logger> Route::logger() {
  return parent_.logger();
}

bool Route::pathcostIsSet() {
  return pathCostIsSet_;
}
