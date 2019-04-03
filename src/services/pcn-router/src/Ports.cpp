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
}

Ports::~Ports() {}

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
  throw std::runtime_error("[Ports]: Method setIp not implemented");
}

std::string Ports::getNetmask() {
  // This method retrieves the netmask value.
  return netmask_;
}

void Ports::setNetmask(const std::string &value) {
  // This method set the netmask value.
  throw std::runtime_error("[Ports]: Method setNetmask not implemented");
}

std::string Ports::getMac() {
  // This method retrieves the mac value.
  return mac_;
}

void Ports::setMac(const std::string &value) {
  // This method set the mac value.
  throw std::runtime_error("[Ports]: Method setMac not implemented");
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