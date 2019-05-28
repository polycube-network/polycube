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

#define VLAN_WILDCARD 0x00

enum entry_type { STATIC = 0, DYNAMIC };

struct port {
  uint16_t mode;
  uint16_t native_vlan;
  bool native_vlan_enabled;
} __attribute__((packed));

struct port_vlan_key {
  uint32_t port;
  uint16_t vlan;
} __attribute__((packed));

struct port_vlan_value {
  uint16_t vlan;
  uint16_t stp_state;
} __attribute__((packed));

struct fwd_key {
  uint16_t vlan;
  uint64_t mac;
} __attribute__((packed));

struct fwd_entry {
  uint32_t timestamp;
  uint32_t port;
  enum entry_type type;
} __attribute__((packed));
