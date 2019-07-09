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

#include "Ports.h"
#include "Router.h"
#include "UtilityMethods.h"

#include "../../../polycubed/src/utils/netlink.h"
#include "../../../polycubed/src/utils/ns.h"

Ports::Ports(polycube::service::Cube<Ports> &parent,
             std::shared_ptr<polycube::service::PortIface> port,
             const PortsJsonObject &conf)
    : Port(port), parent_(static_cast<Router &>(parent)) {
  if (conf.macIsSet())
    mac_ = conf.getMac();
  else
    mac_ = polycube::service::utils::get_random_mac();

  if (!is_netmask_valid(
          conf.getNetmask()))  // TODO: not sure that the validation is needed
    throw std::runtime_error("Netmask is in invalid format");

  ip_ = conf.getIp();
  netmask_ = conf.getNetmask();

  std::string port_name(getName());

  // TODO: check that no other router port exists in the same network

  /*
  * Add the port to the datapath
  */

  auto router_port = parent.get_hash_table<uint16_t, r_port>("router_port");
  r_port value{
      .ip = utils::ip_string_to_be_uint(ip_),
      .netmask = utils::ip_string_to_be_uint(netmask_),
      .secondary_ip = {},
      .secondary_netmask = {},
      .mac = utils::mac_string_to_be_uint(mac_),
  };

  uint16_t index = this->index();
  const std::vector<PortsSecondaryipJsonObject> secondary_ips =
      conf.getSecondaryip();
  int i = 0;
  for (auto &addr : secondary_ips) {
    value.secondary_ip[i] = utils::ip_string_to_be_uint(addr.getIp());
    value.secondary_netmask[i] = utils::ip_string_to_be_uint(addr.getNetmask());
    i++;
  }

  router_port.set(index, value);

  logger()->info(
      "Added new port: {0} (index: {4}) [mac: {1} - ip: {2} - netmask: {3}]",
      getName(), mac_, ip_, netmask_, index);
  for (auto &addr : secondary_ips)
    logger()->info("\t secondary address: [ip: {0} - netmask: {1}]",
                   addr.getIp(), addr.getNetmask());

  /*
  * Add two routes in the routing table
  */
  parent_.add_local_route(conf.getIp(), conf.getNetmask(), getName(), index);

  /*
  * Create an object representing the secondary IP and add the routes related to
  * the secondary ips
  */

  for (auto &addr : secondary_ips) {
    PortsSecondaryip::createInControlPlane(*this, addr.getIp(),
                                           addr.getNetmask(), addr);
  }

  // lambda function to align IP and Netmask between Port and ExtIface
  ParameterEventCallback f_ip;
  f_ip = [&](const std::string param_name, const std::string cidr) {
    try {
      if (!cidr.empty()) {
        // set the ip address of the netdev on the port
        // cidr = ip_address/prefix
        std::istringstream split(cidr);
        std::vector<std::string> info;

        char split_char = '/';
        for (std::string each; std::getline(split, each, split_char);
             info.push_back(each))
          ;
        std::string new_ip = info[0];
        std::string new_netmask =
            utils::get_netmask_from_CIDR(std::stoi(info[1]));

        std::string old_ip = getIp();
        std::string old_netmask = getNetmask();

        logger()->debug("Align ip and netmask of port {0}", name());

        if (old_ip != new_ip)
          setIp_Netlink(new_ip);
        if (old_netmask != new_netmask)
          setNetmask_Netlink(new_netmask);
      }
    } catch (std::exception &e) {
      logger()->trace("iface_ip_notification - False ip notification: {0}",
                      e.what());
    }
  };

  // lambda function to align MAC between Port and ExtIface
  ParameterEventCallback f_mac;
  f_mac = [&](const std::string param_name, const std::string mac) {
    try {
      if (mac_ != mac) {
        logger()->debug("Align mac of port {0}", name());
        doSetMac(mac);
      }
    } catch (std::exception &e) {
      logger()->trace("iface_mac_notification - False mac notification: {0}",
                      e.what());
    }
  };

  if (!parent_.get_shadow()) {
    // Register the new port to IP and MAC notifications arriving from ExtIface
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

void Ports::update(const PortsJsonObject &conf) {
  // This method updates all the object/parameter in Ports object specified in
  // the conf JsonObject.
  // You can modify this implementation.
  Port::set_conf(conf.getBase());

  logger()->info("Updating port");

  setIp(conf.getIp());

  setNetmask(conf.getNetmask());

  if (conf.macIsSet()) {
    setMac(conf.getMac());
  }

  if (conf.secondaryipIsSet()) {
    for (auto &i : conf.getSecondaryip()) {
      auto ip = i.getIp();
      auto netmask = i.getNetmask();
      auto m = getSecondaryip(ip, netmask);
      m->update(i);
    }
  }
}

PortsJsonObject Ports::toJsonObject() {
  PortsJsonObject conf;
  conf.setBase(Port::to_json());

  conf.setIp(getIp());

  conf.setNetmask(getNetmask());

  conf.setMac(getMac());

  // Remove comments when you implement all sub-methods
  for (auto &i : getSecondaryipList()) {
    conf.addPortsSecondaryip(i->toJsonObject());
  }

  return conf;
}

std::string Ports::getIp() {
  // This method retrieves the ip value.

  return ip_;
}

void Ports::setIp(const std::string &value) {
  // This method set the ip value.
  if (ip_ != value) {
    // unset old ip
    if (parent_.get_shadow()) {
      int prefix = get_netmask_length(getNetmask());
      std::function<void()> doThis = [&]{
        parent_.netlink_instance_router_.delete_iface_ip(getName(), ip_, prefix);
      };
      polycube::polycubed::Namespace namespace_ = polycube::polycubed::Namespace::open("pcn-" + parent_.get_name());
      namespace_.execute(doThis);
    }

    setIp_Netlink(value);

    // set new ip
    if (parent_.get_shadow()) {
      int prefix = get_netmask_length(getNetmask());
      std::function<void()> doThis = [&]{
        parent_.netlink_instance_router_.add_iface_ip(getName(), value, prefix);
      };
      polycube::polycubed::Namespace namespace_ = polycube::polycubed::Namespace::open("pcn-" + parent_.get_name());
      namespace_.execute(doThis);
    }

    // Align Port with the peer interface
    int prefix = get_netmask_length(getNetmask());
    std::string cidr = ip_ + "/" + std::to_string(prefix);
    set_peer_parameter("IP", cidr);
  }
}

void Ports::setIp_Netlink(const std::string &value) {
  // This method set the ip value.
  if (ip_ != value) {
    std::string new_ip = value;
    /* Update the port in the datapath */
    uint16_t index = this->index();
    auto router_port = parent_.get_hash_table<uint16_t, r_port>("router_port");

    try {
      r_port value = router_port.get(index);
      value.ip = utils::ip_string_to_be_uint(new_ip);
    } catch (...) {
      logger()->error("Port {0} not found in the data path", this->name());
    }

    logger()->debug(
        "Updated IP port: {0} (index: {4}) [mac: {1} - ip: {2} - netmask: {3}]",
        getName(), getMac(), new_ip, getNetmask(), index);

    /* Update routes in the routing table */
    parent_.remove_local_route(ip_, getNetmask(), getName());
    parent_.add_local_route(new_ip, getNetmask(), getName(), index);

    ip_ = new_ip;
  }
}

std::string Ports::getNetmask() {
  // This method retrieves the netmask value.
  return netmask_;
}

void Ports::setNetmask(const std::string &value) {
  // This method set the netmask value.
  if (netmask_ != value) {
    // unset old netmask
    if (parent_.get_shadow()) {
      int prefix = get_netmask_length(netmask_);
      std::function<void()> doThis = [&]{
        parent_.netlink_instance_router_.delete_iface_ip(getName(), ip_, prefix);
      };
      polycube::polycubed::Namespace namespace_ = polycube::polycubed::Namespace::open("pcn-" + parent_.get_name());
      namespace_.execute(doThis);
    }

    setNetmask_Netlink(value);

    // set new netmask
    if (parent_.get_shadow()) {
      int prefix = get_netmask_length(value);
      std::function<void()> doThis = [&]{
        parent_.netlink_instance_router_.add_iface_ip(getName(), getIp(), prefix);
      };
      polycube::polycubed::Namespace namespace_ = polycube::polycubed::Namespace::open("pcn-" + parent_.get_name());
      namespace_.execute(doThis);
    }

    // Align Port with the peer interface
    int prefix = get_netmask_length(value);
    std::string cidr = getIp() + "/" + std::to_string(prefix);
    set_peer_parameter("IP", cidr);
  }
}

void Ports::setNetmask_Netlink(const std::string &value) {
  // This method set the netmask value.
  if (netmask_ != value) {
    std::string new_netmask = value;
    /* Update the port in the datapath */
    uint16_t index = this->index();
    auto router_port = parent_.get_hash_table<uint16_t, r_port>("router_port");

    try {
      r_port value = router_port.get(index);
      value.netmask = utils::ip_string_to_be_uint(new_netmask);
    } catch (...) {
      logger()->error("Port {0} not found in the data path", this->name());
    }

    logger()->debug(
        "Updated netmask port: {0} (index: {4}) [mac: {1} - ip: {2} - netmask: {3}]",
        getName(), getMac(), getIp(), new_netmask, index);

    /* Update routes in the routing table */
    parent_.remove_local_route(getIp(), netmask_, getName());
    parent_.add_local_route(getIp(), new_netmask, getName(), index);

    netmask_ = new_netmask;
  }
}

std::string Ports::getMac() {
  // This method retrieves the mac value.
  return mac_;
}

void Ports::setMac(const std::string &value) {
  // This method set the mac value.
  doSetMac(value);

  if (parent_.get_shadow()) {
    std::function<void()> doThis = [&]{
      parent_.netlink_instance_router_.set_iface_mac(getName(), mac_);
    };
    polycube::polycubed::Namespace namespace_ =
            polycube::polycubed::Namespace::open("pcn-" + parent_.get_name());
    namespace_.execute(doThis);
  }

  // Align Port with the peer interface
  set_peer_parameter("MAC", value);
}

void Ports::doSetMac(const std::string &new_mac) {
  if (mac_ == new_mac) {
    return;
  }

  /* Update the port in the datapath */
  uint16_t index = this->index();
  auto router_port = parent_.get_hash_table<uint16_t, r_port>("router_port");

  r_port map_value = router_port.get(index);
  map_value.mac = utils::mac_string_to_be_uint(new_mac);
  router_port.set(index, map_value);

  logger()->debug(
      "Updated mac port: {0} (index: {4}) [mac: {1} - ip: {2} - netmask: {3}]",
      getName(), new_mac, getIp(), getNetmask(), index);

  mac_ = new_mac;
}

std::shared_ptr<spdlog::logger> Ports::logger() {
  return parent_.logger();
}

std::shared_ptr<PortsSecondaryip> Ports::getSecondaryip(
    const std::string &ip, const std::string &netmask) {
  logger()->debug("Getting secondary ip [port: {0} - ip: {1} - netmask: {2}]",
                  getName(), ip, netmask);

  for (auto &p : secondary_ips_) {
    if ((p.getIp() == ip) && (p.getNetmask() == netmask))
      return std::make_shared<PortsSecondaryip>(p);
  }
}

std::vector<std::shared_ptr<PortsSecondaryip>> Ports::getSecondaryipList() {
  logger()->debug("Getting all the {1} secondary addresses of port: {0}",
                  getName(), secondary_ips_.size());

  std::vector<std::shared_ptr<PortsSecondaryip>> ips_vect;
  for (auto &it : secondary_ips_) {
    auto ip = it.getIp();
    auto netmask = it.getNetmask();
    ips_vect.push_back(getSecondaryip(ip, netmask));
  }

  return ips_vect;
}

void Ports::addSecondaryip(const std::string &ip, const std::string &netmask,
                           const PortsSecondaryipJsonObject &conf) {
  /*
  * First create the port in the control plane
  */
  logger()->info(
      "Adding secondary address [port: {0} - ip: {1} - netmask: {2}]",
      getName(), ip, netmask);

  auto ret = secondary_ips_.emplace(PortsSecondaryip(*this, conf));

  /*
  * Then update the port in the data path (this also adds the proper routes in
  * the routing table)
  */
  PortsSecondaryip::updatePortInDataPath(*this);
}

void Ports::addSecondaryipList(
    const std::vector<PortsSecondaryipJsonObject> &conf) {
  for (auto &i : conf) {
    std::string ip_ = i.getIp();
    std::string netmask_ = i.getNetmask();
    addSecondaryip(ip_, netmask_, i);
  }
}

void Ports::replaceSecondaryip(const std::string &ip,
                               const std::string &netmask,
                               const PortsSecondaryipJsonObject &conf) {
  delSecondaryip(ip, netmask);
  std::string ip_ = conf.getIp();
  std::string netmask_ = conf.getNetmask();
  addSecondaryip(ip_, netmask_, conf);
}

void Ports::delSecondaryip(const std::string &ip, const std::string &netmask) {
  // Check that the secondary address exists
  bool found = false;
  for (auto &addr : secondary_ips_) {
    if ((addr.getIp() == ip) && (addr.getNetmask() == netmask)) {
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
  PortsSecondaryip::updatePortInDataPath(*this);

  parent_.remove_local_route(ip, netmask, getName());
}

void Ports::delSecondaryipList() {
  for (auto it = secondary_ips_.begin(); it != secondary_ips_.end();) {
    auto tmp = it;
    it++;
    std::string ip = tmp->getIp();
    std::string netmask = tmp->getNetmask();
    delSecondaryip(ip, netmask);
  }
}
