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

#pragma once

#include <functional>
#include <string>
#include <vector>

#include "polycube/common.h"
#include "polycube/services/json.hpp"

namespace polycube {
namespace service {

class CubeIface;
class TransparentCubeIface;
enum class CubeType;

struct PacketInMetadata {
  uint32_t traffic_class;
  uint32_t reason;
  uint32_t metadata[3];
};

struct __attribute__((__packed__)) PacketIn {
  uint16_t cube_id;       /**< Index of the Cube within the patchpanel */
  uint16_t port_id;       /**< Port where the packet was received */
  uint32_t packet_len;    /**< Total length of the packet */
  uint32_t traffic_class; /**< Traffic class the packet belongs to */
  uint16_t reason;        /**< Internal code between dataplane and control
                               plane */
  uint32_t metadata[3];   /**< Buffer that can be used by the dataplane to send
                               additional information to the control plane */
};

typedef std::function<void(const PacketIn *md,
                           const std::vector<uint8_t> &packet)>
    packet_in_cb;

static packet_in_cb empty_packet_in_cb;

struct __attribute__((__packed__)) LogMsg {
  uint16_t type;
  uint16_t cube_id; /**< System-wide cube id */
  uint16_t level;   /**< Log level */
  uint16_t len;     /**< Length of the format message */
  uint64_t args[4];
  char msg[0];
};

typedef std::function<void(const LogMsg *msg)> log_msg_cb;

typedef std::function<void(void)> attach_cb;

typedef std::function<void(LogLevel level)> set_log_level_cb;

class CubeFactory {
 public:
  virtual std::shared_ptr<CubeIface> create_cube(
      const nlohmann::json &conf, const std::vector<std::string> &ingress_code,
      const std::vector<std::string> &egress_code, const log_msg_cb &log_msg,
      const set_log_level_cb &log_level_cb,
      const packet_in_cb &cb = empty_packet_in_cb) = 0;

  virtual std::shared_ptr<TransparentCubeIface> create_transparent_cube(
      const nlohmann::json &conf, const std::vector<std::string> &ingress_code,
      const std::vector<std::string> &egress_code, const log_msg_cb &log_msg,
      const set_log_level_cb &log_level_cb, const packet_in_cb &cb,
      const attach_cb &attach) = 0;

  virtual void destroy_cube(const std::string &name) = 0;
};

}  // namespace service
}  // namespace polycube
