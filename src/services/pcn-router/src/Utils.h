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

/*utility methods*/

#include <string>

/* Take in ingress any int and return the hex in the form "0x.." */
std::string int_to_hex(int t);

unsigned int ip_to_int(const char *ip);

/* Take in ingress an ip, a netmask and a network and return true
*  if the ip is part of the network */
bool address_in_subnet(const std::string &ip, const std::string &netmask,
                       const std::string &network);

/* Take in ingress an ip and a netmask and return the network */
std::string get_network_from_ip(const std::string &ip,
                                const std::string &netmask);

bool is_netmask_valid(const std::string &netmask);
