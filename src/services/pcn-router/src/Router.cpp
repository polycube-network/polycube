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

#include "Router.h"
#include "Router_dp.h"

enum {
  SLOWPATH_ARP_REPLY = 1,
  SLOWPATH_ARP_LOOKUP_MISS,
  SLOWPATH_TTL_EXCEEDED,
  SLOWPATH_PKT_FOR_ROUTER
};

Router::Router(const std::string name, const RouterJsonObject &conf)
  : Cube(conf.getBase(), { router_code }, {}),
    RouterBase(name),
    netlink_instance_router_(polycube::polycubed::Netlink::getInstance()) {
  logger()->info("Creating Router instance");

  addPortsList(conf.getPorts());
  addRouteList(conf.getRoute());
  addArpTableList(conf.getArpTable());

  if (get_shadow()) {
    // netlink notification
    netlink_notification_index_route_added = netlink_instance_router_.registerObserver(
      polycube::polycubed::Netlink::Event::ROUTE_ADDED,
      std::bind(&Router::netlink_notification_route_added, this,
                std::placeholders::_1, std::placeholders::_2));

    netlink_notification_index_route_deleted = netlink_instance_router_.registerObserver(
      polycube::polycubed::Netlink::Event::ROUTE_DELETED,
      std::bind(&Router::netlink_notification_route_deleted, this,
                std::placeholders::_1, std::placeholders::_2));

    netlink_notification_index_link_deleted = netlink_instance_router_.registerObserver(
      polycube::polycubed::Netlink::Event::LINK_DELETED,
      std::bind(&Router::netlink_notification_link_deleted, this,
                std::placeholders::_1, std::placeholders::_2));

    netlink_notification_index_new_address = netlink_instance_router_.registerObserver(
      polycube::polycubed::Netlink::Event::NEW_ADDRESS,
      std::bind(&Router::netlink_notification_new_address, this,
                std::placeholders::_1, std::placeholders::_2));
  }
}

Router::~Router() {
  logger()->info("Destroying Router instance");
  if (get_shadow()) {
    netlink_instance_router_.unregisterObserver(
          polycube::polycubed::Netlink::Event::ROUTE_ADDED,
          netlink_notification_index_route_added);
    netlink_instance_router_.unregisterObserver(
          polycube::polycubed::Netlink::Event::ROUTE_DELETED,
          netlink_notification_index_route_deleted);
    netlink_instance_router_.unregisterObserver(
          polycube::polycubed::Netlink::Event::LINK_DELETED,
          netlink_notification_index_link_deleted);
    netlink_instance_router_.unregisterObserver(
          polycube::polycubed::Netlink::Event::NEW_ADDRESS,
          netlink_notification_index_new_address);
  }
}

void Router::packet_in(Ports &port,
    polycube::service::PacketInMetadata &md,
    const std::vector<uint8_t> &packet) {
  logger()->debug("Packet received from port {0}", port.name());

  switch (md.reason) {
  case SLOWPATH_TTL_EXCEEDED:
    generate_icmp_ttlexceed(port, md, packet);
    break;

  case SLOWPATH_ARP_LOOKUP_MISS:
    generate_arp_request(port, md, packet);
    break;

  case SLOWPATH_ARP_REPLY:
    generate_arp_reply(port, md, packet);
    break;

  case SLOWPATH_PKT_FOR_ROUTER:
    handle_router_pkt(port, md, packet);
    break;

  default:
    logger()->error("Not valid reason {0} received", md.reason);
  }
}

// Basic default implementation, place your extension here (if needed)
std::shared_ptr<Ports> Router::getPorts(const std::string &name) {
  // call default implementation in base class
  return RouterBase::getPorts(name);
}

// Basic default implementation, place your extension here (if needed)
std::vector<std::shared_ptr<Ports>> Router::getPortsList() {
  // call default implementation in base class
  return RouterBase::getPortsList();
}

// Basic default implementation, place your extension here (if needed)
void Router::addPorts(const std::string &name, const PortsJsonObject &conf) {
  RouterBase::addPorts(name, conf);
  if (get_shadow()) {
    // this aligns the mac of the port with that of the veth in the namespace
    getPorts(name)->setMac(getPorts(name)->getMac());
  }
}

// Basic default implementation, place your extension here (if needed)
void Router::addPortsList(const std::vector<PortsJsonObject> &conf) {
  // call default implementation in base class
  RouterBase::addPortsList(conf);
}

// Basic default implementation, place your extension here (if needed)
void Router::replacePorts(const std::string &name,
                          const PortsJsonObject &conf) {
  // call default implementation in base class
  RouterBase::replacePorts(name, conf);
}

