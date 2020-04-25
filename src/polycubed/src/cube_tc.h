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

#include "cube.h"
#include "polycube/services/guid.h"

#include <api/BPF.h>
#include <api/BPFTable.h>

#include <spdlog/spdlog.h>

#include <exception>
#include <map>
#include <set>
#include <vector>

namespace polycube {
namespace polycubed {

class PortTC;
class CubeXDP;
class TransparentCubeTC;
class TransparetCubeXDP;

class CubeTC : public Cube {
  friend class PortTC;
  friend class CubeXDP;
  friend class TransparentCubeTC;
  friend class TransparentCubeXDP;

 public:
  explicit CubeTC(const std::string &name, const std::string &service_name,
                  const std::vector<std::string> &ingres_code,
                  const std::vector<std::string> &egress_code, LogLevel level,
                  bool shadow, bool span);
  virtual ~CubeTC();

 protected:
  static std::string get_wrapper_code();

  static void do_compile(int module_index, ProgramType type, LogLevel level_,
                         ebpf::BPF &bpf, const std::string &code, int index, bool shadow, bool span);
  static int do_load(ebpf::BPF &bpf);
  static void do_unload(ebpf::BPF &bpf);

  void compile(ebpf::BPF &bpf, const std::string &code, int index,
               ProgramType type);
  int load(ebpf::BPF &bpf, ProgramType type);
  void unload(ebpf::BPF &bpf, ProgramType type);

  static void send_packet_ns_span_mode(void *cb_cookie, void *data, int data_size);
  void start_thread_span_mode();
  void stop_thread_span_mode();
  bool stop_thread_;
  std::unique_ptr<std::thread> pkt_in_thread_;

  void set_span(const bool value) override;

 private:
  static const std::string CUBE_TC_COMMON_WRAPPER;
  static const std::string CUBETC_HELPERS;
  static const std::string CUBETC_WRAPPER;
};

}  // namespace polycubed
}  // namespace polycube
