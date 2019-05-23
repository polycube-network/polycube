/*
 * Copyright 2017 The Polycube Authors
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

#include "netlink.h"

#include <libbpf.h>
#include <linux/if.h>
#include <iostream>

#include <arpa/inet.h>
#include <sstream>
#include <sys/socket.h>

#include "../exceptions.h"

#ifndef SOL_NETLINK
#define SOL_NETLINK 270
#endif

namespace polycube {
namespace polycubed {

class Netlink::NetlinkNotification {
 public:
  NetlinkNotification(Netlink *parent) : parent_(parent), running(true) {
    /* Allocate a new socket */
    struct sockaddr_nl addr;
    int option = 1;
    bzero (&addr, sizeof(addr));

    sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    addr.nl_family = AF_NETLINK;
    addr.nl_groups = (RTNLGRP_LINK | RTMGRP_IPV4_IFADDR | RTMGRP_IPV4_ROUTE) ;

    if (bind(sock,(struct sockaddr *)&addr,sizeof(addr)) < 0) {
      parent_->logger->error("Netlink error bind");
      close(sock);
    }

    if (setsockopt(sock, SOL_NETLINK, NETLINK_LISTEN_ALL_NSID,(char*)&option,sizeof(option)) < 0) {
      parent_->logger->error("Netlink error setsockopt");
      close(sock);
    }

    parent_->logger->debug("Started NetlinkNotification");

    thread_ = std::thread(&NetlinkNotification::execute_wait, this);
    thread_.detach();
  }

  void execute_wait() {
    int result;
    fd_set readset;
    struct timeval tv;

    while (running) {
      do {
        tv.tv_sec = NETLINK_TIMEOUT;
        tv.tv_usec = 0;
        FD_ZERO(&readset);
        FD_SET(sock, &readset);
        // The struct tv is decremented every time the select terminates.
        // If the value is not updated, the next time select is called uses
        // 0 as timeout value, behaving as a non-blocking socket.
        result = select(sock + 1, &readset, NULL, NULL, &tv);
      } while (result < 0 && errno == EINTR && running);

      if (result > 0) {
        if (FD_ISSET(sock, &readset)) {
          // The socket_fd has data available to be read
          int received_bytes = 0;
          struct nlmsghdr *nlh;
          char buffer[4096];

          bzero(buffer, sizeof(buffer));

          received_bytes = recv(sock, buffer, sizeof(buffer), 0);
          if (received_bytes < 0)
            parent_->logger->error("Netlink Notification error received_bytes");

          nlh = (struct nlmsghdr *) buffer;
          recv_func(sock,nlh, &Netlink::getInstance(),received_bytes);
        }
      }
    }
  }

  ~NetlinkNotification() {
    running = false;
    // TODO: I would prefer to avoid the timeout, the destructor for this object
    // is called
    // only once the program ends and the thread is killed
    close(sock);
  }

 private:
  struct nl_sock *sk;
  Netlink *parent_;
  bool running;
  std::thread thread_;
  static const long int NETLINK_TIMEOUT = 1;
  int sock;

  static int recv_func(int socket, struct nlmsghdr *msg, void *arg, int received_bytes) {
    Netlink *parent = (Netlink *)arg;
    struct nlmsghdr *nlh = msg;

    if (nlh->nlmsg_type == RTM_DELLINK) {
      struct ifinfomsg *iface = (struct ifinfomsg *)NLMSG_DATA(nlh);
      struct rtattr *hdr = IFLA_RTA(iface);
      if (hdr->rta_type == IFLA_IFNAME) {
        parent->notify_link_deleted(iface->ifi_index,
                                    std::string((char *)RTA_DATA(hdr)));
      }
    }

    if (nlh->nlmsg_type == RTM_NEWLINK) {
      struct ifinfomsg *iface = (struct ifinfomsg *)NLMSG_DATA(nlh);
      struct rtattr *hdr = IFLA_RTA(iface);
      if (hdr->rta_type == IFLA_IFNAME) {
        parent->notify_link_added(iface->ifi_index,
                                    std::string((char *)RTA_DATA(hdr)));
      }
    }

    if (nlh->nlmsg_type == RTM_NEWADDR) {
      struct ifaddrmsg *iface = (struct ifaddrmsg *) NLMSG_DATA(nlh);
      struct rtattr *hdr = IFA_RTA(iface);

      char address[32];
      char netmask[32];
      int rtl = IFA_PAYLOAD(nlh);

      while (rtl && RTA_OK(hdr, rtl)) {
        if (hdr->rta_type == IFA_LOCAL)
          inet_ntop(AF_INET, RTA_DATA(hdr), address, sizeof(address));
        hdr = RTA_NEXT(hdr, rtl);
      }

      /* Write the new information to a string (separated by '/').
         This string will be passed to the notify method */
      int netmask_len = iface->ifa_prefixlen;
      std::ostringstream info_new_address;
      info_new_address << address << "/" << netmask_len;
      std::string info_address = info_new_address.str();

      parent->notify_new_address(iface->ifa_index, info_address);
    }

    if (nlh->nlmsg_type == RTM_NEWROUTE || nlh->nlmsg_type == RTM_DELROUTE) {
      /* manage the routing table */
      struct rtmsg *route_entry; /* This struct represent a route entry in the
                                    routing table */
      struct rtattr *route_attribute; /* This struct contain route attributes
                                         (route type) */
      int route_attribute_len = 0;
      unsigned char route_netmask = 0;
      unsigned char route_protocol = 0;
      char destination_address[32];
      char gateway_address[32] = "-";
      int index = 0;
      int metrics = 0;

      route_entry = (struct rtmsg *)NLMSG_DATA(nlh);

      route_netmask = route_entry->rtm_dst_len;
      route_protocol = route_entry->rtm_protocol;
      route_attribute = (struct rtattr *)RTM_RTA(route_entry);

      /* Get the len route attribute */
      route_attribute_len = RTM_PAYLOAD(nlh);

      /* Loop through all attributes */
      for (; RTA_OK(route_attribute, route_attribute_len);
          route_attribute = RTA_NEXT(route_attribute, route_attribute_len)) {
        /* Route destination address */
        if (route_attribute->rta_type == RTA_DST) {
          inet_ntop(AF_INET, RTA_DATA(route_attribute), destination_address,
                    sizeof(destination_address));
        }

        /* The gateway of the route */
        if (route_attribute->rta_type == RTA_GATEWAY) {
          inet_ntop(AF_INET, RTA_DATA(route_attribute), gateway_address,
                    sizeof(gateway_address));
        }

        /* Output interface index */
        if (route_attribute->rta_type == RTA_OIF) {
          int *in = (int *)RTA_DATA(route_attribute);
          index = (int)*in;
        }
      }

    /* Write the route information to a string (separated by '/').
       This string will be passed to the notify method */
      std::ostringstream infoRoute;
      int net_len = route_netmask;
      infoRoute << destination_address << "/" << net_len << "/" << gateway_address;
      std::string info_route = infoRoute.str();

      if (nlh->nlmsg_type == RTM_DELROUTE) {
        parent->notify_route_deleted(index, info_route);
      } else if (nlh->nlmsg_type == RTM_NEWROUTE) {
        parent->notify_route_added(index, info_route);
      }
    }

    parent->notify_all(0, "");

    return NL_OK;
  }
};