// Basic default implementation, place your extension here (if needed)
void Router::delPorts(const std::string &name) {
  logger()->info("Remove port {0}", name);

  auto port = get_port(name);

  // remove the secondary addresses of the port (and the related routes in the
  // routing table)
  port->delSecondaryipList();

  //if getIP is empty it means that you have a port with no IP address
  //so you do not need to call remove_local_route
  if(port->getIp().empty() == false)
    remove_local_route(port->getIp(), name);

  auto router_port = get_hash_table<uint16_t, r_port>("router_port");

  // remove the port from the datapath
  uint16_t index = port->index();
  router_port.remove(index);
  logger()->debug("Removed from 'router_port' - key: {0}", int_to_hex(index));

  // call default implementation in base class
  RouterBase::delPorts(name);

  logger()->info("Port {0} was removed", name);
}

// Basic default implementation, place your extension here (if needed)
void Router::delPortsList() {
  // call default implementation in base class
  RouterBase::delPortsList();
}

std::shared_ptr<Route> Router::getRoute(const std::string &network,
                                        const std::string &nexthop) {
  std::tuple<string, string> key(network, nexthop);

  return std::shared_ptr<Route>(&routes_.at(key), [](Route *) {});
}

std::vector<std::shared_ptr<Route>> Router::getRouteList() {
  std::vector<std::shared_ptr<Route>> routes_vect;
  for (auto &it : routes_) {
    routes_vect.push_back(getRoute(it.second.getNetwork(),
                                   it.second.getNexthop()));
    logger()->debug("\t'route [network: {0} - nexthop: {1}]",
                    it.second.getNetwork(), it.second.getNexthop());
  }

  return routes_vect;
}

void Router::addRoute(const std::string &network,
                      const std::string &nexthop,
                      const RouteJsonObject &conf) {
  logger()->debug(
      "Trying to add route [network: {0} - nexthop: {1}]", network, nexthop);

  std::tuple<string, string> key(network, nexthop);

  if (routes_.count(key) != 0)
    throw std::runtime_error("Route already exists");

  routes_.emplace(std::piecewise_construct, std::forward_as_tuple(key),
                  std::forward_as_tuple(*this, conf));
}

// Basic default implementation, place your extension here (if needed)
void Router::addRouteList(const std::vector<RouteJsonObject> &conf) {
  // call default implementation in base class
  RouterBase::addRouteList(conf);
}

// Basic default implementation, place your extension here (if needed)
void Router::replaceRoute(const std::string &network,
                          const std::string &nexthop,
                          const RouteJsonObject &conf) {
  // call default implementation in base class
  RouterBase::replaceRoute(network, nexthop, conf);
}

void Router::delRoute(const std::string &network, const std::string &nexthop) {
  logger()->debug(
      "Trying to remove route [network: {0} - nexthop: {1}]", network, nexthop);

  // FIXME: is this good? cannot users delete "local" routes.
  // It is possible in linux
  if (nexthop == "local")  // FIXME: use constants for these two values
    throw std::runtime_error("Users can not delete a local route");

  remove_route(network, nexthop);
}

// Basic default implementation, place your extension here (if needed)
void Router::delRouteList() {
  // call default implementation in base class
  RouterBase::delRouteList();
}

std::shared_ptr<ArpTable> Router::getArpTable(const std::string &address) {
  uint32_t ip_key = ip_string_to_nbo_uint(address);

  try {
    auto arp_table = get_hash_table<uint32_t, arp_entry>("arp_table");

    arp_entry entry = arp_table.get(ip_key);
    std::string mac = nbo_uint_to_mac_string(entry.mac);
    auto port = get_port(entry.port);

    return std::make_shared<ArpTable>(
        ArpTable(*this, mac, address, port->name()));
  } catch (std::exception &e) {
    logger()->error("Unable to find ARP table entry for address {0}. {1}",
                    address, e.what());
    throw std::runtime_error("ARP table entry not found");
  }
}

std::vector<std::shared_ptr<ArpTable>> Router::getArpTableList() {
  std::vector<std::shared_ptr<ArpTable>> arp_table_entries;

  // The ARP table is read from the data path
  try {
    auto arp_table = get_hash_table<uint32_t, arp_entry>("arp_table");
    auto arp_entries = arp_table.get_all();

    for (auto &entry : arp_entries) {
      auto key = entry.first;
      auto value = entry.second;

      std::string ip = nbo_uint_to_ip_string(key);
      std::string mac = nbo_uint_to_mac_string(value.mac);

      auto port = get_port(value.port);

      logger()->debug("Returning entry [ip: {0} - mac: {1} - interface: {2}]",
                      ip, mac, port->name());

      arp_table_entries.push_back(
          std::make_shared<ArpTable>(ArpTable(*this, mac, ip, port->name())));
    }
  } catch (std::exception &e) {
    logger()->error("Error while trying to get the ARP table");
    throw std::runtime_error("Unable to get the ARP table list");
  }

  return arp_table_entries;
}

