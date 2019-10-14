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

#include "../base/PacketBase.h"


class Packetcapture;

using namespace polycube::service::model;

class Packet : public PacketBase {

  uint32_t ts_microsec;
  uint32_t ts_sec;
  uint32_t packet_len;
  uint32_t capture_len;

 public:
  std::vector<uint8_t> packet;
  Packet(Packetcapture &parent, const PacketJsonObject &conf);
  virtual ~Packet();

  /// <summary>
  /// Capture length
  /// </summary>
  uint32_t getCapturelen() override;
  void setCapturelen(const uint32_t &value) override;

  /// <summary>
  /// Packet length
  /// </summary>
  uint32_t getPacketlen() override;
  void setPacketlen(const uint32_t &value) override;

  /// <summary>
  /// timestamp - seconds
  /// </summary>
  uint32_t getTimestampSeconds() override;
  void setTimestampSeconds(const uint32_t &value) override;

  /// <summary>
  /// timestamp - microseconds
  /// </summary>
  uint32_t getTimestampMicroseconds() override;
  void setTimestampMicroseconds(const uint32_t &value) override;

  void setRawPacketData(const std::vector<uint8_t> &input);
  std::vector<uint8_t> getRawPacketData();

  /// <summary>
  /// packet raw data
  /// </summary>
  std::string getRawdata() override;
  void setRawdata(const std::string &value) override;
};
