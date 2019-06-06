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

#pragma once

#include "../base/StpBase.h"
#include "ext.h"
#include "stp/stp.h"

#include <tins/dot1q.h>
#include <tins/ethernetII.h>
#include <tins/tins.h>

class Bridge;

using namespace polycube::service::model;

class Stp : public StpBase {
 public:
  Stp(Bridge &parent, const StpJsonObject &conf);
  virtual ~Stp();

  /// <summary>
  /// VLAN identifier for this entry
  /// </summary>
  uint16_t getVlan() override;

  /// <summary>
  /// Bridge priority for STP
  /// </summary>
  uint16_t getPriority() override;
  void setPriority(const uint16_t &value) override;

  /// <summary>
  /// Delay used by STP bridges for port state transition
  /// </summary>
  uint32_t getForwardDelay() override;
  void setForwardDelay(const uint32_t &value) override;

  /// <summary>
  /// Interval between transmissions of BPDU messages
  /// </summary>
  uint32_t getHelloTime() override;
  void setHelloTime(const uint32_t &value) override;

  /// <summary>
  /// Maximum age of a BPDU
  /// </summary>
  uint32_t getMaxMessageAge() override;
  void setMaxMessageAge(const uint32_t &value) override;

  static void sendBPDUproxy(struct ofpbuf *bpdu, int port_no, void *aux);
  static uint64_t reverseMac(uint64_t mac);

  void processBPDU(int port_id, const std::vector<uint8_t> &packet);
  struct stp *getStpInstance();
  void incrementCounter();
  void decrementCounter();
  void changeMac(const std::string &mac);
  bool portShouldForward(int port_id);
  void updateParameters(const StpJsonObject &conf);

 private:
  void stpTickFunction();
  void sendBPDU(struct ofpbuf *bpdu, int port_no, void *aux);
  void updateFwdTable();
  void updatePortsState();

  uint16_t vlan_;
  uint16_t priority_ = 32768;
  uint32_t forward_delay_ = 10;
  uint32_t hello_time_ = 2;
  uint32_t max_message_age_ = 20;
  int counter_ = 0;

  std::atomic<bool> stop_;
  std::thread stp_tick_thread_;
  std::mutex stp_mutex_;
  struct stp *stp_;
};