void Router::addArpTable(const std::string &address,
                         const ArpTableJsonObject &conf) {
  logger()->debug("Creating ARP entry [ip: {0} - mac: {1} - interface: {2}",
                  address, conf.getMac(), conf.getInterface());

  uint64_t mac = mac_string_to_nbo_uint(conf.getMac());
  uint32_t index = get_port(conf.getInterface())->index();

  // FIXME: Check if entry already exists?
  auto arp_table = get_hash_table<uint32_t, arp_entry>("arp_table");
  arp_table.set(ip_string_to_nbo_uint(address),
               arp_entry{.mac = mac, .port = index});
}

// Basic default implementation, place your extension here (if needed)
void Router::addArpTableList(const std::vector<ArpTableJsonObject> &conf) {
  // call default implementation in base class
  RouterBase::addArpTableList(conf);
}

// Basic default implementation, place your extension here (if needed)
void Router::replaceArpTable(const std::string &address,
                             const ArpTableJsonObject &conf) {
  // call default implementation in base class
  RouterBase::replaceArpTable(address, conf);
}

void Router::delArpTable(const std::string &address) {
  std::shared_ptr<ArpTable> entry;

  try {
    entry = getArpTable(address);
  } catch (std::exception &e) {
    logger()->error("Unable to remove the ARP table entry for address {0}. {1}",
                    address, e.what());
    throw std::runtime_error("ARP table entry not found");
  }

  uint32_t key = ip_string_to_nbo_uint(address);

  auto arp_table = get_hash_table<uint32_t, arp_entry>("arp_table");

  try {
    arp_table.remove(key);
  } catch (...) {
    throw std::runtime_error("ARP table entry not found");
  }
}

// Basic default implementation, place your extension here (if needed)
void Router::delArpTableList() {
  // call default implementation in base class
  RouterBase::delArpTableList();
}

/*
* Methods to manage routing table in the datapath
*/

/*
* Given a nexthop, selects the proper interface of the router to reach it
*/
std::string Router::search_interface_from_nexthop(const std::string &nexthop) {
  for (auto item : routes_) {
    auto &network_ = std::get<0>(item.first);
    auto &nexthop_ = std::get<1>(item.first);
    // ignore non directly reachable routes
    if (nexthop_ != "local" && nexthop_ != "0.0.0.0") {
      continue;
    }

    std::string ip_route;
    std::string netmask_route;
    split_ip_and_prefix(network_, ip_route, netmask_route);
    if (address_in_subnet(ip_route, netmask_route, nexthop)) {
      return item.second.getInterface();
    }
  }

  throw std::runtime_error("Non reacheable " + nexthop + " network");
}

/*
* Given a route, add it to the datapath in case its cost is the best one
* towards the given destination
*/
void Router::add_active_nexthop_in_ebpf_map(const std::string &network,
                                    const std::string &nexthop,
                                    int32_t new_pathcost,
                                    const int &port_id) {
  int index = port_id;
  std::string new_nexthop = nexthop;
  int32_t min = INT32_MAX;
  std::string ip_route;
  std::string netmask_route;
  split_ip_and_prefix(network, ip_route, netmask_route);

  // Iterate over the current elements of the routing table

  for (auto elem : routes_) {
    if (elem.second.getNetwork() == network) {
      if (elem.second.getNexthop() == "local") {
        logger()->debug(
            "There is a local entry. Route do not added to the datapath");
        return;  // there is a local entry yet
      }

      if (elem.second.pathcostIsSet() && elem.second.getPathcost() < min) {
        new_nexthop = elem.second.getNexthop();
        min = elem.second.getPathcost();
      }
    }
  }

  if (new_pathcost < min) {
    logger()->debug("The new path cost '{0}' is better than the previous '{1}'",
                    new_pathcost, min);
    new_nexthop = nexthop;  // the new path is better than the old one, so we
                            // update the nexthop
  } else {
    // in this case we don't update the nexthop, as the old one is better than
    // the new
    logger()->debug("The new path cost '{0}' is worse than the previous '{1}'",
                    new_pathcost, min);
    index = get_port(search_interface_from_nexthop(new_nexthop))->index();
  }

  // the key in the datapath is given by "network" and "mask length".
  // then, if the nexthop changes, we just replace its value.
  // Please, note that the pathcost is not provided to the datapath, which only
  // keeps the best route
  // for each destination

  auto routing_table = get_hash_table<rt_k, rt_v>("routing_table");

  rt_k key{
      .netmask_len = get_netmask_length(netmask_route),
      .network = ip_string_to_nbo_uint(ip_route),
  };

  rt_v value{
      .port = uint32_t(index),
      .nexthop = ip_string_to_nbo_uint(nexthop),
      .type = TYPE_NOLOCALINTERFACE,
  };

  routing_table.set(key, value);
}

