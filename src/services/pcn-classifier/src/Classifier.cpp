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


#include "Classifier.h"
#include "../matcher/ExactMatcher.h"
#include "../matcher/PrefixMatcher.h"
#include "Classifier_dp.h"
#include "Selector_dp.h"
#include "Utils.h"


Classifier::Classifier(const std::string name,
                                     const ClassifierJsonObject &conf)
    : TransparentCube(conf.getBase(), {}, {}),
      ClassifierBase(name) {
  logger()->info("Creating Classifier instance");

  matchers_[FIELD_INDEX(SMAC)] = std::dynamic_pointer_cast<MatcherInterface>(
      std::make_shared<ExactMatcher<uint64_t>>(*this, SMAC));

  matchers_[FIELD_INDEX(DMAC)] = std::dynamic_pointer_cast<MatcherInterface>(
      std::make_shared<ExactMatcher<uint64_t>>(*this, DMAC));

  matchers_[FIELD_INDEX(ETHTYPE)] = std::dynamic_pointer_cast<MatcherInterface>(
      std::make_shared<ExactMatcher<uint16_t>>(*this, ETHTYPE));

  matchers_[FIELD_INDEX(SRCIP)] = std::dynamic_pointer_cast<MatcherInterface>(
      std::make_shared<PrefixMatcher<uint32_t>>(*this, SRCIP));

  matchers_[FIELD_INDEX(DSTIP)] = std::dynamic_pointer_cast<MatcherInterface>(
      std::make_shared<PrefixMatcher<uint32_t>>(*this, DSTIP));

  matchers_[FIELD_INDEX(L4PROTO)] = std::dynamic_pointer_cast<MatcherInterface>(
      std::make_shared<ExactMatcher<uint8_t>>(*this, L4PROTO));

  matchers_[FIELD_INDEX(SPORT)] = std::dynamic_pointer_cast<MatcherInterface>(
      std::make_shared<ExactMatcher<uint16_t>>(*this, SPORT));

  matchers_[FIELD_INDEX(DPORT)] = std::dynamic_pointer_cast<MatcherInterface>(
      std::make_shared<ExactMatcher<uint16_t>>(*this, DPORT));

  // Load selector programs with classification disabled
  std::string code = selector_code;
  replaceAll(code, "_CLASSIFY", "0");
  std::string ingress_code = code, egress_code = code;
  replaceAll(ingress_code, "_DIRECTION", "ingress");
  replaceAll(egress_code, "_DIRECTION", "egress");
  add_program(ingress_code, 0, ProgramType::INGRESS);
  add_program(egress_code, 0, ProgramType::EGRESS);

  // Load Index64 table
  const uint16_t index64[64] = {
      0,  47, 1,  56, 48, 27, 2,  60, 57, 49, 41, 37, 28, 16, 3,  61,
      54, 58, 35, 52, 50, 42, 21, 44, 38, 32, 29, 23, 17, 11, 4,  62,
      46, 55, 26, 59, 40, 36, 15, 53, 34, 51, 20, 43, 31, 22, 10, 45,
      25, 39, 14, 33, 19, 30, 9,  24, 13, 18, 8,  12, 7,  6,  5,  63};

  auto table = get_array_table<uint16_t>("index64", 0);

  for (int i = 0; i < 64; i++) {
    table.set(i, index64[i]);
  }

  classification_enabled_[ProgramType::INGRESS] = false;
  classification_enabled_[ProgramType::EGRESS] = false;
  active_program_[ProgramType::INGRESS] = 2;
  active_program_[ProgramType::EGRESS] = 2;

  addTrafficClassList(conf.getTrafficClass());
}

Classifier::~Classifier() {
  logger()->info("Destroying Classifier instance");
}

void Classifier::packet_in(polycube::service::Direction direction,
                           polycube::service::PacketInMetadata &md,
                           const std::vector<uint8_t> &packet) {
  logger()->debug("Packet received");
}

std::shared_ptr<TrafficClass> Classifier::getTrafficClass(
    const uint32_t &id) {
  if (traffic_classes_.count(id) == 0) {
    throw std::runtime_error("No traffic class with the given id");
  }

  return traffic_classes_.at(id);
}

