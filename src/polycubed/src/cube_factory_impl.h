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

//#include "polycube/services/guid.h"
#include "polycube/services/cube_factory.h"
#include "polycube/services/cube_iface.h"

#include "controller.h"
#include "datapath_log.h"

#include <memory>

namespace polycube {
namespace polycubed {

using service::BaseCubeIface;
using service::CubeIface;
using service::TransparentCubeIface;
using service::CubeFactory;
using service::packet_in_cb;
using service::log_msg_cb;
using service::set_log_level_cb;
using service::attach_cb;

class CubeFactoryImpl : public CubeFactory {
 public:
  CubeFactoryImpl(const std::string &service_name);
  ~CubeFactoryImpl() = default;

  std::shared_ptr<CubeIface> create_cube(
      const nlohmann::json &conf, const std::vector<std::string> &ingress_code,
      const std::vector<std::string> &egress_code, const log_msg_cb &log_msg,
      const set_log_level_cb &log_level_cb, const packet_in_cb &cb) override;

  std::shared_ptr<TransparentCubeIface> create_transparent_cube(
      const nlohmann::json &conf, const std::vector<std::string> &ingress_code,
      const std::vector<std::string> &egress_code, const log_msg_cb &log_msg,
      const set_log_level_cb &log_level_cb, const packet_in_cb &cb,
      const attach_cb &attach) override;

  void destroy_cube(const std::string &name) override;

  std::vector<std::shared_ptr<BaseCubeIface>> get_cubes();

 private:
  std::shared_ptr<CubeIface> create_cube(
      const std::string &name, const std::vector<std::string> &ingress_code,
      const std::vector<std::string> &egress_code, const log_msg_cb &log_msg,
      const CubeType type, const packet_in_cb &cb, LogLevel level, bool shadow, bool span);

  std::shared_ptr<TransparentCubeIface> create_transparent_cube(
      const std::string &name, const std::vector<std::string> &ingress_code,
      const std::vector<std::string> &egress_code, const log_msg_cb &log_msg,
      const CubeType type, const packet_in_cb &cb, const attach_cb &attach,
      LogLevel level);

  std::string service_name_;
  std::unordered_map<std::string, std::shared_ptr<BaseCubeIface>> cubes_;
  Controller &controller_tc_;
  Controller &controller_xdp_;
  DatapathLog &datapathlog_;
};

}  // namespace polycubed
}  // namespace polycube
