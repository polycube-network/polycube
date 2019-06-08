/*
 * Copyright 2019 The Polycube Authors
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

#include <netlink/netlink.h>
#include <netlink/route/link.h>
#include <netlink/route/link/veth.h>

#include "veth.h"

namespace polycube {
namespace polycubed {

/* Class VethPeer */
VethPeer::VethPeer() : name_(""), ns_("") {}
VethPeer::~VethPeer() {}

void VethPeer::set_name(const std::string &name) {
  name_ = name;
}

std::string VethPeer::get_name() const {
  return name_;
}

void VethPeer::set_namespace(const std::string &ns) {
  Namespace namespace_ = Namespace::open(ns);
  int fd = namespace_.get_fd();
  Netlink::getInstance().move_iface_into_ns(name_, fd);

  ns_ = ns;
}

std::string VethPeer::get_namespace() const {
  return ns_;
}

void VethPeer::set_status(IFACE_STATUS status) {
  if (!ns_.empty()) {
    /* exec set_iface_status into namespace */
    std::function<void()> doThis = [&]{Netlink::getInstance().set_iface_status(name_, status);};
    Namespace namespace_ = Namespace::open(ns_);
    namespace_.execute(doThis);
  } else {
    Netlink::getInstance().set_iface_status(name_, status);
  }
}

void VethPeer::set_mac(const std::string &mac) {
  if (!ns_.empty()) {
    /* exec set_iface_mac into namespace */
    std::function<void()> doThis = [&]{Netlink::getInstance().set_iface_mac(name_, mac);};
    Namespace namespace_ = Namespace::open(ns_);
    namespace_.execute(doThis);
  } else {
    Netlink::getInstance().set_iface_mac(name_, mac);
  }
}

void VethPeer::set_ip(const std::string &ip, const int prefix) {
  if (!ns_.empty()) {
    /* exec set_iface_ip into namespace */
    std::function<void()> doThis = [&]{Netlink::getInstance().set_iface_ip(name_, ip, prefix);};
    Namespace namespace_ = Namespace::open(ns_);
    namespace_.execute(doThis);
  } else {
    Netlink::getInstance().set_iface_ip(name_, ip, prefix);
  }
}

/* Class Veth */
Veth Veth::create(const std::string &peerA, const std::string &peerB) {
  struct rtnl_link *link;
  struct nl_sock *sk;
  struct rtnl_link *peer;

  sk = nl_socket_alloc();
  if (nl_connect(sk, NETLINK_ROUTE) < 0)
    throw std::runtime_error("Veth::create - Unable to connect socket");

  link = rtnl_link_veth_alloc();
  if (!link)
    throw std::runtime_error("Veth::create - Unable to alloc veth");

  rtnl_link_set_name(link, peerA.c_str());
  peer = rtnl_link_veth_get_peer(link);
  rtnl_link_set_name(peer, peerB.c_str());

  if (rtnl_link_add(sk, link, NLM_F_CREATE | NLM_F_EXCL) < 0)
    throw std::runtime_error("Veth::Veth - Unable to add link");

  rtnl_link_put(peer);
  rtnl_link_put(link);
  nl_close(sk);

  return Veth(peerA, peerB);
}

Veth::Veth(const std::string &peerA, const std::string &peerB) {
  peerA_.set_name(peerA);
  peerB_.set_name(peerB);
}

Veth::~Veth() {}

void Veth::remove() {
  struct rtnl_link *link;
  struct nl_sock *sk;
  struct nl_cache *link_cache;

  sk = nl_socket_alloc();
  if (nl_connect(sk, NETLINK_ROUTE) < 0)
    throw std::runtime_error("Veth::remove - Unable to connect socket");

  if (rtnl_link_alloc_cache(sk, AF_UNSPEC, &link_cache) < 0) {
    nl_close(sk);
    throw std::runtime_error("Veth::remove - Unable to allocate cache");
  }

  link = rtnl_link_get_by_name(link_cache, peerA_.get_name().c_str());
  if (!link)
    link = rtnl_link_get_by_name(link_cache, peerB_.get_name().c_str());

  if (!link)
    throw std::runtime_error("Veth::remove - veth not found");

  if (rtnl_link_delete(sk, link) < 0) {
    nl_close(sk);
    throw std::runtime_error("Veth::remove - Unable to remove veth");
  }

  rtnl_link_put(link);
  nl_close(sk);

  peerA_.set_name("");
  peerB_.set_name("");
}

VethPeer& Veth::get_peerA() {
  return peerA_;
}
VethPeer& Veth::get_peerB() {
  return peerB_;
}

}  // namespace polycubed
}  // namespace polycube
