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


#pragma once


#include "../interface/RouteInterface.h"


#include <spdlog/spdlog.h>

#include <linux/version.h>

#include "polycube/services/cube.h"
#include "polycube/services/port.h"
#include "polycube/services/utils.h"

#include "Ports.h"
#include "ArpEntry.h"

#include "UtilityMethods.h"

#include "CircularBuffer.h"

#include <tins/ethernetII.h>

#include <tins/tins.h>

/* this define allow the code that required
   the kernel version 4.15 to work */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,15,0)
  #define NEW_KERNEL_VERS
#endif

//TODO: replace with an ENUM
#define TYPE_NOLOCALINTERFACE 0 // used to compare the 'type' field in the rt_v
#define TYPE_LOCALINTERFACE 1

using polycube::service::CubeType;
using namespace polycube::service;

using namespace Tins;


class Router;

using namespace io::swagger::server::model;

class Route : public RouteInterface {
public:
  Route(Router &parent, const RouteJsonObject &conf);
  Route(Router &parent, const std::string network, const std::string &netmask, const std::string &nexthop, const std::string &interface, const uint32_t pathcost);
  virtual ~Route();

  static void create(Router &parent, const std::string &network, const std::string &netmask, const std::string &netxhop, const RouteJsonObject &conf);
  static std::shared_ptr<Route> getEntry(Router &parent, const std::string &network, const std::string &netmask, const std::string &nexthop);
  static void removeEntry(Router &parent, const std::string &network, const std::string &netmask, const std::string &nexthop);
  static std::vector<std::shared_ptr<Route>> get(Router &parent);
  static void remove(Router &parent);
  nlohmann::fifo_map<std::string, std::string> getKeys();
  std::shared_ptr<spdlog::logger> logger();
  void update(const RouteJsonObject &conf) override;
  RouteJsonObject toJsonObject() override;

  /// <summary>
  /// Outgoing interface
  /// </summary>
  std::string getInterface() override;

  /// <summary>
  /// Destination network netmask
  /// </summary>
  std::string getNetmask() override;

  /// <summary>
  /// Next hop; if destination is local will be shown &#39;local&#39; instead of the ip address
  /// </summary>
  std::string getNexthop() override;

  /// <summary>
  /// Destination network IP
  /// </summary>
  std::string getNetwork() override;

  /// <summary>
  /// Cost of this route
  /// </summary>
  uint32_t getPathcost() override;
  void setPathcost(const uint32_t &value) override;

  // The following methods have been added manually

  bool pathcostIsSet();

private:
  Router &parent_;

  // The following attributes have been added manually
  std::string network_;
  std::string netmask_;
  std::string nexthop_;
  std::string interface_;
  uint32_t pathcost_;

  bool pathCostIsSet_;
};

