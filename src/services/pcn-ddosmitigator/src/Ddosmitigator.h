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

#include "../interface/DdosmitigatorInterface.h"

#include "polycube/services/port.h"
#include "polycube/services/transparent_cube.h"
#include "polycube/services/utils.h"

#include <spdlog/spdlog.h>

#include "BlacklistDst.h"
#include "BlacklistSrc.h"
#include "Stats.h"

using namespace io::swagger::server::model;
using polycube::service::CubeType;

class Ddosmitigator : public polycube::service::TransparentCube,
                      public DdosmitigatorInterface {
 public:
  Ddosmitigator(const std::string name, const DdosmitigatorJsonObject &conf,
                CubeType type = CubeType::TC);
  virtual ~Ddosmitigator();
  std::string generate_code();
  std::vector<std::string> generate_code_vector();

  void update(const DdosmitigatorJsonObject &conf) override;
  DdosmitigatorJsonObject toJsonObject() override;

  void packet_in(polycube::service::Sense sense,
                 polycube::service::PacketInMetadata &md,
                 const std::vector<uint8_t> &packet) override;

  /// <summary>
  ///
  /// </summary>
  std::shared_ptr<Stats> getStats() override;
  void addStats(const StatsJsonObject &value) override;
  void replaceStats(const StatsJsonObject &conf) override;
  void delStats() override;

  /// <summary>
  /// Name of the ddosmitigator service
  /// </summary>
  std::string getName() override;

  /// <summary>
  /// Defines the logging level of a service instance, from none (OFF) to the
  /// most verbose (TRACE). Default: INFO
  /// </summary>
  DdosmitigatorLoglevelEnum getLoglevel() override;
  void setLoglevel(const DdosmitigatorLoglevelEnum &value) override;

  /// <summary>
  /// Blacklisted destination IP addresses
  /// </summary>
  std::shared_ptr<BlacklistDst> getBlacklistDst(const std::string &ip) override;
  std::vector<std::shared_ptr<BlacklistDst>> getBlacklistDstList() override;
  void addBlacklistDst(const std::string &ip,
                       const BlacklistDstJsonObject &conf) override;
  void addBlacklistDstList(
      const std::vector<BlacklistDstJsonObject> &conf) override;
  void replaceBlacklistDst(const std::string &ip,
                           const BlacklistDstJsonObject &conf) override;
  void delBlacklistDst(const std::string &ip) override;
  void delBlacklistDstList() override;

  /// <summary>
  /// UUID of the Cube
  /// </summary>
  std::string getUuid() override;

  /// <summary>
  /// Type of the Cube (TC, XDP_SKB, XDP_DRV)
  /// </summary>
  CubeType getType() override;

  /// <summary>
  /// Blacklisted source IP addresses
  /// </summary>
  std::shared_ptr<BlacklistSrc> getBlacklistSrc(const std::string &ip) override;
  std::vector<std::shared_ptr<BlacklistSrc>> getBlacklistSrcList() override;
  void addBlacklistSrc(const std::string &ip,
                       const BlacklistSrcJsonObject &conf) override;
  void addBlacklistSrcList(
      const std::vector<BlacklistSrcJsonObject> &conf) override;
  void replaceBlacklistSrc(const std::string &ip,
                           const BlacklistSrcJsonObject &conf) override;
  void delBlacklistSrc(const std::string &ip) override;
  void delBlacklistSrcList() override;

  void replaceAll(std::string &str, const std::string &from,
                  const std::string &to);

 public:
  std::string getCode();
  bool reloadCode();

  void setSrcMatch(bool value);
  void setDstMatch(bool value);

  bool src_match_ = false;
  bool dst_match_ = false;

 private:
  // when code is reloaded it will be set to false
  bool is_code_changed_ = false;

 public:
  std::unordered_map<std::string, BlacklistSrc> blacklistsrc_;
  std::unordered_map<std::string, BlacklistDst> blacklistdst_;
};
