/*
 * Copyright 2017 The Polycube Authors
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

#include "polycube/services/utils.h"

#include <iomanip>
#include <iostream>
#include <iterator>
#include <random>
#include <sstream>

#include <arpa/inet.h>

#include <stdio.h>
#include <string.h>

#include <api/BPFTable.h>

namespace polycube {
namespace service {
namespace utils {

// new set of functions

uint32_t ip_string_to_nbo_uint(const std::string &ip) {
  unsigned char a[4];
  int last = -1;
  std::string IP_address = get_ip_from_string(ip);

  int rc = std::sscanf(IP_address.c_str(), "%hhu.%hhu.%hhu.%hhu%n", a + 0, a + 1, a + 2,
                       a + 3, &last);
  if (rc != 4 || IP_address.size() != last)
    throw std::runtime_error("Not an ipv4 address " + ip);

  return uint32_t(a[3]) << 24 | uint32_t(a[2]) << 16 | uint32_t(a[1]) << 8 |
         uint32_t(a[0]);
}

std::string nbo_uint_to_ip_string(uint32_t ip) {
  struct in_addr ip_addr;
  ip_addr.s_addr = ip;
  return std::string(inet_ntoa(ip_addr));
}

/* https://stackoverflow.com/a/7326381 */
uint64_t mac_string_to_nbo_uint(const std::string &mac) {
  uint8_t a[6];
  int last = -1;
  int rc = sscanf(mac.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx%n", a + 0, a + 1,
                  a + 2, a + 3, a + 4, a + 5, &last);
  if (rc != 6 || mac.size() != last) {
    throw std::runtime_error("invalid mac address format " + mac);
  }
  return uint64_t(a[5]) << 40 | uint64_t(a[4]) << 32 | uint64_t(a[3]) << 24 |
         uint64_t(a[2]) << 16 | uint64_t(a[1]) << 8 | uint64_t(a[0]);
}

std::string nbo_uint_to_mac_string(uint64_t mac) {
  uint8_t a[6];
  for (int i = 0; i < 6; i++) {
    a[i] = (mac >> i * 8) & 0xFF;
  }

  char str[19];
  std::sprintf(str, "%02x:%02x:%02x:%02x:%02x:%02x", a[0], a[1], a[2], a[3],
               a[4], a[5]);
  return std::string(str);
}

// legacy
std::string ip_string_to_hexbe_string(const std::string &ip) {
  unsigned char a[4];
  int last = -1;
  int rc = std::sscanf(ip.c_str(), "%hhu.%hhu.%hhu.%hhu%n", a + 0, a + 1, a + 2,
                       a + 3, &last);
  if (rc != 4 || ip.size() != last)
    throw std::runtime_error("Not an ipv4 address " + ip);

  std::stringstream stream;
  stream << std::setfill('0') << std::setw(8) << std::hex
         << (uint32_t(a[3]) << 24 | uint32_t(a[2]) << 16 | uint32_t(a[1]) << 8 |
             uint32_t(a[0]));
  return "0x" + stream.str();
}

uint64_t mac_string_to_uint(const std::string &mac) {
  uint64_t x;
  std::string ret("0x");
  for (int index = 15; index >= 0; index -= 3) {
    ret += mac.substr(index, 2);
  }

  std::stringstream ss;
  ss << std::hex << ret;
  ss >> x;

  return x;
}

std::string mac_string_to_hexbe_string(const std::string &mac) {
  std::string ret("0x");
  for (int index = 15; index >= 0; index -= 3) {
    ret += mac.substr(index, 2);
  }

  return ret;
}

// Convert and ip string into the dotted notation (ip must be in network byte
// order)
std::string ip_to_string(uint32_t ip) {
  struct in_addr ip_addr;
  ip_addr.s_addr = ip;
  return std::string(inet_ntoa(ip_addr));
}

// convert a mac into a string representation
std::string mac_to_string(uint64_t mac) {
  uint8_t array[6];
  for (int i = 0; i < 6; i++) {
    array[i] = (mac >> i * 8) & 0xFF;
  }

  char str[19];
  std::sprintf(str, "%02x:%02x:%02x:%02x:%02x:%02x", array[0], array[1],
               array[2], array[3], array[4], array[5]);
  return std::string(str);
}

