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

#include "stp/stp.h"
#include <mutex>
#include <random>
#include <set>
#include <thread>
#include <vector>
#include <algorithm>

#include "polycube/services/cube.h"
#include "polycube/services/port.h"
#include "polycube/services/utils.h"

#include <tins/tins.h>
#include <tins/ethernetII.h>
#include <tins/dot1q.h>

#include <spdlog/spdlog.h>

using namespace polycube::service;
using namespace Tins;

class Bridge;

class BridgeSTP {
 public:
  BridgeSTP(Bridge &parent, uint16_t vlan_id);
  ~BridgeSTP();
  void add_port(int port_id);
  void remove_port(int port_id);
  int ports_count();

  void process_bpdu(int port_id, const std::vector<uint8_t> &packet);
  void send_bpdu(struct ofpbuf *bpdu, int port_no, void *aux);
  void update_ports_state();
  void update_fwd_table();

  bool port_should_forward(int port_id); // should this port forward packets for this vlan?

  /* STP management functions */
  void set_priority(uint16_t priority);
  void set_hello_time(int hello_time);
  void set_max_age(int max_age);
  void set_forward_delay(int forward_delay);
  void set_address(const std::string &address);

  uint16_t get_priority() const;
  std::string get_address() const;
  std::string get_designated_root() const;
  bool is_root_bridge() const;
  int get_root_path_cost() const ;
  int get_hello_time() const;
  int get_max_age() const;
  int get_forward_delay() const;
  bool check_and_reset_fdb_flush() const;

  /* STP ports related funtions */
  enum stp_state get_port_state(int port_id);
  std::string get_port_state_str(int port_id);
  enum stp_role get_port_role(int port_id);
  void set_port_priority(int port_id, uint8_t new_priority);
  void set_port_path_cost(int port_id, uint16_t path_cost);
  void set_port_speed(int port_id, unsigned int speed);
  void enable_port_change_detection(int port_id);
  void disable_port_change_detection(int port_id);

 private:
  void stp_tick_function();
  static void send_bpdu_proxy(struct ofpbuf *bpdu, int port_no, void *aux);

  std::string mac_to_string(const uint8_t *mac) const;

  struct stp *stp;
  std::mutex stp_mutex;
  uint16_t vlan_id;
  Bridge &parent;
  uint8_t mac[ETH_ADDR_LEN];
  bool stop_;
  std::unique_ptr<std::thread> stp_tick_thread;
  std::shared_ptr<spdlog::logger> l;
  std::set<int> ports_;
};
