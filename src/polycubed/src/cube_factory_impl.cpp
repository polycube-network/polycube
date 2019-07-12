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

#include "cube_factory_impl.h"
#include "cube_tc.h"
#include "cube_xdp.h"
#include "service_controller.h"
#include "transparent_cube_tc.h"
#include "transparent_cube_xdp.h"

#include "polycube/common.h"

#include <iostream>
#include <sstream>
#include <string>

namespace polycube {
namespace polycubed {

CubeFactoryImpl::CubeFactoryImpl(const std::string &service_name)
    : service_name_(service_name),
      controller_tc_(Controller::get_tc_instance()),
      controller_xdp_(Controller::get_xdp_instance()),
      datapathlog_(DatapathLog::get_instance()) {}

std::shared_ptr<CubeIface> CubeFactoryImpl::create_cube(
    const nlohmann::json &conf, const std::vector<std::string> &ingress_code,
    const std::vector<std::string> &egress_code, const log_msg_cb &log_msg,
    const set_log_level_cb &log_level_cb, const packet_in_cb &cb) {
  auto name = conf.at("name").get<std::string>();
  auto type = string_to_cube_type(conf.at("type").get<std::string>());
  auto level = stringLogLevel(conf.at("loglevel").get<std::string>());
  auto shadow = conf.at("shadow").get<bool>();
  auto span = conf.at("span").get<bool>();

  auto cube =
      create_cube(name, ingress_code, egress_code, log_msg, type, cb, level, shadow, span);
  auto base = std::dynamic_pointer_cast<BaseCube>(cube);
  base->set_log_level_cb(log_level_cb);
  return std::move(cube);
}

std::shared_ptr<CubeIface> CubeFactoryImpl::create_cube(
    const std::string &name, const std::vector<std::string> &ingress_code,
    const std::vector<std::string> &egress_code, const log_msg_cb &log_msg,
    const CubeType type, const packet_in_cb &cb, LogLevel level, bool shadow, bool span) {
  std::shared_ptr<CubeIface> cube;
  typename std::unordered_map<std::string,
                              std::shared_ptr<BaseCubeIface>>::iterator iter;
  bool inserted;

  switch (type) {
  case CubeType::XDP_SKB:
  case CubeType::XDP_DRV:
    cube = std::make_shared<CubeXDP>(name, service_name_, ingress_code,
                                     egress_code, level, type, shadow, span);
    break;
  case CubeType::TC:
    cube = std::make_shared<CubeTC>(name, service_name_, ingress_code,
                                    egress_code, level, shadow, span);
    break;
  default:
    throw std::runtime_error("invalid cube type");
  }

  std::tie(iter, inserted) = cubes_.emplace(name, cube);
  if (!inserted) {
    return nullptr;
  }

  ServiceController::register_cube(cube, service_name_);
  datapathlog_.register_cb(cube->get_id(), log_msg);
  if (cb) {
    switch (type) {
    case CubeType::XDP_SKB:
    case CubeType::XDP_DRV:
      controller_xdp_.register_cb(cube->get_id(), cb);
      break;
    case CubeType::TC:
      controller_tc_.register_cb(cube->get_id(), cb);
      break;
    }
  }
  return std::move(cube);
}

std::shared_ptr<TransparentCubeIface> CubeFactoryImpl::create_transparent_cube(
    const nlohmann::json &conf, const std::vector<std::string> &ingress_code,
    const std::vector<std::string> &egress_code, const log_msg_cb &log_msg,
    const set_log_level_cb &log_level_cb, const packet_in_cb &cb,
    const attach_cb &attach) {
  auto name = conf.at("name").get<std::string>();
  auto type = string_to_cube_type(conf.at("type").get<std::string>());
  auto level = stringLogLevel(conf.at("loglevel").get<std::string>());

  auto cube = create_transparent_cube(name, ingress_code, egress_code, log_msg,
                                      type, cb, attach, level);
  auto base = std::dynamic_pointer_cast<BaseCube>(cube);
  base->set_log_level_cb(log_level_cb);
  return std::move(cube);
}

std::shared_ptr<TransparentCubeIface> CubeFactoryImpl::create_transparent_cube(
    const std::string &name, const std::vector<std::string> &ingress_code,
    const std::vector<std::string> &egress_code, const log_msg_cb &log_msg,
    const CubeType type, const packet_in_cb &cb, const attach_cb &attach,
    LogLevel level) {
  std::shared_ptr<TransparentCubeIface> cube;
  typename std::unordered_map<std::string,
                              std::shared_ptr<BaseCubeIface>>::iterator iter;
  bool inserted;

  switch (type) {
  case CubeType::XDP_SKB:
  case CubeType::XDP_DRV:
    cube = std::make_shared<TransparentCubeXDP>(
        name, service_name_, ingress_code, egress_code, level, type, attach);
    break;
  case CubeType::TC:
    cube = std::make_shared<TransparentCubeTC>(
        name, service_name_, ingress_code, egress_code, level, attach);
    break;
  default:
    throw std::runtime_error("invalid cube type");
  }

  std::tie(iter, inserted) = cubes_.emplace(name, cube);
  if (!inserted) {
    return nullptr;
  }

  ServiceController::register_cube(cube, service_name_);
  datapathlog_.register_cb(cube->get_id(), log_msg);
  if (cb) {
    switch (type) {
    case CubeType::XDP_SKB:
    case CubeType::XDP_DRV:
      controller_xdp_.register_cb(cube->get_id(), cb);
      break;
    case CubeType::TC:
      controller_tc_.register_cb(cube->get_id(), cb);
      break;
    }
  }

  return std::move(cube);
}

void CubeFactoryImpl::destroy_cube(const std::string &name) {
  auto cube = cubes_.find(name);
  if (cube == cubes_.end()) {
    return;
  }

  uint32_t id = cube->second->get_id();

  switch (cube->second->get_type()) {
  case CubeType::XDP_SKB:
  case CubeType::XDP_DRV:
    controller_xdp_.unregister_cb(id);
    break;
  case CubeType::TC:
    controller_tc_.unregister_cb(id);
    break;
  }

  datapathlog_.unregister_cb(id);
  ServiceController::unregister_cube(name);
  cubes_.erase(name);
}

std::vector<std::shared_ptr<BaseCubeIface>> CubeFactoryImpl::get_cubes() {
  std::vector<std::shared_ptr<BaseCubeIface>> r;
  r.reserve(cubes_.size());

  for (auto &&i : cubes_) {
    r.push_back(i.second);
  }

  return r;
}

}  // namespace polycubed
}  // namespace polycube
