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

#include "polycube/services/transparent_cube.h"
#include "polycube/services/port.h"
#include "polycube/services/utils.h"
#include "polycube/services/fifo_map.hpp"

#include <spdlog/spdlog.h>

using namespace io::swagger::server::model;
using polycube::service::CubeType;
using polycube::service::ProgramType;

class Transparenthelloworld : public polycube::service::TransparentCube, public TransparenthelloworldInterface {
 public:
  Transparenthelloworld(const std::string name,
                        const TransparenthelloworldJsonObject &conf,
                        CubeType type = CubeType::TC);
  virtual ~Transparenthelloworld();

  void attach() override;
  void packet_in(polycube::service::Sense sense,
                 polycube::service::PacketInMetadata &md,
                 const std::vector<uint8_t> &packet) override;
  void update(const TransparenthelloworldJsonObject &conf) override;
  TransparenthelloworldJsonObject toJsonObject() override;

  /// <summary>
  /// Name of the transparenthelloworld service
  /// </summary>
  std::string getName() override;

  /// <summary>
  /// UUID of the Cube
  /// </summary>
  std::string getUuid() override;

  /// <summary>
  /// Type of the Cube (TC, XDP_SKB, XDP_DRV)
  /// </summary>
  CubeType getType() override;

  /// <summary>
  /// Defines the logging level of a service instance, from none (OFF) to the most verbose (TRACE)
  /// </summary>
  TransparenthelloworldLoglevelEnum getLoglevel() override;
  void setLoglevel(const TransparenthelloworldLoglevelEnum &value) override;

  /// <summary>
  /// Action performed on ingress packets
  /// </summary>
  TransparenthelloworldIngressActionEnum getIngressAction() override;
  void setIngressAction(const TransparenthelloworldIngressActionEnum &value) override;

  /// <summary>
  /// Action performed on egress packets
  /// </summary>
  TransparenthelloworldEgressActionEnum getEgressAction() override;
  void setEgressAction(const TransparenthelloworldEgressActionEnum &value) override;
};
