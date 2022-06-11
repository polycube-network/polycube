/*
 * Copyright 2021 Leonardo Di Giovanna
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


#include "../base/SrcIpRewriteBase.h"


class K8slbrp;

using namespace polycube::service::model;

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

class SrcIpRewrite : public SrcIpRewriteBase {
 public:
    friend class K8slbrp;
  SrcIpRewrite(K8slbrp &parent, const SrcIpRewriteJsonObject &conf);
  virtual ~SrcIpRewrite();

    void update(const SrcIpRewriteJsonObject &conf) override;

  /// <summary>
  /// Range of IP addresses of the clients that must be replaced
  /// </summary>
  std::string getIpRange() override;
  void setIpRange(const std::string &value) override;

  /// <summary>
  /// Range of IP addresses of the that must be used to replace client addresses
  /// </summary>
  std::string getNewIpRange() override;
  void setNewIpRange(const std::string &value) override;

private:
    std::string new_ip_range;
    std::string ip_range;
    uint32_t net;
    uint32_t mask;
};