Netlink::Netlink()
    : logger(spdlog::get("polycubed")),
      notification_(new NetlinkNotification(this)) {}

Netlink::~Netlink() {
  // nl_socket_free(sock_);
}

void Netlink::attach_to_tc(const std::string &iface, int fd, ATTACH_MODE mode) {
  int err;

  struct rtnl_link *link;
  struct nl_cache *cache;
  struct nl_sock *sock;

  uint32_t prio, protocol;

  sock = nl_socket_alloc();
  if ((err = nl_connect(sock, NETLINK_ROUTE)) < 0) {
    logger->error("unable to connect socket: {0}", nl_geterror(err));
    throw std::runtime_error(std::string("Unable to connect socket: ") +
                             nl_geterror(err));
  }

  if ((err = rtnl_link_alloc_cache(sock, AF_UNSPEC, &cache)) < 0) {
    logger->error("unable to allocate cache: {0}", nl_geterror(err));
    throw std::runtime_error(std::string("Unable to allocate cache: ") +
                             nl_geterror(err));
  }

  if (!(link = rtnl_link_get_by_name(cache, iface.c_str()))) {
    logger->error("unable get link");
    throw std::runtime_error("Unable get link");
  }

  // add ingress qdisc to the interface
  struct rtnl_qdisc *qdisc;
  if (!(qdisc = rtnl_qdisc_alloc())) {
    logger->error("unable to allocate qdisc");
    throw std::runtime_error("Unable to allocate qdisc");
  }

  rtnl_tc_set_link(TC_CAST(qdisc), link);
  rtnl_tc_set_parent(TC_CAST(qdisc), TC_HANDLE(0xFFFF, 0xFFF1));
  rtnl_tc_set_handle(TC_CAST(qdisc), TC_HANDLE(0xFFFF, 0));
  rtnl_tc_set_kind(TC_CAST(qdisc), "clsact");

  err = rtnl_qdisc_add(sock, qdisc, NLM_F_CREATE);

  rtnl_qdisc_put(qdisc);

  if (err < 0) {
    logger->error("unable to add qdisc");
    throw std::runtime_error("Unable to add qdisc");
  }

  // add filter to the interface
  struct tcmsg t;

  t.tcm_family = AF_UNSPEC;
  t.tcm_ifindex = rtnl_link_get_ifindex(link);

  t.tcm_handle = TC_HANDLE(0, 0);

  switch (mode) {
  case ATTACH_MODE::EGRESS:
    t.tcm_parent = TC_H_MAKE(TC_H_INGRESS, 0xFFF3U);  // why that number?
    break;
  case ATTACH_MODE::INGRESS:
    t.tcm_parent = TC_H_MAKE(TC_H_INGRESS, 0xFFF2U);  // why that number?
    break;
  }

  protocol = htons(ETH_P_ALL);
  prio = 0;
  t.tcm_info = TC_H_MAKE(prio << 16, protocol);

  struct nl_msg *msg;
  struct nlmsghdr *hdr;

  struct nlattr *opts;

  if (!(msg = nlmsg_alloc())) {
    logger->error("unable allocate nlmsg");
    throw std::runtime_error("Unable allocate nlmsg");
  }

  hdr = nlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ, RTM_NEWTFILTER, sizeof(t),
                  NLM_F_REQUEST | NLM_F_EXCL | NLM_F_CREATE);
  memcpy(nlmsg_data(hdr), &t, sizeof(t));

  NLA_PUT_STRING(msg, TCA_KIND, "bpf");

  if (!(opts = nla_nest_start(msg, TCA_OPTIONS))) {
    nlmsg_free(msg);
    nl_cache_free(cache);
    nl_socket_free(sock);
    goto nla_put_failure;
  }

  NLA_PUT_U32(msg, TCA_BPF_FD, fd);
  NLA_PUT_STRING(msg, TCA_BPF_NAME, iface.c_str());
  NLA_PUT_U32(msg, TCA_BPF_FLAGS, TCA_BPF_FLAG_ACT_DIRECT);

  nla_nest_end(msg, opts);

  nl_send_auto(sock, msg);

  // TODO: read response from the kernel
  nl_cache_free(cache);
  nl_socket_free(sock);
  return;