/*
* Remove a route. The route is removed from the control plane and, if it is also
* in the data plane, from the data plane as well.
* If the control plane has another route for the involved network, such a route
* is then added to the data plane.
*/
void Router::remove_route(const std::string &network,
                          const std::string &nexthop) {
  std::tuple<string, string> key(network, nexthop);

  if (routes_.count(key) == 0)
    throw std::runtime_error("Route does not exist");

  routes_.erase(key);
  logger()->debug(
      "Removed route from control plane [network: {0} - nexthop: {1}]",
      network, nexthop);

  find_new_active_nexthop(network, nexthop);
}

/*
* Remove all routes but those that are local
*/
void Router::remove_all_routes() {
  if (routes_.size() == 0)
    throw std::runtime_error("No entry found in routing table");

  logger()->debug("The routing table in the control plane has {0} entries",
                  routes_.size());

  auto routing_table = get_hash_table<rt_k, rt_v>("routing_table");

  for (auto it = routes_.begin(); it != routes_.end();) {
    if ((it->second.getNexthop()) != "local") {
      std::string network = it->second.getNetwork();
      std::string nexthop = it->second.getNexthop();
      routes_.erase(it++);
      logger()->debug(
          "Removed route from control plane [network: {0} - nexthop: {1}]",
          network, nexthop);

      // retrieve the entry from the fast path for the "netmask" "network", and
      // get the nexthop in order to check whether the route is also in the fast
      // path
      std::string ip_route;
      std::string netmask_route;
      split_ip_and_prefix(network, ip_route, netmask_route);
      try {
        rt_k key{
            .netmask_len = ip_string_to_nbo_uint(netmask_route),
            .network = ip_string_to_nbo_uint(ip_route),
        };

        rt_v value = routing_table.get(key);

        if (value.nexthop == ip_string_to_nbo_uint(nexthop)) {
          //  the nexthop in the fast path corresponds to that just removed in
          //  the control path
          //  then, the entry in the fast path is removed
          logger()->debug("Route found in the data path");
          routing_table.remove(key);
        } else
          logger()->debug("Route not found in the data path");
      } catch (...) {
        logger()->debug("Route not found in the data path");
      }
    } else {
      logger()->debug(
          "Local route not removed [network: {0} - nexthop: {1}]",
          it->second.getNetwork(), it->second.getNexthop());
      it++;
    }
  }
}

/*
* Add two local routes to the routing table
*/
void Router::add_local_route(const std::string &interface_ip,
                             const std::string &port_name,
                             const int port_index) {
  /*
  * Add the route to interface_ip/32 in the data path
  * The entry has key ("port ip address" "32") and value ("0.0.0.0" "local
  * interface")
  */

  std::string ip_route;
  std::string netmask_route;
  split_ip_and_prefix(interface_ip, ip_route, netmask_route);

  auto routing_table = get_hash_table<rt_k, rt_v>("routing_table");

  rt_k key{
      .netmask_len = 32, /* /32 -> 255.255.255.255 */
      .network = ip_string_to_nbo_uint(ip_route),
  };

  rt_v value{
      .port = uint32_t(port_index),
      .nexthop = 0, /* 0.0.0.0 */
      .type = TYPE_LOCALINTERFACE,
  };

  routing_table.set(key, value);

  logger()->info(
      "Added route [network: {0}/32 - nexthop: {1} - interface: {2}]",
      ip_route, "0.0.0.0", port_name);

  /*
  * Add a route to the local network, i.e., to the network directly reachable
  * through the inteface
  */

  // Add the route in the fast path
  uint32_t networkDec = ip_string_to_nbo_uint(ip_route) &
                        ip_string_to_nbo_uint(netmask_route);
  std::string network = nbo_uint_to_ip_string(networkDec);
  rt_k key2{
      .netmask_len = get_netmask_length(netmask_route),
      .network = networkDec,
  };

  rt_v value2{
      .port = uint32_t(port_index),
      .nexthop = 0, /* 0.0.0.0 */
      .type = TYPE_NOLOCALINTERFACE,
  };

  routing_table.set(key2, value2);

  logger()->info(
      "Added route [network: {0}/{1} - nexthop: {2} - interface: {3}]",
      network, get_netmask_length(netmask_route), "0.0.0.0", port_name);

  // Add the route in the table of the control plane

  std::string nexthop("local");
  std::string route = network + "/" +
              std::to_string(get_netmask_length(netmask_route));
  std::tuple<string, string> keyF(route, nexthop);
  uint32_t pathcost = 0;

  routes_.emplace(std::piecewise_construct, std::forward_as_tuple(keyF),
                  std::forward_as_tuple(*this, route, nexthop,
                                        port_name, pathcost));

  // FIXME: add also the /32 route?
}

