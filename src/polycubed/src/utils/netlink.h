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

#pragma once

#include <linux/if_ether.h>
#include <functional>
#include <map>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "extiface_info.h"

#include <netlink/netlink.h>
#include <netlink/route/addr.h>
#include <netlink/route/link.h>
#include <netlink/route/qdisc.h>
#include <netlink/socket.h>

#include <spdlog/spdlog.h>

namespace polycube {
namespace polycubed {

enum class ATTACH_MODE {
  INGRESS,
  EGRESS,
};

enum class IFACE_STATUS {
  UP,
  DOWN,
};

class Observer;  // a passible subscriber callback class

class Netlink {
 public:
  ~Netlink();

  // TODO: Add here new events needed for the future
  enum Event { LINK_ADDED, LINK_DELETED, ROUTE_ADDED, ROUTE_DELETED, NEW_ADDRESS, ALL };
  // enum Event { LINK_DELETED };
  static Netlink &getInstance() {
    static Netlink instance;
    return instance;
  }
  Netlink(Netlink const &) = delete;
  void operator=(Netlink const &) = delete;

  void attach_to_tc(const std::string &iface, int fd,
                    ATTACH_MODE mode = ATTACH_MODE::INGRESS);
  void detach_from_tc(const std::string &iface,
                      ATTACH_MODE mode = ATTACH_MODE::INGRESS);

  void attach_to_xdp(const std::string &iface, int fd, int attach_flags);
  void detach_from_xdp(const std::string &iface, int attach_flags);

  int get_iface_index(const std::string &iface);

  std::map<std::string, ExtIfaceInfo> get_available_ifaces();

  void set_iface_status(const std::string &iface, IFACE_STATUS status);
  void set_iface_mac(const std::string &iface, const std::string &mac);
  std::string get_iface_mac(const std::string &iface);
  void add_iface_ip(const std::string &iface, const std::string &ip, int prefix);
  void add_iface_ipv6(const std::string &iface, const std::string &ip);
  std::string get_iface_ip(const std::string &iface);
  std::string get_iface_netmask(const std::string &iface);
  void delete_iface_ip(const std::string &iface, const std::string &ip, int prefix);
  void move_iface_into_ns(const std::string &iface, int fd);

  // Remove old ip address and add new ip address
  void set_iface_cidr(const std::string &iface, const std::string &cidr);

  template <typename Observer>
  int registerObserver(const Event &event, Observer &&observer) {
    std::lock_guard<std::mutex> lock(notify_mutex);
    // auto it = observers_[event].insert(observers_.end(),
    // std::forward<Observer>(observer));
    int random_key = uniform_distribution_(engine_);
    observers_[event][random_key] = std::forward<Observer>(observer);
    return random_key;
  }

  void unregisterObserver(const Event &event, int observer_index) {
    std::lock_guard<std::mutex> lock(notify_mutex);
    if (observers_.count(event) > 0 &&
        observers_[event].count(observer_index) > 0) {
      observers_[event].erase(observer_index);
    }
  }

  void notify(const Event &event, int ifindex, const std::string &ifname) {
    std::map<int, std::function<void(int, const std::string &)>> ob_copy;
    {
      std::lock_guard<std::mutex> lock(notify_mutex);
      ob_copy.insert(observers_[event].begin(), observers_[event].end());
    }

    for (const auto &it : ob_copy) {
      auto obs = it.second;
      obs(ifindex, ifname);
    }
  }

 private:
  Netlink();
  void notify_link_deleted(int ifindex, const std::string &iface);
  void notify_link_added(int ifindex, const std::string &iface);
  void notify_route_added(int ifindex, const std::string &info_route);
  void notify_route_deleted(int ifindex, const std::string &info_route);
  void notify_new_address(int ifindex, const std::string &info_address);
  void notify_all(int ifindex, const std::string &iface);

  struct nlmsghdr* netlink_alloc();
  struct nlmsghdr* netlink_ip_alloc();
  struct nlmsghdr* netlink_ip_dealloc();
  struct nlmsghdr* netlink_ipv6_alloc();
  int netlink_nl_send(struct nlmsghdr *nlmsg);

  std::shared_ptr<spdlog::logger> logger;
  std::mutex notify_mutex;

  class NetlinkNotification;
  std::unique_ptr<NetlinkNotification> notification_;
  std::map<Event, std::map<int, std::function<void(int, const std::string &)>>>
      observers_;
  std::uniform_int_distribution<int> uniform_distribution_;
  std::mt19937 engine_;  // Mersenne twister MT19937
};

}  // namespace polycubed
}  // namespace polycube