nla_put_failure:
  logger->error("error constructing nlmsg");
  throw std::runtime_error("Error constructing nlmsg");
}

void Netlink::detach_from_tc(const std::string &iface, ATTACH_MODE mode) {
  int err;

  struct rtnl_link *link;
  struct nl_cache *cache;
  struct nl_sock *sock;

  uint32_t protocol;
  uint32_t prio;

  sock = nl_socket_alloc();
  if ((err = nl_connect(sock, NETLINK_ROUTE)) < 0) {
    logger->error("unable to connect socket: {0}", nl_geterror(err));
    throw std::runtime_error(std::string("Unable to connect socket: ") +
                             nl_geterror(err));
  }

  if ((err = rtnl_link_alloc_cache(sock, AF_UNSPEC, &cache)) < 0) {
    logger->error("unable to allocate cache: {0}", nl_geterror(err));
    throw std::runtime_error(std::string("Unable to allocate cache: ") +
                             nl_geterror(err));
  }

  // it is not that bad, probably the link was removed before, so no problem.
  if (!(link = rtnl_link_get_by_name(cache, iface.c_str()))) {
    logger->debug("detach_from_tc: port {0} does not exist", iface);
    goto error;
  }

  // remove filter from the interface
  struct tcmsg t;

  t.tcm_family = AF_UNSPEC;
  t.tcm_ifindex = rtnl_link_get_ifindex(link);

  t.tcm_handle = TC_HANDLE(0, 0);

  switch (mode) {
  case ATTACH_MODE::EGRESS:
    t.tcm_parent = TC_H_MAKE(TC_H_INGRESS, 0xFFF3U);  // why that number?
    break;
  case ATTACH_MODE::INGRESS:
    t.tcm_parent = TC_H_MAKE(TC_H_INGRESS, 0xFFF2U);  // why that number?
    break;
  }

  protocol = htons(ETH_P_ALL);
  prio = 0;
  t.tcm_info = 0;

  struct nl_msg *msg;
  struct nlmsghdr *hdr;

  struct nlattr *opts;

  if (!(msg = nlmsg_alloc())) {
    logger->error("unable allocate nlmsg");
    throw std::runtime_error("Unable allocate nlmsg");
  }

  hdr = nlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ, RTM_DELTFILTER, sizeof(t),
                  NLM_F_REQUEST);
  hdr->nlmsg_pid = 0;
  memcpy(nlmsg_data(hdr), &t, sizeof(t));

  nl_send_auto(sock, msg);

  // TODO: read response from the kernel
  nl_cache_free(cache);
  nl_socket_free(sock);
  return;