std::vector<std::shared_ptr<TrafficClass>>
Classifier::getTrafficClassList() {
  std::vector<std::shared_ptr<TrafficClass>> traffic_classes_v;

  traffic_classes_v.reserve(traffic_classes_.size());

  for (auto const &entry : traffic_classes_) {
    traffic_classes_v.push_back(entry.second);
  }

  return traffic_classes_v;
}

void Classifier::addTrafficClass(const uint32_t &id,
                                 const TrafficClassJsonObject &conf) {
  if (traffic_classes_.count(id) != 0) {
    throw std::runtime_error("Traffic class with the given id already exists");
  }

  if (traffic_classes_.size() == MAX_TRAFFIC_CLASSES) {
    throw std::runtime_error("Maximum number of traffic classes reached");
  }

  traffic_classes_[id] = std::make_shared<TrafficClass>(*this, conf);

  updateProgram(conf.getDirection());

  logger()->info("Traffic class added {0}", traffic_classes_[id]->toString());
}

void Classifier::addTrafficClassList(
    const std::vector<TrafficClassJsonObject> &conf) {
  if (conf.size() == 0) {
    return;
  }

  if (traffic_classes_.size() + conf.size() > MAX_TRAFFIC_CLASSES) {
    throw std::runtime_error("Maximum number of traffic classes reached");
  }

  for (auto &it : conf) {
    if (traffic_classes_.count(it.getId()) != 0) {
      throw std::runtime_error(std::string("Traffic class with id ") +
                               std::to_string(it.getId()) + " already exists");
    }
  }

  bool update_ingress = false, update_egress = false;
  for (auto &it : conf) {
    traffic_classes_[it.getId()] = std::make_shared<TrafficClass>(*this, it);
    
    switch (it.getDirection()) {
      case TrafficClassDirectionEnum::INGRESS:
        update_ingress = true;
        break;
      
      case TrafficClassDirectionEnum::EGRESS:
        update_egress = true;
        break;

      case TrafficClassDirectionEnum::BOTH:
        update_ingress = true;
        update_egress = true;
        break;
    }
  }

  if (update_ingress) {
    updateProgram(ProgramType::INGRESS);
  }
  if (update_egress) {
    updateProgram(ProgramType::EGRESS);
  }

  logger()->info("Traffic class list added");
}

void Classifier::replaceTrafficClass(
    const uint32_t &id, const TrafficClassJsonObject &conf) {
  if (traffic_classes_.count(id) == 0) {
    throw std::runtime_error("No traffic class with the given id");
  }

  bool update_ingress = false, update_egress = false;
  switch (traffic_classes_[id]->getDirection()) {
    case TrafficClassDirectionEnum::INGRESS:
      update_ingress = true;
      break;
    
    case TrafficClassDirectionEnum::EGRESS:
      update_egress = true;
      break;

    case TrafficClassDirectionEnum::BOTH:
      update_ingress = true;
      update_egress = true;
      break;
  }

  traffic_classes_[id] = std::make_shared<TrafficClass>(*this, conf);

  switch (conf.getDirection()) {
    case TrafficClassDirectionEnum::INGRESS:
      update_ingress = true;
      break;
    
    case TrafficClassDirectionEnum::EGRESS:
      update_egress = true;
      break;

    case TrafficClassDirectionEnum::BOTH:
      update_ingress = true;
      update_egress = true;
      break;
  }

  if (update_ingress) {
    updateProgram(ProgramType::INGRESS);
  }
  if (update_egress) {
    updateProgram(ProgramType::EGRESS);
  }

  logger()->info("Traffic class replaced {0}",
                 traffic_classes_[id]->toString());
}

void Classifier::delTrafficClass(const uint32_t &id) {
  if (traffic_classes_.count(id) == 0) {
    throw std::runtime_error("No traffic class with the given id");
  }

  std::shared_ptr<TrafficClass> tclass = traffic_classes_[id]; 

  traffic_classes_.erase(id);

  updateProgram(tclass->getDirection());

  logger()->info("Traffic class deleted {0}", tclass->toString());
}

void Classifier::delTrafficClassList() {
  traffic_classes_.clear();

  updateProgram(TrafficClassDirectionEnum::BOTH);

  logger()->info("Traffic class list deleted");
}

