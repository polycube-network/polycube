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


#include "../base/ClassifierBase.h"
#include "TrafficClass.h"

#include <linux/if_ether.h>
#include <linux/in.h>


#define MAX_TRAFFIC_CLASSES 100000
#define FIELDS_COUNT 8

#define FIELD_INDEX(field) (static_cast<int>(field))
#define FIELD_NAME(field)  (kFieldData[FIELD_INDEX(field)].name)
#define FIELD_TYPE(field)  (kFieldData[FIELD_INDEX(field)].type)


using namespace polycube::service;
using namespace polycube::service::utils;


enum MatchingField {
  SMAC,
  DMAC,
  ETHTYPE,
  SRCIP,
  DSTIP,
  L4PROTO,
  SPORT,
  DPORT
};

struct FieldData {
  std::string name;
  std::string type;
};

// Used to map MatchingField to field name and type
const struct FieldData kFieldData[] {
  {"smac",    "__be64"},
  {"dmac",    "__be64"},
  {"ethtype", "__be16"},
  {"srcip",   "__be32"},
  {"dstip",   "__be32"},
  {"l4proto", "u8"},
  {"sport",   "__be16"},
  {"dport",   "__be16"}
};

// Used to map TrafficClassEthtypeEnum to ethertype code
const uint16_t kEthtypes[] = {ETH_P_ARP, ETH_P_IP};

// Used to map TrafficClassL4protoEnum to L4 protocol code
const uint8_t kL4protos[] = {IPPROTO_ICMP, IPPROTO_TCP, IPPROTO_UDP};

class MatcherInterface;

using namespace polycube::service::model;


class Classifier : public ClassifierBase {
 public:
  Classifier(const std::string name,
                    const ClassifierJsonObject &conf);
  virtual ~Classifier();

  void packet_in(polycube::service::Direction direction,
                 polycube::service::PacketInMetadata &md,
                 const std::vector<uint8_t> &packet) override;

  /// <summary>
  /// Traffic class identified by id
  /// </summary>
  std::shared_ptr<TrafficClass> getTrafficClass(const uint32_t &id) override;
  std::vector<std::shared_ptr<TrafficClass>> getTrafficClassList() override;
  void addTrafficClass(const uint32_t &id,
                       const TrafficClassJsonObject &conf) override;
  void addTrafficClassList(
      const std::vector<TrafficClassJsonObject> &conf) override;
  void replaceTrafficClass(const uint32_t &id,
                           const TrafficClassJsonObject &conf) override;
  void delTrafficClass(const uint32_t &id) override;
  void delTrafficClassList() override;

  void updateProgram(TrafficClassDirectionEnum direction);
  void updateProgram(ProgramType direction);

 private:
  std::unordered_map<uint32_t, std::shared_ptr<TrafficClass>> traffic_classes_;
  std::array<std::shared_ptr<MatcherInterface>, FIELDS_COUNT> matchers_;
  std::unordered_map<ProgramType, bool> classification_enabled_;
  std::unordered_map<ProgramType, int> active_program_;
};