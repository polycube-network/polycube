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

#include "../interface/TransparenthelloworldInterface.h"

#include "polycube/services/fifo_map.hpp"
#include "polycube/services/port.h"
#include "polycube/services/transparent_cube.h"
#include "polycube/services/utils.h"

#include <spdlog/spdlog.h>
#include <tins/ethernetII.h>

using namespace io::swagger::server::model;
using polycube::service::ProgramType;

class Transparenthelloworld : public polycube::service::TransparentCube,
                              public TransparenthelloworldInterface {
 public:
  Transparenthelloworld(const std::string name,
                        const TransparenthelloworldJsonObject &conf);
  virtual ~Transparenthelloworld();

  void attach() override;
  void packet_in(polycube::service::Direction direction,
                 polycube::service::PacketInMetadata &md,
                 const std::vector<uint8_t> &packet) override;
  void update(const TransparenthelloworldJsonObject &conf) override;
  TransparenthelloworldJsonObject toJsonObject() override;

  /// <summary>
  /// Action performed on ingress packets
  /// </summary>
  TransparenthelloworldIngressActionEnum getIngressAction() override;
  void setIngressAction(
      const TransparenthelloworldIngressActionEnum &value) override;

  /// <summary>
  /// Action performed on egress packets
  /// </summary>
  TransparenthelloworldEgressActionEnum getEgressAction() override;
  void setEgressAction(
      const TransparenthelloworldEgressActionEnum &value) override;
};
