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

#include "Ports.h"
#include "Router.h"

#include "../../../polycubed/src/utils/netlink.h"
#include "../../../polycubed/src/utils/ns.h"

Ports::Ports(polycube::service::Cube<Ports> &parent,
    std::shared_ptr<polycube::service::PortIface> port,
    const PortsJsonObject &conf)
    : PortsBase(parent, port) {
  if (conf.macIsSet())
    mac_ = conf.getMac();
  else {
    mac_ = get_random_mac();
    // update mac in the namespace
    if (parent_.get_shadow()) {
      std::function<void()> doThis = [&]{
        parent_.netlink_instance_router_.set_iface_mac(getName(), mac_);
      };
      polycube::polycubed::Namespace namespace_ =
          polycube::polycubed::Namespace::open("pcn-" + parent_.get_name());
      namespace_.execute(doThis);
    }
  }

  std::string ip_address;
  std::string netmask;
  uint32_t ip_uint;
  uint32_t netmask_uint;
  if (conf.ipIsSet()) {
    // TODO: check that no other router port exists in the same network
    ip_ = conf.getIp();
    split_ip_and_prefix(ip_, ip_address, netmask);
    ip_uint = ip_string_to_nbo_uint(ip_address);
    netmask_uint = ip_string_to_nbo_uint(netmask);
  } else {
    ip_ = "";
    ip_uint = 0;
    netmask_uint = 0;
  }

  /*
  * Add the port to the datapath
  */
  auto router_port = parent.get_hash_table<uint16_t, r_port>("router_port");
  r_port value{
      .ip = ip_uint,
      .netmask = netmask_uint,
      .secondary_ip = {},
      .secondary_netmask = {},
      .mac = mac_string_to_nbo_uint(mac_),
  };

  uint16_t index = this->index();
  const std::vector<PortsSecondaryipJsonObject> secondary_ips =
      conf.getSecondaryip();
  int i = 0;
  for (auto &addr : secondary_ips) {
    std::string secondaryIp = addr.getIp();
    split_ip_and_prefix(secondaryIp, ip_address, netmask);
    ip_uint = ip_string_to_nbo_uint(ip_address);
    netmask_uint = ip_string_to_nbo_uint(netmask);
    value.secondary_ip[i] = ip_uint;
    value.secondary_netmask[i] = netmask_uint;
    i++;
  }

  router_port.set(index, value);

  logger()->info(
      "Added new port: {0} (index: {3}) [mac: {1} - ip: {2}]",
      getName(), mac_, ip_, index);
  for (auto &addr : secondary_ips)
    logger()->info("\t secondary address: [ip: {0}]", addr.getIp());

  if (conf.ipIsSet()) {
    /*
    * Add two routes in the routing table
    */
    parent_.add_local_route(conf.getIp(), getName(), index);
  }

  /*
  * Create an object representing the secondary IP and add the routes related to
  * the secondary ips
  */

  for (auto &addr : secondary_ips) {
    PortsSecondaryip::createInControlPlane(*this, addr.getIp(), addr);
  }

  // lambda function to align IP and Netmask between Port and ExtIface
  ParameterEventCallback f_ip;
  f_ip = [&](const std::string param_name, const std::string new_ip) {
    try {
      if (!new_ip.empty()) {
        if (ip_ == new_ip)
          return;

        // set the ip address of the netdev on the port
        logger()->debug("Align ip of the port {0}", name());
        doSetIp(new_ip);
      }
    } catch (std::exception &e) {
      logger()->trace("iface_ip_notification - False ip notification: {0}",
                      e.what());
    }
  };

  // lambda function to align MAC between Port and ExtIface
  ParameterEventCallback f_mac;
  f_mac = [&](const std::string param_name, const std::string new_mac) {
    try {
      if (mac_ == new_mac)
        return;

      // set the mac address of the netdev on the port
      logger()->debug("Align mac of port {0}", name());
      doSetMac(new_mac);
    } catch (std::exception &e) {
      logger()->trace("iface_mac_notification - False mac notification: {0}",
                      e.what());
    }
  };

  if (!parent_.get_shadow()) {
    /* Register the new port to IP and MAC notifications arriving from ExtIface
     * do not use define because strings are easier to manipulate
     * for future extensions */
    subscribe_peer_parameter("IP", f_ip);
    subscribe_peer_parameter("MAC", f_mac);
  }
}

Ports::~Ports() {
  if (!parent_.get_shadow()) {
    unsubscribe_peer_parameter("IP");
    unsubscribe_peer_parameter("MAC");
  }
}

std::string Ports::getIp() {
  // This method retrieves the ip value.
  return ip_;
}

void Ports::setIp(const std::string &value) {
  // This method set the ip value.
  if (ip_ == value) {
    return;
  }

  // update ip in the namespace
  if (parent_.get_shadow()) {
    std::function<void()> doThis = [&]{
      parent_.netlink_instance_router_.set_iface_cidr(getName(), value);
    };
    polycube::polycubed::Namespace namespace_ =
        polycube::polycubed::Namespace::open("pcn-" + parent_.get_name());
    namespace_.execute(doThis);
  }

  doSetIp(value);

  if (!parent_.get_shadow()) {
    // Align Port with the peer interface
    set_peer_parameter("IP", value);
  }
}