void Classifier::updateProgram(TrafficClassDirectionEnum direction) {
  switch (direction) {
    case TrafficClassDirectionEnum::INGRESS:
      updateProgram(ProgramType::INGRESS);
      break;
    
    case TrafficClassDirectionEnum::EGRESS:
      updateProgram(ProgramType::EGRESS);
      break;

    case TrafficClassDirectionEnum::BOTH:
      updateProgram(ProgramType::INGRESS);
      updateProgram(ProgramType::EGRESS);
      break;
  }
}

void Classifier::updateProgram(ProgramType direction) {
  std::string direction_str =
      direction == ProgramType::INGRESS ? "ingress" : "egress";

  // Select classes for the given direction
  std::vector<std::shared_ptr<TrafficClass>> classes;
  for (auto const &entry : traffic_classes_) {
    if (static_cast<int>(entry.second->getDirection()) == 
            static_cast<int>(direction) ||
        entry.second->getDirection() == TrafficClassDirectionEnum::BOTH) 
    classes.push_back(entry.second);
  }

  if (classes.size() == 0) {
    // No classes, disable classification
    if (classification_enabled_[direction]) {
      std::string code = selector_code;
      replaceAll(code, "_CLASSIFY", "0");
      replaceAll(code, "_DIRECTION", direction_str);
      reload(code, 0, direction);

      del_program(active_program_[direction], direction);
      classification_enabled_[direction] = false;
      logger()->debug(direction_str + " classification disabled");
    }

    return;
  }

  for (auto &matcher : matchers_) {
    matcher->initBitvector(traffic_classes_.size());
  }

  // Compute bitvectors

  // Sort by priority in decreasing order
  std::sort(classes.begin(), classes.end(),
            [](std::shared_ptr<TrafficClass> c1,
               std::shared_ptr<TrafficClass> c2) { return !(*c1 <= *c2); });

  bool parse_l3 = false, parse_l4 = false;

  for (auto c : classes) {
    bool need_ip = false;
    bool need_tcp_udp = false;

    // sport
    {
      std::shared_ptr<ExactMatcher<uint16_t>> m =
          std::dynamic_pointer_cast<ExactMatcher<uint16_t>>(
              matchers_[FIELD_INDEX(SPORT)]);

      if (c->sportIsSet()) {
        m->appendValueBit(htons(c->getSport()));
        parse_l4 = true;
        need_tcp_udp = true;

      } else {
        m->appendWildcardBit();
      }
    }

    // dport
    {
      std::shared_ptr<ExactMatcher<uint16_t>> m =
          std::dynamic_pointer_cast<ExactMatcher<uint16_t>>(
              matchers_[FIELD_INDEX(DPORT)]);

      if (c->dportIsSet()) {
        m->appendValueBit(htons(c->getDport()));
        parse_l4 = true;
        need_tcp_udp = true;

      } else {
        m->appendWildcardBit();
      }
    }

    // l4proto
    {
      std::shared_ptr<ExactMatcher<uint8_t>> m =
          std::dynamic_pointer_cast<ExactMatcher<uint8_t>>(
              matchers_[FIELD_INDEX(L4PROTO)]);

      if (c->l4protoIsSet()) {
        m->appendValueBit(kL4protos[static_cast<int>(c->getL4proto())]);
        parse_l3 = true;
        need_ip = true;

      } else if (need_tcp_udp) {
        m->appendValuesBit({IPPROTO_TCP, IPPROTO_UDP});
        parse_l3 = true;
        need_ip = true;

      } else {
        m->appendWildcardBit();
      }
    }

    // srcip
    {
      std::shared_ptr<PrefixMatcher<uint32_t>> m =
          std::dynamic_pointer_cast<PrefixMatcher<uint32_t>>(
              matchers_[FIELD_INDEX(SRCIP)]);

      if (c->srcipIsSet()) {
        m->appendValueBit(ip_string_to_nbo_uint(c->getSrcip()),
                          std::stoi(get_netmask_from_string(c->getSrcip())));
        parse_l3 = true;
        need_ip = true;

      } else {
        m->appendWildcardBit();
      }
    }

    // dstip
    {
      std::shared_ptr<PrefixMatcher<uint32_t>> m =
          std::dynamic_pointer_cast<PrefixMatcher<uint32_t>>(
              matchers_[FIELD_INDEX(DSTIP)]);

      if (c->dstipIsSet()) {
        m->appendValueBit(ip_string_to_nbo_uint(c->getDstip()),
                          std::stoi(get_netmask_from_string(c->getDstip())));
        parse_l3 = true;
        need_ip = true;

      } else {
        m->appendWildcardBit();
      }
    }

    // ethtype
    {
      std::shared_ptr<ExactMatcher<uint16_t>> m =
          std::dynamic_pointer_cast<ExactMatcher<uint16_t>>(
              matchers_[FIELD_INDEX(ETHTYPE)]);

      if (c->ethtypeIsSet()) {
        m->appendValueBit(htons(kEthtypes[static_cast<int>(c->getEthtype())]));

      } else if (need_ip) {
        m->appendValueBit(htons(ETH_P_IP));

      } else {
        m->appendWildcardBit();
      }
    }

    // smac
    {
      std::shared_ptr<ExactMatcher<uint64_t>> m =
          std::dynamic_pointer_cast<ExactMatcher<uint64_t>>(
              matchers_[FIELD_INDEX(SMAC)]);

      if (c->smacIsSet()) {
        m->appendValueBit(mac_string_to_nbo_uint(c->getSmac()));

      } else {
        m->appendWildcardBit();
      }
    }

    // dmac
    {
      std::shared_ptr<ExactMatcher<uint64_t>> m =
          std::dynamic_pointer_cast<ExactMatcher<uint64_t>>(
              matchers_[FIELD_INDEX(DMAC)]);

      if (c->dmacIsSet()) {
        m->appendValueBit(mac_string_to_nbo_uint(c->getDmac()));

      } else {
        m->appendWildcardBit();
      }
    }
  }

  // Generate table declaration and matching code
  std::string tables_code, matching_code;
  for (auto &matcher : matchers_) {
    if (matcher->isActive()) {
      tables_code += matcher->getTableCode();
      matching_code += matcher->getMatchingCode();
    }
  }

  // Generate complete code
  std::string code = classifier_code;
  replaceAll(code, "_MATCHING_TABLES", tables_code);
  replaceAll(code, "_MATCHING_CODE", matching_code);
  replaceAll(code, "_SUBVECTS_COUNT",
             std::to_string((classes.size() - 1) / 64 + 1));
  replaceAll(code, "_CLASSES_COUNT", std::to_string(classes.size()));
  replaceAll(code, "_PARSE_L3", std::to_string(static_cast<int>(parse_l3)));
  replaceAll(code, "_PARSE_L4", std::to_string(static_cast<int>(parse_l4)));
  replaceAll(code, "_DIRECTION", direction_str);
  
  // Load new program
  active_program_[direction] = active_program_[direction] % 2 + 1;
  add_program(code, active_program_[direction], direction);
  logger()->debug(std::string("New ") + direction_str + " program loaded");  

  // Fill matching tables
  for (auto &matcher : matchers_) {
    if (matcher->isActive()) {
      matcher->loadTable(active_program_[direction], direction);
      logger()->debug("{0} matching table for {1} loaded", direction_str,
                      FIELD_NAME(matcher->getField()));
    }
  }

  // Fill classes table
  auto class_ids_table = get_array_table<uint32_t>(
      std::string("class_ids"), active_program_[direction], direction);

  for (int i = 0; i < classes.size(); i++) {
    class_ids_table.set(i, classes[i]->getId());
  }
  logger()->debug("{0} classes table loaded", direction_str);

  // Update selector to point to the active program
  code = selector_code;
  replaceAll(code, "_DIRECTION", direction_str);
  replaceAll(code, "_CLASSIFY", "1");
  replaceAll(code, "_NEXT", std::to_string(active_program_[direction]));
  reload(code, 0, direction);
  logger()->debug("{0} selector updated", direction_str);

  if (classification_enabled_[direction]) {
    del_program(active_program_[direction] % 2 + 1, direction);
  }

  classification_enabled_[direction] = true;
}