/*
* Remove two local route from the routing table
*/
void Router::remove_local_route(const std::string &interface_ip,
                                const std::string &port_name) {
  std::string ip_route;
  std::string netmask_route;
  split_ip_and_prefix(interface_ip, ip_route, netmask_route);
  std::string network = get_network_from_ip(ip_route, netmask_route);
  std::string route = network + "/" +
              std::to_string(get_netmask_length(netmask_route));

  logger()->debug("Removing routes involving the port {0} and the network {1}",
                  port_name, network);

  /*
  * Delete the route corresponding to the network directly connected to the port
  * The route is deleted both from the control plane and from the data plane
  */
  for (auto it = routes_.begin(); it != routes_.end();) {
    if (it->second.getNexthop() == "local" &&
        it->second.getInterface() == port_name &&
        it->second.getNetwork() == route) {
      routes_.erase(it++);  // remove the route from the control plane
      logger()->debug(
          "Removed local route from the control plane [network: {0} "
          "- nexthop: {1} - interface: {2}]",
          route, "0.0.0.0", port_name);

      // Remove or update the route in the data path
      find_new_active_nexthop(route, "0.0.0.0");
      break;
    }

    else
      ++it;
  }

  /*
  * Delete the routes having the next hop in the same network of the address
  * that we are going to remove
  * If the route is also in the fast path, then
  * - delete it if there is no other route for the same network
  * - update it if there is, in the control plane, another route for the same
  * network
  */

  logger()->debug("Looking for other routes involving the port to be removed");

  for (auto it = routes_.begin(); it != routes_.end();) {
    if (it->second.getNexthop() !=
            "local" /*"local" are those networks directly connected*/ &&
        address_in_subnet(it->second.getNexthop(), netmask_route,
                          network)) {
      //  it->second.getInterface() == port_name) {

      // If the nexthop is the same network of the address we are going to
      // remove, then the route must be removed as well

      std::string cur_network(it->second.getNetwork());
      std::string cur_nexthop(it->second.getNexthop());
      std::string cur_interface(it->second.getInterface());

      routes_.erase(it++);

      logger()->debug(
          "Removed local route from the control plane [network: {0} "
          "- nexthop: {1} - interface: {2}]",
          cur_network, cur_nexthop, cur_interface);

      // check if the entry is also in the fast path and either remove or update
      // it, according to the
      // fact that there is another nexthop for that network or not
      find_new_active_nexthop(cur_network, cur_nexthop);
    } else
      ++it;
  }

  // Finally, remove the port_ip/32 route from the data path
  auto routing_table = get_hash_table<rt_k, rt_v>("routing_table");

  struct rt_k key {
    .netmask_len = 32, /* /32 -> 255.255.255.255 */
    .network = ip_string_to_nbo_uint(ip_route),
  };

  routing_table.remove(key);
}

