/*
 * Copyright 2020 The Polycube Authors
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


#include "../base/TrafficClassBase.h"


class Classifier;


using namespace polycube::service::model;

class TrafficClass : public TrafficClassBase {
 public:
  TrafficClass(Classifier &parent, const TrafficClassJsonObject &conf);
  virtual ~TrafficClass();

  TrafficClassJsonObject toJsonObject() override;

  /// <summary>
  /// Id of the class, set in metadata of matching packets
  /// </summary>
  uint32_t getId() override;

  /// <summary>
  /// Packets matching multiple classes are assigned to the one with highest priority
  /// </summary>
  uint32_t getPriority() override;
  void setPriority(const uint32_t &value) override;

  /// <summary>
  /// Direction (INGRESS, EGRESS or BOTH) of the packet (default: BOTH)
  /// </summary>
  TrafficClassDirectionEnum getDirection() override;
  void setDirection(const TrafficClassDirectionEnum &value) override;

  /// <summary>
  /// Source MAC address of the packet
  /// </summary>
  std::string getSmac() override;
  void setSmac(const std::string &value) override;
  bool smacIsSet();

  /// <summary>
  /// Destination MAC address of the packet
  /// </summary>
  std::string getDmac() override;
  void setDmac(const std::string &value) override;
  bool dmacIsSet();

  /// <summary>
  /// Ethertype of the packet (ARP | IP)
  /// </summary>
  TrafficClassEthtypeEnum getEthtype() override;
  void setEthtype(const TrafficClassEthtypeEnum &value) override;
  bool ethtypeIsSet();

  /// <summary>
  /// Source IP address prefix of the packet
  /// </summary>
  std::string getSrcip() override;
  void setSrcip(const std::string &value) override;
  bool srcipIsSet();

  /// <summary>
  /// Destination IP address prefix of the packet
  /// </summary>
  std::string getDstip() override;
  void setDstip(const std::string &value) override;
  bool dstipIsSet();

  /// <summary>
  /// Level 4 protocol of the packet (ICMP | TCP | UDP)
  /// </summary>
  TrafficClassL4protoEnum getL4proto() override;
  void setL4proto(const TrafficClassL4protoEnum &value) override;
  bool l4protoIsSet();

  /// <summary>
  /// Source port of the packet
  /// </summary>
  uint16_t getSport() override;
  void setSport(const uint16_t &value) override;
  bool sportIsSet();

  /// <summary>
  /// Destination port of the packet
  /// </summary>
  uint16_t getDport() override;
  void setDport(const uint16_t &value) override;
  bool dportIsSet();

  bool operator<=(const TrafficClass& other);
  std::string toString();

 private:
  uint32_t id_;
  uint32_t priority_;
  TrafficClassDirectionEnum direction_;
  std::string smac_;
  bool smac_set_;
  std::string dmac_;
  bool dmac_set_;
  TrafficClassEthtypeEnum ethtype_;
  bool ethtype_set_;
  std::string srcip_;
  bool srcip_set_;
  std::string dstip_;
  bool dstip_set_;
  TrafficClassL4protoEnum l4proto_;
  bool l4proto_set_;
  uint16_t sport_;
  bool sport_set_;
  uint16_t dport_;
  bool dport_set_;

  bool isValid();
};