void Ports::doSetIp(const std::string &new_ip) {
  if (ip_ == new_ip) {
    return;
  }

  std::string ip_address;
  std::string netmask;
  split_ip_and_prefix(new_ip, ip_address, netmask);

  /* Update the port in the datapath */
  uint16_t index = this->index();
  auto router_port = parent_.get_hash_table<uint16_t, r_port>("router_port");

  try {
    r_port value = router_port.get(index);
    value.ip = ip_string_to_nbo_uint(ip_address);
    value.netmask = ip_string_to_nbo_uint(netmask);
    router_port.set(index, value);
  } catch (...) {
    logger()->error("Port {0} not found in the data path", this->name());
  }

  logger()->debug(
      "Updated IP port: {0} (index: {3}) [mac: {1} - ip: {2}]",
      getName(), getMac(), new_ip, index);

  /* Update routes in the routing table */
  if (!ip_.empty()) {
    parent_.remove_local_route(ip_, getName());
  }
  parent_.add_local_route(new_ip, getName(), index);

  ip_ = new_ip;
}

std::shared_ptr<PortsSecondaryip> Ports::getSecondaryip(const std::string &ip) {
  logger()->debug("Getting secondary ip [port: {0} - ip: {1}]",
                    getName(), ip);

  for (auto p : secondary_ips_) {
    if (p.getIp() == ip)
      return std::make_shared<PortsSecondaryip>(p);
  }
}

std::vector<std::shared_ptr<PortsSecondaryip>> Ports::getSecondaryipList() {
  logger()->debug("Getting all the {1} secondary addresses of port: {0}",
                  getName(), secondary_ips_.size());

  std::vector<std::shared_ptr<PortsSecondaryip>> ips_vect;
  for (auto it : secondary_ips_) {
    auto ip = it.getIp();
    ips_vect.push_back(getSecondaryip(ip));
  }
  return ips_vect;
}

void Ports::addSecondaryip(const std::string &ip, const PortsSecondaryipJsonObject &conf) {
  /*
  * First create the port in the control plane
  */
  logger()->info(
      "Adding secondary address [port: {0} - ip: {1}]", getName(), ip);

  auto ret = secondary_ips_.emplace(PortsSecondaryip(*this, conf));

  /*
  * Then update the port in the data path (this also adds the proper routes in
  * the routing table)
  */
  updatePortInDataPath();
}

// Basic default implementation, place your extension here (if needed)
void Ports::addSecondaryipList(const std::vector<PortsSecondaryipJsonObject> &conf) {
  // call default implementation in base class
  PortsBase::addSecondaryipList(conf);
}

// Basic default implementation, place your extension here (if needed)
void Ports::replaceSecondaryip(const std::string &ip, const PortsSecondaryipJsonObject &conf) {
  // call default implementation in base class
  PortsBase::replaceSecondaryip(ip, conf);
}

void Ports::delSecondaryip(const std::string &ip) {
  // Check that the secondary address exists
  bool found = false;
  for (auto addr : secondary_ips_) {
    if (addr.getIp() == ip) {
      found = true;
      // remove the port from the data structure
      secondary_ips_.erase(addr);
      break;
    }
  }
  if (!found)
    throw std::runtime_error(
        "Address does not exist (or it is not a secondary address)");

  // change the port in the datapath
  updatePortInDataPath();

  parent_.remove_local_route(ip, getName());
}

// Basic default implementation, place your extension here (if needed)
void Ports::delSecondaryipList() {
  // call default implementation in base class
  PortsBase::delSecondaryipList();
}

std::string Ports::getMac() {
  // This method retrieves the mac value.
  return mac_;
}

void Ports::setMac(const std::string &value) {
  // This method set the mac value.
  if (mac_ == value) {
    return;
  }

  // update mac in the namespace
  if (parent_.get_shadow()) {
    std::function<void()> doThis = [&]{
      parent_.netlink_instance_router_.set_iface_mac(getName(), mac_);
    };
    polycube::polycubed::Namespace namespace_ =
        polycube::polycubed::Namespace::open("pcn-" + parent_.get_name());
    namespace_.execute(doThis);
  }

  doSetMac(value);

  if (!parent_.get_shadow()) {
    // Align Port with the peer interface
    set_peer_parameter("MAC", value);
  }
}

void Ports::doSetMac(const std::string &new_mac) {
  if (mac_ == new_mac) {
    return;
  }

  /* Update the port in the datapath */
  uint16_t index = this->index();
  auto router_port = parent_.get_hash_table<uint16_t, r_port>("router_port");

  r_port map_value = router_port.get(index);
  map_value.mac = mac_string_to_nbo_uint(new_mac);
  router_port.set(index, map_value);

  logger()->debug(
      "Updated mac port: {0} (index: {3}) [mac: {1} - ip: {2}]",
      getName(), new_mac, getIp(), index);

  mac_ = new_mac;
}

void Ports::updatePortInDataPath() {
  std::string ip_address;
  std::string netmask;
  uint32_t ip_uint = 0;
  uint32_t netmask_uint = 0;

  split_ip_and_prefix(ip_, ip_address, netmask);
  ip_uint = ip_string_to_nbo_uint(ip_address);
  netmask_uint = ip_string_to_nbo_uint(netmask);

  auto router_port = parent_.get_hash_table<uint16_t, r_port>("router_port");
  r_port value{
      .ip = ip_uint,
      .netmask = netmask_uint,
      .secondary_ip = {},
      .secondary_netmask = {},
      .mac = mac_string_to_nbo_uint(mac_),
  };

  uint16_t index = this->index();
  int i = 0;
  for (auto addr : secondary_ips_) {
    std::string secondaryIp = addr.getIp();
    split_ip_and_prefix(secondaryIp, ip_address, netmask);
    ip_uint = ip_string_to_nbo_uint(ip_address);
    netmask_uint = ip_string_to_nbo_uint(netmask);
    value.secondary_ip[i] = ip_uint;
    value.secondary_netmask[i] = netmask_uint;
    i++;
  }

  router_port.set(index, value);
}
