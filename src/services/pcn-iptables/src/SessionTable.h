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

#pragma once

#include "../interface/SessionTableInterface.h"

#include <spdlog/spdlog.h>

enum {
  NEW,
  ESTABLISHED,
  RELATED,
  INVALID,
  SYN_SENT,
  SYN_RECV,
  FIN_WAIT_1,
  FIN_WAIT_2,
  LAST_ACK,
  TIME_WAIT
};

class Iptables;

using namespace io::swagger::server::model;

class SessionTable : public SessionTableInterface {
 public:
  SessionTable(Iptables &parent, const SessionTableJsonObject &conf);
  virtual ~SessionTable();

  std::shared_ptr<spdlog::logger> logger();
  void update(const SessionTableJsonObject &conf) override;
  SessionTableJsonObject toJsonObject() override;

  /// <summary>
  /// Source IP
  /// </summary>
  std::string getSrc() override;

  /// <summary>
  /// Destination IP
  /// </summary>
  std::string getDst() override;

  /// <summary>
  /// Level 4 Protocol.
  /// </summary>
  std::string getL4proto() override;

  /// <summary>
  /// Source Port
  /// </summary>
  uint16_t getSport() override;

  /// <summary>
  /// Destination
  /// </summary>
  uint16_t getDport() override;

  /// <summary>
  /// Entry direction (forward/reverse)
  /// </summary>
  std::string getDirection() override;

  /// <summary>
  /// Session ID
  /// </summary>
  uint32_t getId() override;

  /// <summary>
  /// Connection state
  /// </summary>
  std::string getState() override;

  /// <summary>
  /// Expire TTL
  /// </summary>
  uint64_t getTtl() override;

  /// <summary>
  /// Sequence
  /// </summary>
  uint32_t getSequence() override;

  /// <summary>
  /// Dnat new ip
  /// </summary>
  std::string getDnatip() override;

  /// <summary>
  /// Dnat new port
  /// </summary>
  uint16_t getDnatport() override;

  /// <summary>
  /// Snat new ip
  /// </summary>
  std::string getSnatip() override;

  /// <summary>
  /// Snat new port
  /// </summary>
  uint16_t getSnatport() override;

  static std::string stateFromNumberToString(int state);

 private:
  Iptables &parent_;
  SessionTableJsonObject fields;
};
