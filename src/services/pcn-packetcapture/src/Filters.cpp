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


#include "Filters.h"
#include "Packetcapture.h"
#include "Packetcapture_dp_ingress.h"
#include "Packetcapture_dp_egress.h"



Filters::Filters(Packetcapture &parent, const FiltersJsonObject &conf)
    : FiltersBase(parent), set_dstIp(false), set_srcIp(false), set_dstPort(false), set_srcPort(false), set_l4proto(false) {

  if (conf.snaplenIsSet()) {
    setSnaplen(conf.getSnaplen());
  }

  if (conf.srcIsSet()) {
    setSrc(conf.getSrc());
  }

  if (conf.dstIsSet()) {
    setDst(conf.getDst());
  }

  if (conf.l4protoIsSet()) {
    setL4proto(conf.getL4proto());
  }

  if (conf.sportIsSet()) {
    setSport(conf.getSport());
  }

  if (conf.dportIsSet()) {
    setDport(conf.getDport());
  }
  bootstrap = false;
  snaplen = 262144;
}

Filters::~Filters() {}

uint32_t Filters::getSnaplen() {
  return snaplen;
}

void Filters::setSnaplen(const uint32_t &value) {
  snaplen = value;
  set_snaplen = true;
  if (!bootstrap)
    parent_.updateFiltersMaps();
}

std::string Filters::getSrc() {
  return srcIp;
}

void Filters::setSrc(const std::string &value) {
  srcIp = value;

  uint32_t ip_src_filter = 0;
  netmaskSrc = (0xFFFFFFFF << (32 - std::stoi(value.substr(value.find("/")+1)))) & 0xFFFFFFFF;
  std::string source_ip = value.substr(0, value.find("/"));
  inet_pton(AF_INET, source_ip.data(), &ip_src_filter);
  ip_src_filter = ntohl(ip_src_filter);
  networkSrc = ip_src_filter & netmaskSrc;

  set_srcIp = true;
  if (!bootstrap)
    parent_.updateFiltersMaps();
}

std::string Filters::getDst() {
  return dstIp;
}

void Filters::setDst(const std::string &value) {
  dstIp = value;

  uint32_t ip_dst_filter = 0;
  netmaskDst = (0xFFFFFFFF << (32 - std::stoi(value.substr(value.find("/")+1)))) & 0xFFFFFFFF;
  std::string source_ip = value.substr(0, value.find("/"));
  inet_pton(AF_INET, source_ip.data(), &ip_dst_filter);
  ip_dst_filter = ntohl(ip_dst_filter);
  networkDst = ip_dst_filter & netmaskDst;

  set_dstIp = true;
  if (!bootstrap)
    parent_.updateFiltersMaps();
}

std::string Filters::getL4proto() {
  return l4proto;
}

void Filters::setL4proto(const std::string &value) {
  if ((value.compare(std::string("tcp")) == 0) || (value.compare(std::string("udp")) == 0)){
    l4proto = value;
    set_l4proto = true;
    if (!bootstrap)
      parent_.updateFiltersMaps();
  } else {
    throw std::runtime_error("Bad value at setL4proto. Please enter 'tcp' or 'udp'");
  }
}

uint16_t Filters::getSport() {
  return this->srcPort;
}

void Filters::setSport(const uint16_t &value) {
  srcPort = value;
  set_srcPort = true;
  if (!bootstrap)
    parent_.updateFiltersMaps();
}

uint16_t Filters::getDport() {
  return dstPort;
}

void Filters::setDport(const uint16_t &value) {
  dstPort = value;
  set_dstPort = true;
  if (!bootstrap)
    parent_.updateFiltersMaps();
}

uint32_t Filters::getNetworkFilterSrc() {
  return networkSrc;
}

uint32_t Filters::getNetworkFilterDst() {
  return networkDst;
}

uint32_t Filters::getNetmaskFilterSrc() {
  return netmaskSrc;
}

uint32_t Filters::getNetmaskFilterDst() {
  return netmaskDst;
}
