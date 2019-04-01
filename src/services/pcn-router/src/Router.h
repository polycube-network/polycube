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

#include "../interface/RouterInterface.h"

#include "polycube/services/cube.h"
#include "polycube/services/port.h"
#include "polycube/services/utils.h"

#include <spdlog/spdlog.h>

#include "ArpEntry.h"
#include "Ports.h"
#include "Route.h"

#include <tins/ethernetII.h>
#include <tins/tins.h>

using namespace io::swagger::server::model;
using namespace Tins;
using namespace polycube::service;

/* definitions copied from dataplane */
/* Routing Table Key */
struct rt_k {
  uint32_t netmask_len;
  uint32_t network;
} __attribute__((packed));

/* Routing Table Value
the type field is used to know if the destination is one interface of the router
*/

struct rt_v {
  uint32_t port;
  uint32_t nexthop;
  uint8_t type;
} __attribute__((packed));

class Router : public polycube::service::Cube<Ports>, public RouterInterface {
  friend class Ports;
  friend class Route;

 public:
  Router(const std::string name, const RouterJsonObject &conf);
  virtual ~Router();
  std::string generate_code();
  std::vector<std::string> generate_code_vector();
  void packet_in(Ports &port, polycube::service::PacketInMetadata &md,
                 const std::vector<uint8_t> &packet) override;

  void update(const RouterJsonObject &conf) override;
  RouterJsonObject toJsonObject() override;

  /// <summary>
  /// Entry associated with the ARP table
  /// </summary>
  std::shared_ptr<ArpEntry> getArpEntry(const std::string &address) override;
  std::vector<std::shared_ptr<ArpEntry>> getArpEntryList() override;
  void addArpEntry(const std::string &address,
                   const ArpEntryJsonObject &conf) override;
  void addArpEntryList(const std::vector<ArpEntryJsonObject> &conf) override;
  void replaceArpEntry(const std::string &address,
                       const ArpEntryJsonObject &conf) override;
  void delArpEntry(const std::string &address) override;
  void delArpEntryList() override;

  /// <summary>
  /// Entry associated with the routing table
  /// </summary>
  std::shared_ptr<Route> getRoute(const std::string &network,
                                  const std::string &netmask,
                                  const std::string &nexthop) override;
  std::vector<std::shared_ptr<Route>> getRouteList() override;
  void addRoute(const std::string &network, const std::string &netmask,
                const std::string &nexthop,
                const RouteJsonObject &conf) override;
  void addRouteList(const std::vector<RouteJsonObject> &conf) override;
  void replaceRoute(const std::string &network, const std::string &netmask,
                    const std::string &nexthop,
                    const RouteJsonObject &conf) override;
  void delRoute(const std::string &network, const std::string &netmask,
                const std::string &nexthop) override;
  void delRouteList() override;

  /// <summary>
  /// Entry of the ports table
  /// </summary>
  std::shared_ptr<Ports> getPorts(const std::string &name) override;
  std::vector<std::shared_ptr<Ports>> getPortsList() override;
  void addPorts(const std::string &name, const PortsJsonObject &conf) override;
  void addPortsList(const std::vector<PortsJsonObject> &conf) override;
  void replacePorts(const std::string &name,
                    const PortsJsonObject &conf) override;
  void delPorts(const std::string &name) override;
  void delPortsList() override;

  // The following methods have been added by hand

  // FIXME: the following methods should be protected. Ports should be friend

  void add_local_route(const std::string &interface_ip,
                       const std::string &interface_netmask,
                       const std::string &port_name, const int port_index);

  void remove_local_route(const std::string &interface_ip,
                          const std::string &interface_netmask,
                          const std::string &port_name);

  void delete_route(const std::string &interface_ip,
                    const std::string &interface_netmask);

  std::string search_interface_from_nexthop(const std::string &nexthop);

  void add_active_nexthop_in_ebpf_map(const std::string &network,
                                      const std::string &netmask,
                                      const std::string &nexthop,
                                      int32_t actual_pathcost,
                                      const int &port_id);

  void remove_route(const std::string &network, const std::string &netmask,
                    const std::string &nexthop);

  void remove_all_routes();

 private:
  // The following variables have been added by hand
  std::map<std::tuple<std::string, std::string, std::string>, Route> routes_;

  // Mutex used to regulate access to the buffer of stored packets
  std::mutex mu;

  // Circular buffer
  std::map<unsigned int, CircularBuffer> arp_request_map;

  // The following methods have been added by hand

  // Methods to manage packets coming from the fast path
  void handle_router_pkt(Port &port, PacketInMetadata &md,
                         const std::vector<uint8_t> &packet);
  EthernetII make_echo_reply(EthernetII &origin, const IPv4Address &src_ip,
                             const IPv4Address &dst_ip, ICMP &old_icmp);
  void generate_icmp_ttlexceed(Port &port, PacketInMetadata &md,
                               const std::vector<uint8_t> &packet);
  void generate_arp_request(Port &port, PacketInMetadata &md,
                            const std::vector<uint8_t> &packet);
  void generate_arp_reply(Port &port, PacketInMetadata &md,
                          const std::vector<uint8_t> &packet);

  // Methods to manage the routing table
  void find_new_active_nexthop(const std::string &network,
                               const std::string &netmask,
                               const std::string &nexthop);
};
