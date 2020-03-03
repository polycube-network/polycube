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

#include <list>
#include <fstream>
#include <chrono>
#include "../base/PacketcaptureBase.h"
#include "Packet.h"
#include "Globalheader.h"
#include <tins/ethernetII.h>
#include <tins/tins.h>
#include <linux/filter.h>

using namespace polycube::service::model;
using polycube::service::ProgramType;
using polycube::service::Direction;

class Packetcapture : public PacketcaptureBase {

 std::shared_ptr<Globalheader> global_header;
 std::list<std::shared_ptr<Packet>> packets_captured;
 uint8_t CapStatus;
 bool network_mode_flag;
 bool writeHeader;
 std::ofstream myFile;
 std::string dt;
 std::string temp_folder;
 std::string random_number;
 std::chrono::system_clock::time_point timeP;
 std::chrono::nanoseconds temp_offset;
 std::string filter;
 bool bootstrap = true; /* variable used to set default filter when the service is created */

private:
    struct sock_fprog cbpf;
    std::string filterCode;

public:
 void addPacket(const std::vector<uint8_t> &packet);
 void to_timeval(std::chrono::system_clock::duration& d, struct timeval& tv);

 public:
  Packetcapture(const std::string name, const PacketcaptureJsonObject &conf);
  virtual ~Packetcapture();

  void packet_in(polycube::service::Direction direction,
                 polycube::service::PacketInMetadata &md,
                 const std::vector<uint8_t> &packet) override;

  void attach() override;

  /// <summary>
  /// Packet capture status
  /// </summary>
  PacketcaptureCaptureEnum getCapture() override;
  void setCapture(const PacketcaptureCaptureEnum &value) override;

  /// <summary>
  ///
  /// </summary>
  bool getAnonimize() override;
  void setAnonimize(const bool &value) override;

  /// <summary>
  /// dump capture
  /// </summary>
  std::string getDump() override;
  void setDump(const std::string &value) override;
  /// <summary>
  /// Operating mode
  /// </summary>
  bool getNetworkmode() override;
  void setNetworkmode(const bool &value) override;

  /// <summary>
  /// filtering string (e.g., 'host 1.2.3.4 and src port 80')
  /// </summary>
  void setFilter(const std::string &value) override;
  std::string getFilter() override;

  /// <summary>
  ///
  /// </summary>
  std::shared_ptr<Packet> getPacket() override;
  void addPacket(const PacketJsonObject &value) override;
  void replacePacket(const PacketJsonObject &conf) override;
  void delPacket() override;

  /// <summary>
  ///
  /// </summary>
  std::shared_ptr<Globalheader> getGlobalheader() override;
  void addGlobalheader(const GlobalheaderJsonObject &value) override;
  void replaceGlobalheader(const GlobalheaderJsonObject &conf) override;
  void delGlobalheader() override;

  /// <summary>
  /// Snapshot length
  /// </summary>
  uint32_t getSnaplen() override;
  void setSnaplen(const uint32_t &value) override;

  void updateFilter();
  void writeDump(const std::vector<uint8_t> &packet);

  void filterCompile(std::string str, struct sock_fprog *cbpf);

  std::string replaceAll(std::string str, const std::string &from, const std::string &to);
};