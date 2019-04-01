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

#include "../interface/SrcIpRewriteInterface.h"

#include <spdlog/spdlog.h>
#include <stdio.h>
#include <stdlib.h>

class Lbrp;

using namespace io::swagger::server::model;

/* definitions copied from datapath */
struct src_ip_r_key {
  uint32_t netmask_len;
  uint32_t network;
} __attribute__((packed));

struct src_ip_r_value {
  uint32_t sense;
  uint32_t net;   // host network byte order
  uint32_t mask;  // host network byte order
} __attribute__((packed));

class SrcIpRewrite : public SrcIpRewriteInterface {
 public:
  friend class Lbrp;
  SrcIpRewrite(Lbrp &parent, const SrcIpRewriteJsonObject &conf);
  virtual ~SrcIpRewrite();

  static void create(Lbrp &parent, const SrcIpRewriteJsonObject &conf);
  static std::shared_ptr<SrcIpRewrite> getEntry(Lbrp &parent);
  static void removeEntry(Lbrp &parent);
  std::shared_ptr<spdlog::logger> logger();
  void update(const SrcIpRewriteJsonObject &conf) override;
  SrcIpRewriteJsonObject toJsonObject() override;

  /// <summary>
  /// Range of IP addresses of the that must be used to replace client addresses
  /// </summary>
  std::string getNewIpRange() override;
  void setNewIpRange(const std::string &value) override;

  /// <summary>
  /// Range of IP addresses of the clients that must be replaced
  /// </summary>
  std::string getIpRange() override;
  void setIpRange(const std::string &value) override;

 private:
  Lbrp &parent_;
  std::string new_ip_range;
  std::string ip_range;
  uint32_t net;
  uint32_t mask;
};