std::string format_debug_string(std::string str, const uint64_t args[4]) {
  struct replace_element {
    std::string from;
    std::string to;
    size_t pos;
  };

  std::vector<replace_element> to_replace;
  bool to_rellocate[4] = {false};
  int arg_index = 0;
  for (size_t i = 0; i < str.size(); i++) {
    if (str[i] == '%') {
      // %% means to print a '%'. Skip it.
      if (str[i + 1] == '%') {
        i++;
        continue;
      }
      // look for custom specifiers and format them
      if (str[i + 1] == 'I') {
        to_replace.push_back(
            replace_element{"%I", ip_to_string(args[arg_index]), i});
        to_rellocate[arg_index] = true;
      } else if (str[i + 1] == 'M') {
        to_replace.push_back(
            replace_element{"%M", mac_to_string(args[arg_index]), i});
        to_rellocate[arg_index] = true;
      } else if (str[i + 1] == 'P') {
        to_replace.push_back(
            replace_element{"%P", std::to_string(ntohs(args[arg_index])), i});
        to_rellocate[arg_index] = true;
      }
      arg_index++;
    }
  }

  // move the args elements in case there were some custom specifiers
  uint64_t new_args[4];
  for (int i = 0, j = 0; i < 4; i++) {
    if (!to_rellocate[i]) {
      new_args[j] = args[i];
      j++;
    }
  }

  // replace the custom specifiers starting from the back
  for (auto it = to_replace.rbegin(); it != to_replace.rend(); ++it) {
    str.replace(it->pos, 2, it->to);
  }

  // +50 is enough for holding the arguments
  char buf[str.size() + 50];

  std::snprintf(buf, sizeof(buf), str.c_str(), new_args[0], new_args[1],
                new_args[2], new_args[3]);

  return std::string(buf);
}

std::string get_random_mac() {
  std::random_device rd;
  std::mt19937 mt(rd());
  std::uniform_int_distribution<uint64_t> dist(1, UINT64_MAX);

  uint64_t port_mac = dist(mt);
  port_mac &= ~(1ULL << 40);  // unicast
  port_mac |= (1ULL << 41);   // locally administrated

  // Copy mac to uint8_t array
  uint8_t mac[6];
  uint64_t t = port_mac;
  for (int i = 5; i >= 0; i--) {
    mac[i] = t & 0xFF;
    t >>= 8;
  }
  char mac_str[50];
  std::sprintf(mac_str, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2],
               mac[3], mac[4], mac[5]);
  std::string mac_string = mac_str;
  return mac_string;
}

uint64_t hex_string_to_uint(const std::string &str) {
  uint64_t x;
  std::stringstream ss;
  ss << std::hex << str;
  ss >> x;

  return x;
}

std::string get_ip_from_string(const std::string &ipv_net) {
  size_t pos = ipv_net.find("/");
  if (pos == std::string::npos) {
    return ipv_net;
  }
  return ipv_net.substr(0, pos);
}

std::string get_netmask_from_string(const std::string &ipv) {
  std::string ipv_net = ipv;
  size_t pos = ipv_net.find("/");
  if (pos == std::string::npos) {
    return std::string();  // throw?
  }
  return ipv_net.substr(pos + 1, std::string::npos);
}

uint32_t get_netmask_length(const std::string &netmask_string) {
  struct in_addr buf;
  char address[100];
  int res = inet_pton(
      AF_INET, netmask_string.c_str(),
      &buf); /*convert ip address in binary form in network byte order */

  if (res == 1) {
    uint32_t counter = 0;
    int n = buf.s_addr;
    while (n) {
      counter++;
      n &= (n - 1);
    }
    return counter;
  } else
    throw std::runtime_error("IP Address is not in a valid format");
}

std::string get_netmask_from_prefixlength(const int prefixlength) {
  uint32_t ipv4Netmask;

  if (prefixlength == 0) {
    return "0.0.0.0";
  }

  ipv4Netmask = 0xFFFFFFFF;
  ipv4Netmask <<= 32 - prefixlength;
  ipv4Netmask = ntohl(ipv4Netmask);
  struct in_addr addr = {ipv4Netmask};

  return inet_ntoa(addr);
}

void split_ip_and_prefix(const std::string &ip_and_prefix,
                        std::string &ip_address, std::string &netmask) {
  // ip_and_prefix = ip_address/prefix
  std::istringstream split(ip_and_prefix);
  std::vector<std::string> info;
  char split_char = '/';
  for (std::string each; std::getline(split, each, split_char);
       info.push_back(each));
  ip_address = info[0];
  netmask = get_netmask_from_prefixlength(std::atoi(info[1].c_str()));
}

}  // namespace utils
}  // namespace service
}  // namespace polycube
