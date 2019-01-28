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

#include "bcc_mutex.h"
#include "polycube/services/guid.h"
#include "node.h"
#include "patchpanel.h"
#include "id_generator.h"
#include "polycube/services/cube_iface.h"
#include "polycube/services/json.hpp"
#include "polycube/services/port_iface.h"

#include <api/BPF.h>
#include <api/BPFTable.h>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_sinks.h>

#include <exception>
#include <map>
#include <set>
#include <vector>
#include <string>
#include <sstream>

// Print compileed eBPF C code (wrappers provided by polycubed included)
//   as it is passed to bcc in order to get compiled and executed.
// Print # Lines of Code
// Print # Lines of Code (not empty)
// #define LOG_COMPILEED_CODE

// Print eBPF code compilation time.
// bpf.init      -> time bcc uses to compile C into eBPF bytecode
// bpf.load_func -> time bcc uses to compile eBPF bytecode
// #define LOG_COMPILATION_TIME

// Output of such defines is polycubed log

using polycube::service::CubeIface;
using polycube::service::PortIface;
using polycube::service::ProgramType;
using polycube::service::CubeType;

using json = nlohmann::json;

namespace polycube {
namespace polycubed {

class Cube : public CubeIface {
 public:
  explicit Cube(const std::string &name,
                const std::string &service_name,
                PatchPanel &patch_panel_ingress_,
                PatchPanel &patch_panel_egress_,
                LogLevel level, CubeType type,
                const std::string &master_code = "");
  virtual ~Cube();

  // It is not possible to copy nor assign nor move an cube.
  Cube(const Cube &) = delete;
  Cube &operator=(const Cube &) = delete;
  Cube(Cube &&a) = delete;

  void reload(const std::string &code, int index, ProgramType type);
  int add_program(const std::string &code, int index, ProgramType type);
  void del_program(int index, ProgramType type);

  // ports
  std::shared_ptr<PortIface> add_port(const std::string &name);
  void remove_port(const std::string &name);
  std::shared_ptr<PortIface> get_port(const std::string &name);

  CubeType get_type() const;

  virtual void update_forwarding_table(int index, int value, bool is_netdev);

  uint32_t get_id() const;

  uint16_t get_index(ProgramType type) const;

  const std::string get_name() const;
  const std::string get_service_name() const;
  const Guid &uuid() const;

  int get_table_fd(const std::string &table_name, int index,
                   ProgramType type);

  void set_log_level(LogLevel level);
  LogLevel get_log_level() const;

  void log_compileed_code(std::string& code);

  virtual int load(ebpf::BPF &bpf, ProgramType type) = 0;
  virtual void unload(ebpf::BPF &bpf, ProgramType type) = 0;
  virtual void compile(ebpf::BPF &bpf, const std::string &code,
                      int index, ProgramType type) = 0;

  json toJson(bool include_ports) const;

  static const int _POLYCUBE_MAX_PORTS = 128;
  static const int _POLYCUBE_MAX_BPF_PROGRAMS = 64;

  static std::vector<std::string> cflags;

  // ebpf wrappers
  static const std::string MASTER_CODE;
  static const std::string CUBE_H;

protected:
  CubeType type_;
  void init(const std::vector<std::string> &ingress_code,
            const std::vector<std::string> &egress_code);
  void uninit();

  uint16_t allocate_port_id();
  void release_port_id(uint16_t id);

  PatchPanel &patch_panel_ingress_;
  PatchPanel &patch_panel_egress_;

  std::string name_;
  std::string service_name_;
  Guid uuid_;
  uint32_t id_;

  int ingress_fd_;
  int egress_fd_;
  uint16_t ingress_index_;
  uint16_t egress_index_;

  std::unique_ptr<ebpf::BPF> master_program_;

  std::array<std::unique_ptr<ebpf::BPF>, _POLYCUBE_MAX_BPF_PROGRAMS> ingress_programs_;
  std::array<std::unique_ptr<ebpf::BPF>, _POLYCUBE_MAX_BPF_PROGRAMS> egress_programs_;

  std::array<std::string, _POLYCUBE_MAX_BPF_PROGRAMS> ingress_code_;
  std::array<std::string, _POLYCUBE_MAX_BPF_PROGRAMS> egress_code_;

  std::unique_ptr<ebpf::BPFArrayTable<uint32_t>> forward_chain_;
  std::unique_ptr<ebpf::BPFProgTable> ingress_programs_table_;
  std::unique_ptr<ebpf::BPFProgTable> egress_programs_table_;

  std::map<std::string, std::shared_ptr<PortIface>> ports_by_name_;
  std::map<int, std::shared_ptr<PortIface>> ports_by_index_;
  std::set<uint16_t> free_ports_;  // keeps track of available ports_

  std::shared_ptr<spdlog::logger> logger;
  LogLevel level_;

  std::mutex cube_mutex_;  // blocks operations over the whole cube
 private:
  void do_reload(const std::string &code, int index, ProgramType type);
  static IDGenerator id_generator_;
};

}  // namespace polycubed
}  // namespace polycube