error:
  nl_cache_free(cache);
  nl_socket_free(sock);
}

int Netlink::get_iface_index(const std::string &iface) {
  int err, ifindex;
  struct rtnl_link *link;
  struct nl_cache *cache;
  struct nl_sock *sock;

  sock = nl_socket_alloc();
  if ((err = nl_connect(sock, NETLINK_ROUTE)) < 0) {
    logger->error("unable to connect socket: {0}", nl_geterror(err));
    throw std::runtime_error(std::string("Unable to connect socket: ") +
                             nl_geterror(err));
  }

  if ((err = rtnl_link_alloc_cache(sock, AF_UNSPEC, &cache)) < 0) {
    logger->error("unable to allocate cache: {0}", nl_geterror(err));
    throw std::runtime_error(std::string("Unable to allocate cache: ") +
                             nl_geterror(err));
  }

  if (!(link = rtnl_link_get_by_name(cache, iface.c_str()))) {
    std::cout << " error" << std::endl;
    return -1;
  }

  ifindex = rtnl_link_get_ifindex(link);

  nl_cache_free(cache);
  nl_socket_free(sock);
  return ifindex;
}

struct cb_data {
  std::map<std::string, ExtIfaceInfo> *ifaces;
  struct nl_cache *addr_cache;
  struct rtnl_link *link;
};

static void addr_cb(struct nl_object *o, void *data_) {
  auto logger = spdlog::get("polycubed");

  struct rtnl_addr *addr = (rtnl_addr *)o;
  if (addr == NULL) {
    logger->debug("addr is NULL %d\n", errno);
    return;
  }

  struct cb_data *data = (struct cb_data *)data_;
  if (data == NULL) {
    logger->debug("ifaces is NULL %d");
    return;
  }

  int cur_ifindex = rtnl_addr_get_ifindex(addr);
  int req_ifindex = rtnl_link_get_ifindex(data->link);
  if (cur_ifindex != req_ifindex)
    return;

  const struct nl_addr *local = rtnl_addr_get_local(addr);
  if (local == NULL) {
    logger->debug("rtnl_addr_get failed\n");
    return;
  }

  char addr_str[1000];
  const char *addr_s = nl_addr2str(local, addr_str, sizeof(addr_str));
  if (addr_s == NULL) {
    logger->debug("nl_addr2str failed\n");
    return;
  }

  std::string name(rtnl_link_get_name(data->link));
  data->ifaces->at(name).add_address(addr_s);
}

