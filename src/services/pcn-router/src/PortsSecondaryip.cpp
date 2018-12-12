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


#include "PortsSecondaryip.h"
#include "Router.h"

#include <stdexcept>

PortsSecondaryip::PortsSecondaryip(Ports &parent, const PortsSecondaryipJsonObject &conf): parent_(parent) {
  ip_ = conf.getIp();
  netmask_ = conf.getNetmask();

  parent.logger()->debug("Adding secondary address [ip: {0} - netmask: {1}]",ip_,netmask_);

  //TODO: check that no other router port exists in the same network

  /*
  * Add two routes in the routing table
  */
  int index = parent.parent_.Cube::get_port(parent.getName())->index();
  parent.parent_.add_local_route(ip_, netmask_, parent.getName(),index);
}

PortsSecondaryip::~PortsSecondaryip() { }

void PortsSecondaryip::update(const PortsSecondaryipJsonObject &conf) {
  //This method updates all the object/parameter in PortsSecondaryip object specified in the conf JsonObject.
  //You can modify this implementation.


}

PortsSecondaryipJsonObject PortsSecondaryip::toJsonObject(){
  PortsSecondaryipJsonObject conf;


  conf.setIp(getIp());

  conf.setNetmask(getNetmask());

  return conf;
}


void PortsSecondaryip::create(Ports &parent, const std::string &ip, const std::string &netmask, const PortsSecondaryipJsonObject &conf){

  /*
  * First create the port in the control plane
  */
  createInControlPlane(parent,ip,netmask, conf);

  /*
  * Then update the port in the data path (this also adds the proper routes in the routing table)
  */
  updatePortInDataPath(parent);
}

//TODO: move in the Ports class? Probably it is better
void PortsSecondaryip::updatePortInDataPath(Ports &parent) {

  auto router_port = parent.parent_.get_hash_table<uint16_t, r_port>("router_port");

  r_port value {
    .ip = utils::ip_string_to_be_uint(parent.getIp()),
    .netmask = utils::ip_string_to_be_uint(parent.getNetmask()),
    .secondary_ip = {},
    .secondary_netmask = {},
    .mac = utils::mac_string_to_be_uint(parent.getMac()),
  };

  uint16_t index = parent.parent_.get_port(parent.getName())->index();
  int i = 0;
  for (auto &addr : parent.getSecondaryipList()) {
    value.secondary_ip[i] = utils::ip_string_to_be_uint(addr->getIp());
    value.secondary_netmask[i] = utils::ip_string_to_be_uint(addr->getNetmask());
    i++;
  }

  router_port.set(index, value);
}

void PortsSecondaryip::createInControlPlane(Ports &parent, const std::string &ip, const std::string &netmask, const PortsSecondaryipJsonObject &conf){

  //This method creates the actual PortsSecondaryip object given thee key param.

  //TODO: a port cannot have more than SECONDARY_ADDRESS secondary addresses. This constraint come
  //from a constant defined in the fast path

  parent.logger()->info("Adding secondary address [port: {0} - ip: {1} - netmask: {2}]",parent.getName(),ip,netmask);

  parent.logger()->info("Port {0} has already {1} secondary addresses",parent.getName(),parent.secondary_ips_.size());
  auto ret = parent.secondary_ips_.emplace(PortsSecondaryip(parent, conf));
  parent.logger()->info("Now port {0} has {1} secondary addresses",parent.getName(),parent.secondary_ips_.size());

}

std::shared_ptr<PortsSecondaryip> PortsSecondaryip::getEntry(Ports &parent, const std::string &ip, const std::string &netmask){
  //This method retrieves the pointer to PortsSecondaryip object specified by its keys.

  parent.logger()->debug("Getting secondary ip [port: {0} - ip: {1} - netmask: {2}]",parent.getName(),ip,netmask);

  for(auto p : parent.secondary_ips_) {
    if((p.ip_ == ip) && (p.netmask_== netmask))
      return std::make_shared<PortsSecondaryip>(p);
  }
}

void PortsSecondaryip::removeEntry(Ports &parent, const std::string &ip, const std::string &netmask){
  //This method removes the single PortsSecondaryip object specified by its keys.
  //Remember to call here the remove static method for all-sub-objects of PortsSecondaryip.

  parent.logger()->info("Remove secondary address [ip: {0} - netmask: {1}] from port {2}", ip, netmask, parent.getName());

  //Check that the secondary address exists
  bool found = false;
  for(auto &addr : parent.secondary_ips_ ) {
    if((addr.ip_ == ip) && (addr.netmask_ == netmask)) {
      found = true;
      // remove the port from the data structure
      parent.secondary_ips_.erase(addr);
      break;
    }
  }
  if(!found)
     throw std::runtime_error("Address does not exist (or it is not a secondary address)");

  //change the port in the datapath
  updatePortInDataPath(parent);

  parent.parent_.remove_local_route(ip,netmask,parent.getName());
}

std::vector<std::shared_ptr<PortsSecondaryip>> PortsSecondaryip::get(Ports &parent){
  //This methods get the pointers to all the PortsSecondaryip objects in Ports.

  parent.logger()->debug("Getting all the {1} secondary addresses of port: {0}",parent.getName(),parent.secondary_ips_.size());

  std::vector<std::shared_ptr<PortsSecondaryip>> ips_vect;
  for(auto &it : parent.secondary_ips_)
    ips_vect.push_back(PortsSecondaryip::getEntry(parent, it.ip_,it.netmask_));

  return ips_vect;
}

void PortsSecondaryip::remove(Ports &parent){
  //This method removes all PortsSecondaryip objects in Ports.
  //Remember to call here the remove static method for all-sub-objects of PortsSecondaryip.

  parent.logger()->info("Removing all the secondary addresses of port {0}",parent.getName());

  for(auto it = parent.secondary_ips_.begin(); it != parent.secondary_ips_.end();) {
    auto tmp = it;
    it++;
    const std::string ip = tmp->ip_;
    const std::string netmask = tmp->netmask_;
    PortsSecondaryip::removeEntry(parent, ip, netmask);
  }

}

std::string PortsSecondaryip::getIp(){
  //This method retrieves the ip value.
  return ip_;
}


std::string PortsSecondaryip::getNetmask(){
  //This method retrieves the netmask value.
  return netmask_;
}

std::shared_ptr<spdlog::logger> PortsSecondaryip::logger() {
  return parent_.logger();
}