/*
* Looks for a new nexthop for a destination network.
* If it is found, the route in the fastpath is updated, otherwise it is removed
*/
void Router::find_new_active_nexthop(const std::string &network,
                                     const std::string &nexthop) {
  std::string ip_route;
  std::string netmask_route;
  split_ip_and_prefix(network, ip_route, netmask_route);
  auto routing_table = get_hash_table<rt_k, rt_v>("routing_table");

  rt_k key{
      .netmask_len = get_netmask_length(netmask_route),
      .network = ip_string_to_nbo_uint(ip_route),
  };
  rt_v value = routing_table.get(key);

  if (value.nexthop == ip_string_to_nbo_uint(ip_route)) {
    //  the nexthop in the fast path corresponds to that just removed in the
    //  control path
    //  then, the entry in the fast path is removed if no other nexthop is
    //  found, otherwise it is just updated
    //  if more than one other nexthop exists, we use that with smallest cost

    logger()->debug("Route found in the data path");

    int32_t min = INT32_MAX;
    std::string new_nexthop;
    for (auto elem : routes_) {
      if (elem.second.getNetwork() == network) {
        logger()->debug(
            "Alternative route found for network [network: {0} "
            "- nexthop: {1} - interface: {2} - cost: {3}]",
            elem.second.getNetwork(), elem.second.getNexthop(),
            elem.second.getInterface(), elem.second.getPathcost());

        // these entry is for the network we are considering

        if (elem.second.getNexthop() == "local") {
          // a local entry is found for such a network. However it is very
          // strange. In fact,
          // a network directly connected should have the smallest cost, then be
          // alway active
          new_nexthop = "0.0.0.0";
          min = 0;
          break;
        }

        if (elem.second.pathcostIsSet() && elem.second.getPathcost() < min) {
          // the cost is better then the current one, then this is a candidate
          // nexthop
          new_nexthop = elem.second.getNexthop();
          min = elem.second.getPathcost();
        }
      }
    }

    if (new_nexthop.empty()) {
      //  no alternative nexthop is found, then the route is removed from the
      //  fastpath
      routing_table.remove(key);
      return;
    }

    else {
      int port_id =
          get_port(search_interface_from_nexthop(new_nexthop))->index();
      routing_table.set(key, rt_v{.port = uint32_t(port_id),
                                  .nexthop = ip_string_to_nbo_uint(new_nexthop),
                                  .type = TYPE_NOLOCALINTERFACE});
    }
  } else
    logger()->debug("Route not found in the data path");
}

/*
* Methods to manage packets coming from the fast path
*/

void Router::handle_router_pkt(Port &port, PacketInMetadata &md,
                               const std::vector<uint8_t> &packet) {
  EthernetII p(&packet[0], packet.size());
  IPv4Address src_ip(md.metadata[0]);
  IPv4Address dst_ip(md.metadata[1]);
  int protocol = md.metadata[2];

  /*check the packet protocol to prepare the correct response*/
  /*To implement: handle other type of pkts */
  if (protocol == IPPROTO_ICMP) {
    // Retrieve the ICMP MESSAGE and check what kind of message it is
    ICMP &icmp_payload = p.rfind_pdu<ICMP>();

    if (icmp_payload.type() == ICMP::ECHO_REQUEST) {
      /*echo request for one router interface, prepare the response */
      logger()->info("new echo request arrived from {0} to {1}",
                     src_ip.to_string(), dst_ip.to_string());
      EthernetII echoreply_packet =
          make_echo_reply(p, src_ip, dst_ip, icmp_payload);

      port.send_packet_out(echoreply_packet);
    }
  }

  return;
}

EthernetII Router::make_echo_reply(EthernetII &origin,
                                   const IPv4Address &src_ip,
                                   const IPv4Address &dst_ip, ICMP &old_icmp) {
  EthernetII echoreply_packet(origin.src_addr(), origin.dst_addr());

  IP ip_header = IP(src_ip, dst_ip);
  ip_header.ttl(64);

  echoreply_packet /= ip_header;

  // FLAG ECHO REPLY
  ICMP::Flags fl = ICMP::ECHO_REPLY;

  old_icmp.type(fl);

  echoreply_packet /= old_icmp;

  return echoreply_packet;
}

void Router::generate_icmp_ttlexceed(Port &port, PacketInMetadata &md,
                                     const std::vector<uint8_t> &packet) {
  /* packet is dropped and the router sends an ICMP TTL exceeded packet */

  EthernetII p(&packet[0], packet.size());

  IP &ip = p.rfind_pdu<IP>();
  IPv4Address dst_ip(ip.src_addr());
  IPv4Address src_ip(md.metadata[0]);

  logger()->info("send ICMP packet TIME_EXCEDEED code to host {0}",
                 dst_ip.to_string());
  // take the upper layer PDU and create a new PDU with the first 8 bytes
  PDU::serialization_type pdu = ip.inner_pdu()->serialize();

  ip.inner_pdu(RawPDU(&pdu[0], 8));

  // prepare the ICMP packet
  EthernetII icmp_packet(p.src_addr(), p.dst_addr());
  /* the src_addr become the destinationaddress,
   * the same for dst_addr
   */

  // IP HEADER: source is src_ip, destination is dst_ip
  IP ip_header = IP(dst_ip, src_ip);

  ip_header.ttl(64);  // default 128

  icmp_packet /= ip_header;

  // ICMP MESSAGE
  ICMP::Flags fl = (ICMP::Flags)11;

  ICMP icmp = ICMP(fl);  // FLAG TIME_EXCEDEED

  // add the old IP header in the ICMP message
  icmp.inner_pdu(ip);

  // add ICMP message in the response packet
  icmp_packet /= icmp;

  port.send_packet_out(icmp_packet);
}

