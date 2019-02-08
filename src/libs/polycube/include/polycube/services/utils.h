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

#include <arpa/inet.h>
#include <netinet/in.h>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#pragma once

namespace polycube {
namespace service {
namespace utils {

/* ip (a.b.c.d) to string and viceversa in big endian */
uint32_t ip_string_to_be_uint(const std::string &ip);
std::string be_uint_to_ip_string(uint32_t ip);

/* mac (aa:bb:cc:dd:ee:ff) to string and vicersa in big endian */
uint64_t mac_string_to_be_uint(const std::string &mac);
std::string be_uint_to_mac_string(uint64_t mac);

/* transforms an ipv4 dotted representation into a hexadecimal big endian */
/* deprecated */
std::string ip_string_to_hexbe_string(const std::string &ip);

/* transforms a MAC address (such as "aa:bb:cc:dd:ee:ff") into an hexadecimal
 * big endian */
/* deprecated */
std::string mac_string_to_hexbe_string(const std::string &mac);

/* transforms an hexadecimal string into an unsigned integer */
uint64_t hex_string_to_uint(const std::string &str);

/* Creates a random MAC address */
std::string get_random_mac();

/* Take in ingress a string like 192.168.0.1/24 and return only the ip
 * 192.168.0.1 */
std::string get_ip_from_string(const std::string &ipv_net);

/* Take in ingress a string like 192.168.0.1/24 and return only the "prefix
 * length" -> 24 in this case */
std::string get_netmask_from_string(const std::string &ipv_net);

/*
 * formats a debug string, custom specifiers are evaluated by a
 * custom implemented logic
 */
std::string format_debug_string(std::string str, const uint64_t args[4]);

#ifdef HAVE_POLYCUBE_TOOLS
void print_packet(const uint8_t *pkt, uint32_t len);
#endif

}  // namespace utils
}  // namespace service
}  // namespace polycube
