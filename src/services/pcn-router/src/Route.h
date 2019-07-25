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

#include "../base/RouteBase.h"

#include <linux/version.h>

#include "ArpTable.h"
#include "Ports.h"

#include "CircularBuffer.h"

#include <tins/ethernetII.h>

#include <tins/tins.h>

/* this define allow the code that required
   the kernel version 4.15 to work */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0)
#define NEW_KERNEL_VERS
#endif

// TODO: replace with an ENUM
#define TYPE_NOLOCALINTERFACE 0  // used to compare the 'type' field in the rt_v
#define TYPE_LOCALINTERFACE 1

class Router;

using namespace polycube::service::model;

class Route : public RouteBase {
 public:
  Route(Router &parent, const RouteJsonObject &conf);
  Route(Router &parent, const std::string network, const std::string &nexthop,
        const std::string &interface, const uint32_t pathcost);
  virtual ~Route();

  /// <summary>
  /// Destination network IP
  /// </summary>
  std::string getNetwork() override;

  /// <summary>
  /// Next hop; if destination is local will be shown &#39;local&#39; instead of the ip address
  /// </summary>
  std::string getNexthop() override;

  /// <summary>
  /// Outgoing interface
  /// </summary>
  std::string getInterface() override;

  /// <summary>
  /// Cost of this route
  /// </summary>
  uint32_t getPathcost() override;
  void setPathcost(const uint32_t &value) override;

  // The following methods have been added manually
  bool pathcostIsSet();

 private:
  // The following attributes have been added manually
  std::string network_;
  std::string nexthop_;
  std::string interface_;
  uint32_t pathcost_;

  bool pathCostIsSet_;
};
