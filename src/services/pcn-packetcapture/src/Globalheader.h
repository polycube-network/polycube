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


#include "../base/GlobalheaderBase.h"


class Packetcapture;

using namespace polycube::service::model;

class Globalheader : public GlobalheaderBase {

  uint32_t snaplen;

 public:
  Globalheader(Packetcapture &parent, const GlobalheaderJsonObject &conf);
  virtual ~Globalheader();

  /// <summary>
  /// magic number
  /// </summary>
  uint32_t getMagic() override;
  void setMagic(const uint32_t &value) override;

  /// <summary>
  /// major version number
  /// </summary>
  uint16_t getVersionMajor() override;
  void setVersionMajor(const uint16_t &value) override;

  /// <summary>
  /// minor version number
  /// </summary>
  uint16_t getVersionMinor() override;
  void setVersionMinor(const uint16_t &value) override;

  /// <summary>
  /// GMT to local correction
  /// </summary>
  int32_t getThiszone() override;
  void setThiszone(const int32_t &value) override;

  /// <summary>
  /// accuracy of timestamps
  /// </summary>
  uint32_t getSigfigs() override;
  void setSigfigs(const uint32_t &value) override;

  /// <summary>
  /// max length of captured packets, in octets
  /// </summary>
  uint32_t getSnaplen() override;
  void setSnaplen(const uint32_t &value) override;

  /// <summary>
  /// Capture linktype (eg. ethernet, wifi..)
  /// </summary>
  uint32_t getLinktype() override;
  void setLinktype(const uint32_t &value) override;
};