void Router::generate_arp_request(Port &port, PacketInMetadata &md,
                                  const std::vector<uint8_t> &packet) {
  /*target ip is the nexhtop of a routing table entry or the destination host if
   * the route is local, passed from datapath*/

  unsigned int target_ip = md.metadata[0];  // in network byte order
  int index = md.metadata[1];               // out port index

  unsigned int src_ip = md.metadata[2];  // the primary or one of the secondary
                                         // addresses of the port

  auto port_out = get_port(index);
  std::string src_mac = port_out->getMac();

  // save packet using the target_ip for send it after the arp reply
  mu.lock();
  auto iter = arp_request_map.find(target_ip);
  if (iter == arp_request_map.end()) {  // no queue present, create one
    CircularBuffer q;
    q.enQueue(packet);
    arp_request_map[target_ip] = q;
  }

  else {  // queue exists, add element
    CircularBuffer q = iter->second;
    q.enQueue(packet);
    arp_request_map[target_ip] = q;
  }
  mu.unlock();
  /* prepare the arp request
     source mac is port src_mac
     source ip is src_ip
     target ip is target_ip
 */

  IPv4Address target_ip_addr(target_ip);
  IPv4Address src_ip_addr(src_ip);
  HWAddress<6> src_mac_addr(src_mac);
  logger()->debug(
      "sending ARP request on port {0} 'who has {1} tell MAC to {2}",
      port_out->name(), target_ip_addr.to_string(), src_ip_addr.to_string());
  EthernetII arp_request_packet = ARP::make_arp_request(
      target_ip_addr, src_ip_addr,
      src_mac_addr);  // check if parameters format are correct

  port_out->send_packet_out(arp_request_packet);
}

void Router::generate_arp_reply(Port &port, PacketInMetadata &md,
                                const std::vector<uint8_t> &packet) {
  EthernetII arp_reply(&packet[0], packet.size());

  unsigned int src_ip = md.metadata[0];
  logger()->info("ARP reply '{0} is at {1}'", IPv4Address(src_ip).to_string(),
                 arp_reply.src_addr().to_string());

  // find the packet to send
  mu.lock();
  auto iter = arp_request_map.find(src_ip);

  if (iter != arp_request_map.end()) {
    // queue found, send a packet to the port
    CircularBuffer q = iter->second;

    if (q.isEmpty()) {  // the buffer is empty, delete the value from the map
      logger()->info("packets buffer is empty");
      arp_request_map.erase(src_ip);
    }

    else {
      std::vector<uint8_t> reply_packet = q.deQueue();

      if (q.isEmpty())  // the buffer is empty, delete the value from the map
        arp_request_map.erase(src_ip);
      else
        arp_request_map[src_ip] = q;  // update the queue

      // update the mac addresses and send packet
      EthernetII ethframe(&reply_packet[0], reply_packet.size());

      ethframe.src_addr(arp_reply.dst_addr());
      ethframe.dst_addr(arp_reply.src_addr());

      port.send_packet_out(ethframe);
    }
  } else {
    if (get_shadow()) {
      port.send_packet_ns(arp_reply);
    } else {
      // no packet found, don't reply
      logger()->info("no packet found for ARP reply");
    }
  }
  mu.unlock();
}

/* Netlink */
void Router::netlink_notification_route_added(int ifindex, const std::string &info_route) {
  std::lock_guard<std::mutex> guard(router_mutex);

  // Check if the interface is of this cube
  std::string name_port = get_veth_name_from_index(ifindex);
  if (!name_port.empty()) {
    // update the routing table
    // info_route = network/prefix/nexthop
    std::istringstream split(info_route);
    std::vector<std::string> info;

    char split_char = '/';
    for (std::string each; std::getline(split, each, split_char); info.push_back(each));
    std::string network = info[0];
    std::string prefix = info[1];
    std::string nexthop = info[2];
    if (nexthop == "-")
      nexthop = "0.0.0.0";

    try {
      int port_index = get_port(name_port)->index();
      add_linux_route(network, prefix, nexthop, name_port, port_index);
    } catch (std::exception &e) {
      logger()->trace("route added - False netlink notification: {0}", e.what());
    }

  }
}

