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

#include "UtilityMethods.h"
#include "Router.h"

/*utility methods*/

std::string from_int_to_hex(int t) {
  std::stringstream stream;
  stream << "0x" << std::hex << t;
  return stream.str();
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
    throw std::runtime_error("IP Address is not in a valide format");
}

unsigned int ip_to_int(const char *ip) {
  unsigned value = 0;
  // bytes processed.
  int i;
  // next digit to process.
  const char *start;

  start = ip;
  for (i = 0; i < 4; i++) {
    char c;
    int n = 0;
    while (1) {
      c = *start;
      start++;
      if (c >= '0' && c <= '9') {
        n *= 10;
        n += c - '0';
      }
      /* We insist on stopping at "." if we are still parsing
         the first, second, or third numbers. If we have reached
         the end of the numbers, we will allow any character. */
      else if ((i < 3 && c == '.') || i == 3) {
        break;
      } else {
        std::string to_ip(ip);
        throw std::runtime_error("Ip address is not in a valide format");
      }
    }
    if (n >= 256) {
      throw std::runtime_error("Ip address is not in a valide format");
    }
    value *= 256;
    value += n;
  }
  return value;
}

bool address_in_subnet(const std::string &ip, const std::string &netmask,
                       const std::string &network) {
  uint32_t ipAddress = ip_to_int(ip.c_str());
  uint32_t mask = ip_to_int(netmask.c_str());
  uint32_t net = ip_to_int(network.c_str());
  if ((ipAddress & mask) == (net & mask))
    return true;
  else
    return false;
}

std::string get_network_from_ip(const std::string &ip,
                                const std::string &netmask) {
  // get the network from ip
  uint32_t address = ip_to_int(ip.c_str());
  uint32_t mask = ip_to_int(netmask.c_str());
  uint32_t net = address & mask;
  char buffer[100];
  sprintf(buffer, "%d.%d.%d.%d", (net >> 24) & 0xFF, (net >> 16) & 0xFF,
          (net >> 8) & 0xFF, (net)&0xFF);
  std::string network(buffer);
  return network;
}

bool is_netmask_valid(const std::string &netmask) {
  uint32_t mask = ip_to_int(netmask.c_str());
  /*if (mask == 0)
    return false;*/
  if (mask & (~mask >> 1)) {
    return false;
  } else {
    return true;
  }
}
