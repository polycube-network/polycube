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


#include "Packet.h"
#include "Packetcapture.h"
#include <locale>
#include <codecvt>
#include <string>


Packet::Packet(Packetcapture &parent, const PacketJsonObject &conf)
    : PacketBase(parent), capture_len(0), packet_len(0), ts_microsec(0), ts_sec(0) {
  if (conf.capturelenIsSet()) {
    setCapturelen(conf.getCapturelen());
  }

  if (conf.packetlenIsSet()) {
    setPacketlen(conf.getPacketlen());
  }

  if (conf.timestampSecondsIsSet()) {
    setTimestampSeconds(conf.getTimestampSeconds());
  }

  if (conf.timestampMicrosecondsIsSet()) {
    setTimestampMicroseconds(conf.getTimestampMicroseconds());
  }

}


Packet::~Packet() {}

uint32_t Packet::getCapturelen() {
  return this->capture_len;
}

void Packet::setCapturelen(const uint32_t &value) {
  this->capture_len = value;
}

uint32_t Packet::getPacketlen() {
  return this->packet_len;
}

void Packet::setPacketlen(const uint32_t &value) {
  this->packet_len = value;
}

uint32_t Packet::getTimestampSeconds() {
  return this->ts_sec;
}

void Packet::setTimestampSeconds(const uint32_t &value) {
  this->ts_sec = value;
}

uint32_t Packet::getTimestampMicroseconds() {
  return this->ts_microsec;
}

void Packet::setTimestampMicroseconds(const uint32_t &value) {
  this->ts_microsec = value;
}

void Packet::setRawPacketData(const std::vector<uint8_t> &input){
  this->packet = input;
}

std::vector<uint8_t> Packet::getRawPacketData(){
  return this->packet;
}

std::string ascii_to_utf8(const std::vector<uint8_t> &packet){
  std::string ret("");
  char ch;

  for (int i = 0 ; i < packet.size(); i++){
    if (packet[i] < 128){
      ret.push_back((char) packet[i]);
    } else {
      ch = (char) packet[i];
      ret.push_back((char)((packet[i] >> 6) | 0xC0));
      ret.push_back((ch & 0x3F) | 0x80);
    }
  }

  return ret;
}

std::string Packet::getRawdata() {
  return ascii_to_utf8(packet);
}

void Packet::setRawdata(const std::string &value) {
  throw std::runtime_error("Packet::setRawdata: Method not implemented");
}