void Router::netlink_notification_route_deleted(int ifindex, const std::string &info_route) {
  std::lock_guard<std::mutex> guard(router_mutex);

  // Check if the interface is of this cube
  std::string name_port = get_veth_name_from_index(ifindex);
  if (!name_port.empty()) {
    // update the routing table
    // info_route = network/prefix/nexthop
    std::istringstream split(info_route);
    std::vector<std::string> info;

    char split_char = '/';
    for (std::string each; std::getline(split, each, split_char); info.push_back(each));
    std::string network = info[0];
    std::string prefix = info[1];
    std::string nexthop = info[2];
    if (nexthop == "-")
      nexthop = "0.0.0.0";

    uint32_t prefix_ = std::stoi(prefix);
    std::string netmask = utils::get_netmask_from_prefixlength(prefix_);
    try {
      std::string ip_port = get_port(name_port)->getIp();

      // remove the route only if it is not local
      if (!address_in_subnet(ip_port, netmask, network) && prefix_ != 32)
        remove_linux_route(network, prefix, nexthop, name_port);

    } catch (std::exception &e) {
      logger()->trace("route deleted - False netlink notification: {0}", e.what());
    }

  }
}

void Router::netlink_notification_link_deleted(int ifindex, const std::string &iface) {
  std::lock_guard<std::mutex> guard(router_mutex);

  // Check if the interface is of this cube
  std::string name_port = get_veth_name_from_index(ifindex);
  if (!name_port.empty()) {
    try {
      if (get_port(name_port)) {
        delPorts(name_port);
      }
    } catch (std::exception &e) {
      logger()->trace("link_deleted - False netlink notification: {0}", e.what());
    }
  }
}

void Router::netlink_notification_new_address(int ifindex, const std::string &new_ip) {
  std::lock_guard<std::mutex> guard(router_mutex);

  // Check if the interface is of this cube
  std::string name_port = get_veth_name_from_index(ifindex);
  if (!name_port.empty()) {
    try {
      auto port = get_port(name_port);
      std::string old_ip = port->getIp();

      if (old_ip != new_ip)
        port->doSetIp(new_ip);

    } catch (std::exception &e) {
      logger()->trace("new_address - False netlink notification: {0}", e.what());
    }
  }
}

// Add Linux route to the routing table
void Router::add_linux_route(const std::string &network,
                             const std::string &prefix,
                             const std::string &nexthop,
                             const std::string &port_name,
                             const int port_index) {
  // Add the network reachable through the inteface
  uint32_t prefix_ = std::stoi(prefix);
  auto routing_table = get_hash_table<rt_k, rt_v>("routing_table");

  rt_k key{
      .netmask_len = prefix_,
      .network = ip_string_to_nbo_uint(network),
  };

  try {
    rt_v value1 = routing_table.get(key);

    if (value1.nexthop == ip_string_to_nbo_uint(nexthop)) {
      logger()->trace("This route already exists, it is not added");
      return;
    } else
      logger()->trace("Route not found in the data path");
  } catch (...) {
    logger()->trace("Route not found in the data path");
  }

  // add route because not found in the data path
  rt_v value{
      .port = uint32_t(port_index),
      .nexthop = ip_string_to_nbo_uint(nexthop),
      .type = TYPE_NOLOCALINTERFACE,
  };
  routing_table.set(key, value);

  logger()->info("Added Linux route [network: {0}/{1} - nexthop: {2} - interface: {3}]",
                  network, prefix, nexthop, port_name);

  // Add the route in the table of the control plane
  std::string route = network + "/" + prefix;
  std::tuple<string, string> keyF(route, nexthop);
  uint32_t pathcost = 1;

  routes_.emplace(std::piecewise_construct, std::forward_as_tuple(keyF),
                  std::forward_as_tuple(*this, route, nexthop, port_name,
                                        pathcost));
}

// Remove Linux route from the routing table
void Router::remove_linux_route(const std::string &network,
                                const std::string &prefix,
                                const std::string &nexthop,
                                const std::string &port_name) {
  uint32_t prefix_ = std::stoi(prefix);
  auto routing_table = get_hash_table<rt_k, rt_v>("routing_table");

  rt_k key{
    .netmask_len = prefix_,
    .network = ip_string_to_nbo_uint(network),
  };

  try {
    rt_v value = routing_table.get(key);

    if (value.nexthop == ip_string_to_nbo_uint(nexthop)) {
      routing_table.remove(key);
    } else {
      logger()->trace("Route not found in the data path");
      return;
    }
  } catch (...) {
    logger()->trace("Route not found in the data path");
    return;
  }

  logger()->info("Removed Linux route [network: {0}/{1} - nexthop: {2} - interface: {3}]",
                network, prefix, nexthop, port_name);

  // remove the route in the table of the control plane
  std::string route = network + "/" + prefix;
  std::tuple<string, string> key_cp(route, nexthop);
  if (routes_.count(key_cp) != 0)
    routes_.erase(key_cp);
}
