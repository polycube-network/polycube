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

#include "bcc_mutex.h"
#include "id_generator.h"
#include "patchpanel.h"
#include "polycube/services/cube_factory.h"
#include "polycube/services/cube_iface.h"
#include "polycube/services/guid.h"
#include "polycube/services/json.hpp"

#include <api/BPF.h>
#include <api/BPFTable.h>

#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>

#include <exception>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using polycube::service::BaseCubeIface;
using polycube::service::PortIface;
using polycube::service::ProgramType;
using polycube::service::CubeType;

using json = nlohmann::json;

namespace polycube {
namespace polycubed {

class BaseCube : virtual public BaseCubeIface {
 public:
  explicit BaseCube(const std::string &name, const std::string &service_name,
                    const std::string &master_code,
                    PatchPanel &patch_panel_ingress_,
                    PatchPanel &patch_panel_egress_, LogLevel level,
                    CubeType type);
  virtual ~BaseCube();

  // It is not possible to copy nor assign nor move an cube.
  BaseCube(const BaseCube &) = delete;
  BaseCube &operator=(const BaseCube &) = delete;
  BaseCube(BaseCube &&a) = delete;

  void reload(const std::string &code, int index, ProgramType type);
  int add_program(const std::string &code, int index, ProgramType type);
  void del_program(int index, ProgramType type);

  static std::string get_wrapper_code();

  CubeType get_type() const;
  uint32_t get_id() const;
  uint16_t get_index(ProgramType type) const;
  const std::string get_name() const;
  const std::string get_service_name() const;
  const Guid &uuid() const;

  int get_table_fd(const std::string &table_name, int index, ProgramType type);
  const ebpf::TableDesc &get_table_desc(const std::string &table_name, int index,
                             ProgramType type);

  void set_log_level(LogLevel level);
  LogLevel get_log_level() const;

  void set_conf(const nlohmann::json &conf);
  virtual nlohmann::json to_json() const;

  void set_log_level_cb(const polycube::service::set_log_level_cb &cb);

 protected:
  static const int _POLYCUBE_MAX_BPF_PROGRAMS = 64;
  static const int _POLYCUBE_MAX_PORTS = 128;
  static_assert(_POLYCUBE_MAX_PORTS <= 0xffff,
          "_POLYCUBE_MAX_PORTS shouldn't be great than 0xffff, "
          "id 0xffff was used by iptables wild card index");
  static std::vector<std::string> cflags;

  virtual int load(ebpf::BPF &bpf, ProgramType type) = 0;
  virtual void unload(ebpf::BPF &bpf, ProgramType type) = 0;
  virtual void compile(ebpf::BPF &bpf, const std::string &code, int index,
                       ProgramType type) = 0;

  void init(const std::vector<std::string> &ingress_code,
            const std::vector<std::string> &egress_code);
  void uninit();

  void reload_all();

  PatchPanel &patch_panel_ingress_;
  PatchPanel &patch_panel_egress_;

  CubeType type_;
  std::string name_;
  std::string service_name_;
  Guid uuid_;
  uint32_t id_;

  int ingress_fd_;
  int egress_fd_;
  uint16_t ingress_index_;
  uint16_t egress_index_;

  std::unique_ptr<ebpf::BPF> master_program_;

  std::array<std::unique_ptr<ebpf::BPF>, _POLYCUBE_MAX_BPF_PROGRAMS>
      ingress_programs_;
  std::array<std::unique_ptr<ebpf::BPF>, _POLYCUBE_MAX_BPF_PROGRAMS>
      egress_programs_;

  std::array<std::string, _POLYCUBE_MAX_BPF_PROGRAMS> ingress_code_;
  std::array<std::string, _POLYCUBE_MAX_BPF_PROGRAMS> egress_code_;

  std::unique_ptr<ebpf::BPFProgTable> ingress_programs_table_;
  std::unique_ptr<ebpf::BPFProgTable> egress_programs_table_;

  std::shared_ptr<spdlog::logger> logger;
  LogLevel level_;

  std::mutex cube_mutex_;  // blocks operations over the whole cube
 private:
  // ebpf wrappers
  static const std::string BASECUBE_MASTER_CODE;
  static const std::string BASECUBE_WRAPPER;

  void do_reload(const std::string &code, int index, ProgramType type);
  static IDGenerator id_generator_;

  polycube::service::set_log_level_cb log_level_cb_;
};

}  // namespace polycubed
}  // namespace polycube