static void link_cb(struct nl_object *o, void *data_) {
  auto logger = spdlog::get("polycubed");

  struct rtnl_link *link = (rtnl_link *)o;
  if (link == NULL) {
    logger->debug("link is NULL");
    return;
  }

  unsigned flags = rtnl_link_get_flags(link);

  if (!(flags & IFF_UP) || (flags & IFF_LOOPBACK))
    return;

  struct cb_data *data = (struct cb_data *)data_;
  if (data == NULL) {
    logger->debug("ifaces is NULL %d");
    return;
  }

  std::string name(rtnl_link_get_name(link));

  data->ifaces->insert(
      std::pair<std::string, ExtIfaceInfo>(name, ExtIfaceInfo(name)));

  data->link = link;

  nl_cache_foreach(data->addr_cache, addr_cb, data);
}

std::map<std::string, ExtIfaceInfo> Netlink::get_available_ifaces() {
  std::map<std::string, ExtIfaceInfo> ifaces;

  int err, ifindex;
  struct rtnl_link *link;
  struct nl_cache *link_cache, *addr_cache;
  struct nl_sock *sock;

  sock = nl_socket_alloc();
  if ((err = nl_connect(sock, NETLINK_ROUTE)) < 0) {
    logger->error("unable to connect socket: {0}", nl_geterror(err));
    throw std::runtime_error(std::string("Unable to connect socket: ") +
                             nl_geterror(err));
  }

  if ((err = rtnl_link_alloc_cache(sock, AF_UNSPEC, &link_cache)) < 0) {
    logger->error("unable to allocate cache: {0}", nl_geterror(err));
    throw std::runtime_error(std::string("Unable to allocate cache: ") +
                             nl_geterror(err));
  }

  if ((err = rtnl_addr_alloc_cache(sock, &addr_cache)) < 0) {
    logger->error("unable to allocate cache: {0}", nl_geterror(err));
    throw std::runtime_error(std::string("Unable to allocate cache: ") +
                             nl_geterror(err));
  }

  struct cb_data d {
    .ifaces = &ifaces, .addr_cache = addr_cache,
  };

  nl_cache_foreach(link_cache, link_cb, &d);

  nl_cache_free(addr_cache);
  nl_cache_free(link_cache);
  nl_socket_free(sock);
  return ifaces;
}

void Netlink::notify_link_deleted(int ifindex, const std::string &iface) {
  notify(Netlink::Event::LINK_DELETED, ifindex, iface);
}

void Netlink::notify_link_added(int ifindex, const std::string &iface) {
  notify(Netlink::Event::LINK_ADDED, ifindex, iface);
}

void Netlink::notify_route_added(int ifindex, const std::string &info_route) {
  notify(Netlink::Event::ROUTE_ADDED, ifindex, info_route);
}

void Netlink::notify_route_deleted(int ifindex, const std::string &info_route) {
  notify(Netlink::Event::ROUTE_DELETED, ifindex, info_route);
}

void Netlink::notify_new_address(int ifindex, const std::string &info_address) {
  notify(Netlink::Event::NEW_ADDRESS, ifindex, info_address);
}

void Netlink::notify_all(int ifindex, const std::string &iface) {
  notify(Netlink::Event::ALL, ifindex, iface);
}

void Netlink::attach_to_xdp(const std::string &iface, int fd,
                            int attach_flags) {
  logger->debug("attaching XDP program to iface {0}", iface);

  if (bpf_attach_xdp(iface.c_str(), fd, attach_flags) < 0) {
    logger->error("failed to attach XDP program to port: {0}", iface);
    throw BPFError("Failed to attach XDP program to port: " + iface);
  }

  logger->debug("XDP program attached to port: {0}", iface);
}

void Netlink::detach_from_xdp(const std::string &iface, int attach_flags) {
  logger->debug("detaching XDP program from port {0}", iface);

  // it is not that bad, probably the link was removed before, so no problem.
  if (get_iface_index(iface) == -1) {
    logger->debug("detach_from_xdp: port {0} does not exist", iface);
    return;
  }

  if (bpf_attach_xdp(iface.c_str(), -1, attach_flags) < 0) {
    logger->error("failed to detach XDP program from port: {0}", iface);
    throw BPFError("Failed to detach XDP program from port: " + iface);
  }

  logger->debug("XDP program detached from port: {0}", iface);
}

}  // namespace polycubed
}  // namespace polycube
