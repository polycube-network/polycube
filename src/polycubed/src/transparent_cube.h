/*
 * Copyright 2018 The Polycube Authors
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

#include "base_cube.h"

using polycube::service::TransparentCubeIface;
using polycube::service::ParameterEventCallback;

namespace polycube {
namespace polycubed {

class PeerIface;

class TransparentCube : public BaseCube, public TransparentCubeIface {
 public:
  explicit TransparentCube(const std::string &name,
                           const std::string &service_name,
                           PatchPanel &patch_panel, LogLevel level,
                           CubeType type, const service::attach_cb &attach);
  virtual ~TransparentCube();

  // Sets the index of the next program to call after the program of the given
  // direction is executed.
  // The higher 16 bits are reserved for XDP programs, if they are set they
  // override the value of the lower 16 bits.
  void set_next(uint32_t next, ProgramType type);
  uint16_t get_next(ProgramType type);
  void set_parent(PeerIface *parent);
  PeerIface *get_parent();
  void set_parameter(const std::string &parameter, const std::string &value);
  void send_packet_out(const std::vector<uint8_t> &packet, service::Direction direction,
                       bool recirculate = false);

  void set_conf(const nlohmann::json &conf);
  nlohmann::json to_json() const;

  void subscribe_parent_parameter(const std::string &param_name,
                                  ParameterEventCallback &callback);
  void unsubscribe_parent_parameter(const std::string &param_name);
  std::string get_parent_parameter(const std::string &param_name);

 protected:
  void uninit();
  service::attach_cb attach_;
  PeerIface *parent_;
  static std::string get_wrapper_code();
  uint32_t ingress_next_;
  uint32_t egress_next_;

  std::unordered_map<std::string, ParameterEventCallback> subscription_list;
  std::mutex subscription_list_mutex;
};

}  // namespace polycubed
}  // namespace polycube
