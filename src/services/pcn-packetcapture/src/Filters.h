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


#include "../base/FiltersBase.h"


 struct filters_table {
  bool network_filter_src_flag;
  uint32_t network_filter_src;
  uint32_t netmask_filter_src;
  bool network_filter_dst_flag;
  uint32_t network_filter_dst;
  uint32_t netmask_filter_dst;
  bool src_port_flag;
  uint16_t src_port_filter;
  bool dst_port_flag;
  uint16_t dst_port_filter;
  bool l4proto_flag;
  int l4proto_filter;   /* if l4proto_filter = 1 is TCP only else if l4proto_filter = 2 is UDP only */
  uint32_t snaplen;
};

class Packetcapture;

using namespace polycube::service::model;

class Filters : public FiltersBase {


  bool set_srcIp, set_dstIp, set_srcPort, set_dstPort, set_l4proto, set_snaplen;

  bool bootstrap = true;
  std::string srcIp = "0.0.0.0/24";
  std::string dstIp = "0.0.0.0/24";
  uint32_t networkSrc = 0;
  uint32_t networkDst = 0;
  uint32_t netmaskSrc = 0;
  uint32_t netmaskDst = 0;
  uint16_t srcPort = 0;
  uint16_t dstPort = 0;
  std::string l4proto = "";
  uint32_t snaplen = 262144;   /* 65535 for no sliced packets */

 public:
  Filters(Packetcapture &parent, const FiltersJsonObject &conf);
  virtual ~Filters();

  /// <summary>
  /// Snapshot length
  /// </summary>
  uint32_t getSnaplen() override;
  void setSnaplen(const uint32_t &value) override;

  /// <summary>
  /// IP source filter
  /// </summary>
  std::string getSrc() override;
  void setSrc(const std::string &value) override;
  bool srcIp_is_set(){ return set_srcIp; };

  /// <summary>
  /// IP destination filter
  /// </summary>
  std::string getDst() override;
  void setDst(const std::string &value) override;
  bool dstIp_is_set(){ return set_dstIp; };

  /// <summary>
  /// Level 4 protocol filter
  /// </summary>
  std::string getL4proto() override;
  void setL4proto(const std::string &value) override;
  bool l4proto_is_set(){ return set_l4proto; };

  /// <summary>
  /// Source port filter
  /// </summary>
  uint16_t getSport() override;
  void setSport(const uint16_t &value) override;
  bool srcPort_is_set(){ return set_srcPort; };

  /// <summary>
  /// Destination port filter
  /// </summary>
  uint16_t getDport() override;
  void setDport(const uint16_t &value) override;
  bool dstPort_is_set(){ return set_dstPort; };

  uint32_t getNetworkFilterSrc();
  uint32_t getNetworkFilterDst();
  uint32_t getNetmaskFilterSrc();
  uint32_t getNetmaskFilterDst();
};
