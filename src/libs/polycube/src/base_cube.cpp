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

#include "polycube/services/base_cube.h"

namespace polycube {
namespace service {

BaseCube::BaseCube(const nlohmann::json &conf,
                   const std::vector<std::string> &ingress_code,
                   const std::vector<std::string> &egress_code)
  : dismounted_(false),
    logger_(std::make_shared<spdlog::logger>(
          conf.at("name").get<std::string>(), (spdlog::sinks_init_list){
                    std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                        logfile_, 1048576 * 5, 3),
                    std::make_shared<spdlog::sinks::stdout_sink_mt>()})) {
  auto loglevel_ = stringLogLevel(conf.at("loglevel").get<std::string>());
  logger()->set_level(logLevelToSPDLog(loglevel_));
  handle_log_msg = [&](const LogMsg *msg) -> void { datapath_log_msg(msg); };
}

BaseCube::~BaseCube() {
  // just in case
  dismount();
}

int BaseCube::get_table_fd(const std::string &table_name, int index,
                           ProgramType type) {
  return cube_->get_table_fd(table_name, index, type);
}

void BaseCube::reload(const std::string &code, int index, ProgramType type) {
  cube_->reload(code, index, type);
}

int BaseCube::add_program(const std::string &code, int index,
                          ProgramType type) {
  return cube_->add_program(code, index, type);
}

void BaseCube::del_program(int index, ProgramType type) {
  cube_->del_program(index, type);
}

RawTable BaseCube::get_raw_table(const std::string &table_name, int index,
                                 ProgramType type) {
  int fd = get_table_fd(table_name, index, type);
  RawTable t(&fd);
  return std::move(t);
}

void BaseCube::datapath_log_msg(const LogMsg *msg) {
  spdlog::level::level_enum level_ =
      logLevelToSPDLog((polycube::LogLevel)msg->level);
  std::string print;

  switch (msg->type) {
  case 0:
    print = utils::format_debug_string(msg->msg, msg->args);
    logger()->log(level_, print.c_str());
    break;

  default:
    logger()->warn("Received bad message type in datapath_log_msg");
    return;
  }
}

void BaseCube::set_log_level(LogLevel level) {
  set_control_plane_log_level(level);
  cube_->set_log_level(level);
}

void BaseCube::set_control_plane_log_level(LogLevel level) {
  logger()->set_level(logLevelToSPDLog(level));
}

LogLevel BaseCube::get_log_level() const {
  return cube_->get_log_level();
}

const Guid &BaseCube::get_uuid() const {
  return cube_->uuid();
}

const std::string BaseCube::get_name() const {
  return cube_->get_name();
}

const std::string BaseCube::getName() const {
  return get_name();
}

CubeType BaseCube::get_type() const {
  return cube_->get_type();
}

std::shared_ptr<spdlog::logger> BaseCube::logger() {
  return logger_;
}

void BaseCube::dismount() {
  std::lock_guard<std::mutex> guard(cube_mutex);

  if (dismounted_)
    return;

  dismounted_ = true;
  // invalidate handlers
  handle_log_msg = nullptr;

  // TODO: remove from controller and datapathlog?
}

nlohmann::json BaseCube::to_json() const {
  return cube_->to_json();
}

}  // namespace service
}  // namespace polycube
