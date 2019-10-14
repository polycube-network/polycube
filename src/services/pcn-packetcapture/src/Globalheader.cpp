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


#include "Globalheader.h"
#include "Packetcapture.h"
#define LINKTYPE_ETHERNET 1
#define MAGICNUMBER 0xa1b2c3d4
#define MAJOR_VERSION 2
#define MINOR_VERSION 4



Globalheader::Globalheader(Packetcapture &parent, const GlobalheaderJsonObject &conf)
    : GlobalheaderBase(parent) {
  if (conf.magicIsSet()) {
    setMagic(conf.getMagic());
  }

  if (conf.versionMajorIsSet()) {
    setVersionMajor(conf.getVersionMajor());
  }

  if (conf.versionMinorIsSet()) {
    setVersionMinor(conf.getVersionMinor());
  }

  if (conf.thiszoneIsSet()) {
    setThiszone(conf.getThiszone());
  }

  if (conf.sigfigsIsSet()) {
    setSigfigs(conf.getSigfigs());
  }

  if (conf.snaplenIsSet()) {
    setSnaplen(conf.getSnaplen());
  }

  if (conf.linktypeIsSet()) {
    setLinktype(conf.getLinktype());
  }

}

Globalheader::~Globalheader() {}

uint32_t Globalheader::getMagic() {
  return MAGICNUMBER;
}

void Globalheader::setMagic(const uint32_t &value) {
  throw std::runtime_error("Globalheader::setMagic: Method not implemented");
}

uint16_t Globalheader::getVersionMajor() {
  return MAJOR_VERSION;
}

void Globalheader::setVersionMajor(const uint16_t &value) {
  throw std::runtime_error("Globalheader::setVersionMajor: Method not implemented");
}

uint16_t Globalheader::getVersionMinor() {
  return MINOR_VERSION;
}

void Globalheader::setVersionMinor(const uint16_t &value) {
  throw std::runtime_error("Globalheader::setVersionMinor: Method not implemented");
}

int32_t Globalheader::getThiszone() {
  return 0x0;
}

void Globalheader::setThiszone(const int32_t &value) {
  throw std::runtime_error("Globalheader::setThiszone: Method not implemented");
}

uint32_t Globalheader::getSigfigs() {
  return 0x0;
}

void Globalheader::setSigfigs(const uint32_t &value) {
  throw std::runtime_error("Globalheader::setSigfigs: Method not implemented");
}

uint32_t Globalheader::getSnaplen() {
  return snaplen;
}

void Globalheader::setSnaplen(const uint32_t &value) {
  snaplen = value;
}

uint32_t Globalheader::getLinktype() {
  return LINKTYPE_ETHERNET;
}

void Globalheader::setLinktype(const uint32_t &value) {
  throw std::runtime_error("Globalheader::setLinktype: Method not implemented");
}


